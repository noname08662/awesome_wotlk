#pragma once

#include <windows.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <format>
#include <initializer_list>
#include <memory>
#include <new>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "include/FrameScript/FrameScript.h"
#include "include/Graphics/CGxDevice.h"
#include "include/Graphics/CGxDeviceD3d.h"
#include "include/M2/CM2Model.h"
#include "include/Math/Math.h"
#include "include/Math/Primitives.h"
#include "include/Widget/CCooldown.h"
#include "include/Widget/CSimpleFrame.h"
#include "include/Widget/CSimpleModel.h"
#include "include/Widget/CSimpleScrollFrame.h"
#include "include/Widget/CSimpleTop.h"

#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "blpcodec/deps/stb/stb_image_write.h"

namespace misc_capture_frame {
namespace fmt {
// FRAMECAPTURE_* format string
inline std::string fromString(std::string_view text) {
    std::string literal = "\"";
    for (unsigned char c : text) {
        if (c == '\\' || c == '"') {
            literal += '\\';
            literal += static_cast<char>(c);
        } else if (c < 0x20) {
            literal += std::format("\\{:03}", c);
        } else {
            literal += static_cast<char>(c);
        }
    }
    return literal + '"';
}

inline std::string fromNumber(double value) { return std::format("{}", value); }

inline std::string fromFormat(std::string_view global, std::initializer_list<std::string> args = {}) {
    std::string expr = std::format("({} or \"{}\"):format(", global, global);
    const auto* separator = "";
    for (const std::string& arg : args) {
        expr += std::exchange(separator, ", ");
        expr += arg;
    }
    return expr + ')';
}
}  // namespace fmt

inline constexpr uint32_t kStripRows = 256;
inline constexpr int kMaxSamples = 8;
inline constexpr uint64_t kPixelBudget = 32ull << 20;   // pixels of each full-size surface
inline constexpr uint64_t kSampleBudget = 64ull << 20;  // pixels x samples of the multisampled colour and depth targets
inline constexpr float kMinScale = 0.25f;

struct CaptureRequest {
    CSimpleFrame* frame = nullptr;
    float longest_side = 0.0f;
    float scale = 1.0f;
};

struct FrameCaptureState {
    CaptureRequest pending;
    CSimpleFrame* root = nullptr;
    TSGrowableArray<std::string> batch{{}, 8};
    TSGrowableArray<std::string> messages{{}, 8};  // Lua chunks run on the next update

    bool isInCaptureTree(const CSimpleFrame* frame) const {
        for (const CSimpleFrame* it = frame; it != nullptr; it = it->parent_) {
            if (it == root) { return true; }
        }
        return false;
    }

    bool isAncestorOfCaptureRoot(const CSimpleFrame* frame) const {
        for (const CSimpleFrame* it = root->parent_; it != nullptr; it = it->parent_) {
            if (it == frame) { return true; }
        }
        return false;
    }

    void capturePrint(std::string message_expr) {
        messages.push_back(std::format("print(\"|cff33ff99FrameCapture|r: \" .. {})", message_expr));
    }

    void captureFailed(std::string_view global, std::initializer_list<std::string> args = {}) {
        capturePrint(fmt::fromFormat("FRAMECAPTURE_FAILED", {fmt::fromFormat(global, args)}));
    }

    void flushCaptureMessages() {
        if (messages.empty()) { return; }
        batch.swap(messages);
        for (const std::string& chunk : batch) {
            static_cast<void>(framescript::execute{}(chunk.c_str(), "FrameCapture", nullptr));
        }
        batch.clear();
        batch.shrink_to_fit();
    }
};

static_assert(std::is_trivially_destructible_v<FrameCaptureState>);
inline constinit FrameCaptureState frame_capture{};

// every OnFrameRender override writes its regions through the base, so filtering here isolates the captured tree in
// both the strata batches and the scroll children's own batches
HOOKKIT_BIND(CSimpleFrame::onFrameRenderLayerBase_hook, [](CSimpleFrame* self, CRenderBatch* batch, uint32_t layer) {
    if (frame_capture.root != nullptr && !frame_capture.isInCaptureTree(self)) { return; }
    self->onFrameRenderLayerBase(batch, layer);
});

// the one OnFrameRender override that queues its own geometry (swipe and edge) after the base instead of through
// regions or render callbacks, so the base filter alone lets other frames' cooldowns through
HOOKKIT_BIND(CCooldown::onFrameRenderLayer_hook, [](CCooldown* self, CRenderBatch* batch, uint32_t layer) {
    if (frame_capture.root != nullptr && !frame_capture.isInCaptureTree(self)) { return; }
    self->onFrameRenderLayer(batch, layer);
});

// CM2Scene::Animate drains the scene's animate and draw lists, which only CSimpleModel::OnLayerUpdate refills, once per
// frame; the on-screen pass has already used them up, so arm the model again the same way before each capture pass
inline void captureRenderModel(void* param) {
    auto* model_frame = static_cast<CSimpleModel*>(param);
    CM2Model* model = model_frame->model_;
    if (model != nullptr && model_frame->is_drawn_ != 0 && model_frame->pending_camera_index_ == -1) {
        model->setAnimating(1);
        const uint32_t queue = model->attach_parent_ != nullptr
            ? CM2Model::eDrawWithParent | CM2Model::eParticlesWithParent
            : CM2Model::eDrawThisFrame | CM2Model::eParticlesThisFrame;
        model->flags10_ = static_cast<CM2Model::Flags10>(model->flags10_ | queue);
    }
    CSimpleModel::renderModel{}(model_frame);
}

// render callbacks (models, cooldowns, the world, scroll children) of the captured tree, plus the scroll frames on the
// path down to it so a target inside a scroll child is still drawn through its clip
HOOKKIT_BIND(CRenderBatch::newCbNode_hook, [](CRenderBatch* self, void(__cdecl* callback)(void*), CSimpleFrame* frame) {
    if (frame_capture.root != nullptr) {
        const auto address = reinterpret_cast<uintptr_t>(callback);
        const bool scroll_path =
            address == CSimpleScrollFrame::renderScrollChild::kAddress && frame_capture.isAncestorOfCaptureRoot(frame);
        if (!scroll_path && !frame_capture.isInCaptureTree(frame)) { return reinterpret_cast<void*>(callback); }
        if (address == CSimpleModel::renderModel::kAddress) { callback = captureRenderModel; }
    }
    return self->newCbNode(callback, frame);
});

struct ComRelease {
    void operator()(IUnknown* object) const {
        if (object != nullptr) { object->Release(); }
    }
};

using SurfacePtr = std::unique_ptr<IDirect3DSurface9, ComRelease>;

inline SurfacePtr createTarget(IDirect3DDevice9* d3d, uint32_t width, uint32_t height, D3DMULTISAMPLE_TYPE samples) {
    IDirect3DSurface9* surface = nullptr;
    if (FAILED(d3d->CreateRenderTarget(width, height, D3DFMT_A8R8G8B8, samples, 0, FALSE, &surface, nullptr))) {
        return nullptr;
    }
    return SurfacePtr{surface};
}

inline bool hasStencil(D3DFORMAT format) {
    return format == D3DFMT_D24S8 || format == D3DFMT_D24X4S4 || format == D3DFMT_D15S1 || format == D3DFMT_D24FS8;
}

// copies rows [y0, y0 + rows) of a resolved pass through a strip-sized target, so the readback never needs a
// full-size system memory surface
struct StripReader {
    IDirect3DDevice9* d3d;
    IDirect3DSurface9* strip;
    IDirect3DSurface9* sys;
    uint32_t width;

    bool read(IDirect3DSurface9* source, uint32_t y0, uint32_t rows, std::vector<uint32_t>& out) const {
        const RECT src_rect{0, static_cast<LONG>(y0), static_cast<LONG>(width), static_cast<LONG>(y0 + rows)};
        const RECT dst_rect{0, 0, static_cast<LONG>(width), static_cast<LONG>(rows)};
        if (FAILED(d3d->StretchRect(source, &src_rect, strip, &dst_rect, D3DTEXF_NONE))) { return false; }
        if (FAILED(d3d->GetRenderTargetData(strip, sys))) { return false; }
        D3DLOCKED_RECT locked{};
        if (FAILED(sys->LockRect(&locked, nullptr, D3DLOCK_READONLY))) { return false; }
        out.resize(static_cast<size_t>(width) * rows);
        const auto pitch = static_cast<size_t>(locked.Pitch);
        const std::span src(static_cast<const uint8_t*>(locked.pBits), pitch * rows);
        for (uint32_t y = 0; y < rows; ++y) {
            std::memcpy(&out[y * width], &src[y * pitch], static_cast<size_t>(width) * 4);
        }
        sys->UnlockRect();
        return true;
    }
};

// straight alpha from the same render over black and over white: white - black = 1 - alpha
inline int matteAlpha(uint32_t on_black, uint32_t on_white) {
    int alpha_sum = 0;
    for (int c = 0; c < 3; ++c) {
        const int black = static_cast<int>((on_black >> (8 * c)) & 0xFF);
        const int white = static_cast<int>((on_white >> (8 * c)) & 0xFF);
        alpha_sum += 255 - std::clamp(white - black, 0, 255);
    }
    return (alpha_sum + 1) / 3;
}

inline void writeMattePixel(uint32_t on_black, int alpha, std::span<uint8_t, 4> dst) {
    for (int c = 0; c < 3; ++c) {
        const int black = static_cast<int>((on_black >> (8 * c)) & 0xFF);
        dst[2 - c] = static_cast<uint8_t>(std::min(255, (black * 255 + alpha / 2) / alpha));
    }
    dst[3] = static_cast<uint8_t>(alpha);
}

enum class MatteResult { eSaved, eEmpty, eReadFailed, eOutOfMemory, eWriteFailed };

// two sweeps over the strips: find the drawn box, then unmatte just that box into the PNG
inline MatteResult writeCapturePng(const std::string& path, const StripReader& reader, IDirect3DSurface9* black,
    IDirect3DSurface9* white, uint32_t height, Recti& out_box) {
    const uint32_t width = reader.width;
    std::vector<uint32_t> black_rows;
    std::vector<uint32_t> white_rows;
    uint32_t min_x = width;
    uint32_t min_y = height;
    uint32_t max_x = 0;
    uint32_t max_y = 0;
    for (uint32_t y0 = 0; y0 < height; y0 += kStripRows) {
        const uint32_t rows = std::min(kStripRows, height - y0);
        if (!reader.read(black, y0, rows, black_rows) || !reader.read(white, y0, rows, white_rows)) {
            return MatteResult::eReadFailed;
        }
        for (uint32_t y = 0; y < rows; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                const size_t i = y * width + x;
                if (matteAlpha(black_rows[i], white_rows[i]) == 0) { continue; }
                min_x = std::min(min_x, x);
                max_x = std::max(max_x, x);
                min_y = std::min(min_y, y0 + y);
                max_y = std::max(max_y, y0 + y);
            }
        }
    }
    if (min_x > max_x || min_y > max_y) { return MatteResult::eEmpty; }

    const uint32_t box_w = max_x - min_x + 1;
    const uint32_t box_h = max_y - min_y + 1;
    std::vector<uint8_t> rgba;
    try {
        rgba.resize(static_cast<size_t>(box_w) * box_h * 4);
    } catch (const std::bad_alloc&) { return MatteResult::eOutOfMemory; }
    for (uint32_t y0 = min_y - (min_y % kStripRows); y0 <= max_y; y0 += kStripRows) {
        const uint32_t rows = std::min(kStripRows, height - y0);
        if (!reader.read(black, y0, rows, black_rows) || !reader.read(white, y0, rows, white_rows)) {
            return MatteResult::eReadFailed;
        }
        for (uint32_t y = std::max(y0, min_y); y < y0 + rows && y <= max_y; ++y) {
            for (uint32_t x = min_x; x <= max_x; ++x) {
                const size_t i = (y - y0) * width + x;
                const int alpha = matteAlpha(black_rows[i], white_rows[i]);
                if (alpha == 0) { continue; }
                const size_t offset = (static_cast<size_t>(y - min_y) * box_w + (x - min_x)) * 4;
                writeMattePixel(black_rows[i], alpha, std::span(rgba).subspan(offset).first<4>());
            }
        }
    }
    out_box.min_x = static_cast<int32_t>(min_x);
    out_box.min_y = static_cast<int32_t>(min_y);
    out_box.max_x = static_cast<int32_t>(max_x + 1);
    out_box.max_y = static_cast<int32_t>(max_y + 1);
    const bool written = stbi_write_png(path.c_str(), static_cast<int>(box_w), static_cast<int>(box_h), 4, rgba.data(),
                             static_cast<int>(box_w * 4)) != 0;
    return written ? MatteResult::eSaved : MatteResult::eWriteFailed;
}

inline std::string capturePath(CSimpleFrame* frame) {
    std::string name;
    if (const char* raw = frame->getName()) {
        for (const char ch : std::string_view(raw)) {
            if (std::isalnum(static_cast<unsigned char>(ch)) != 0 || ch == '_') { name += ch; }
        }
    }
    if (name.empty()) { name = "Anonymous"; }
    SYSTEMTIME t{};
    GetLocalTime(&t);
    CreateDirectoryA("Screenshots", nullptr);
    return std::format("Screenshots\\FrameCapture_{}_{:04}{:02}{:02}_{:02}{:02}{:02}_{:03}.png", name, t.wYear,
        t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
}

// window pixels spanned by the shown frames of the tree (regions hanging outside their frame aren't counted)
inline Vec2f captureTreeSize(CSimpleTop* top, float window_w, float window_h) {
    bool any = false;
    Rectf bounds{};
    for (CSimpleFrame* frame = top->frames_.head(); frame != nullptr; frame = top->frames_.next(frame)) {
        if (frame->is_drawn_ == 0 || !frame_capture.isInCaptureTree(frame)) { continue; }
        const Rectf& rect = frame->rect_;
        if (rect.right <= rect.left || rect.top <= rect.bottom) { continue; }
        if (!any) {
            bounds = rect;
            any = true;
            continue;
        }
        bounds.left = std::min(bounds.left, rect.left);
        bounds.right = std::max(bounds.right, rect.right);
        bounds.bottom = std::min(bounds.bottom, rect.bottom);
        bounds.top = std::max(bounds.top, rect.top);
    }
    if (!any) { return {}; }
    const Vec2f ndc = math::ddcToNdc({bounds.right - bounds.left, bounds.top - bounds.bottom});
    return {ndc.x * window_w, ndc.y * window_h};
}

inline void captureFrame(CSimpleTop* top, const CaptureRequest& request) {
    CGxDevice* device = CGxDevice::getDevice();
    CGxDeviceD3d* d3d_device = CGxDeviceD3d::of(device);
    if (d3d_device == nullptr || d3d_device->d3d_device_ == nullptr) {
        frame_capture.captureFailed("FRAMECAPTURE_NO_DEVICE");
        return;
    }
    IDirect3DDevice9* d3d = d3d_device->d3d_device_;
    const float window_w = device->cur_window_.right;
    const float window_h = device->cur_window_.top;
    if (window_w < 1.0f || window_h < 1.0f) { return; }

    frame_capture.root = request.frame;
    const Vec2f tree = captureTreeSize(top, window_w, window_h);
    frame_capture.root = nullptr;
    const float tree_longest = std::max(tree.x, tree.y);
    if (tree_longest < 1.0f) {
        frame_capture.captureFailed("FRAMECAPTURE_NO_SIZE");
        return;
    }

    // every UI viewport is normalized x cur_window_, so scaling the window scales models, scroll children and the
    // minimap with everything else; clamp uniformly or the aspect would change
    const float wanted = request.longest_side > 0.0f ? request.longest_side / tree_longest : request.scale;
    const float max_scale = std::min({static_cast<float>(d3d_device->caps_.MaxTextureWidth) / window_w,
        static_cast<float>(d3d_device->caps_.MaxTextureHeight) / window_h,
        std::sqrt(static_cast<float>(kPixelBudget) / (window_w * window_h))});
    const float scale = std::clamp(wanted, kMinScale, std::max(kMinScale, max_scale));
    const auto width = static_cast<uint32_t>(std::lround(window_w * scale));
    const auto height = static_cast<uint32_t>(std::lround(window_h * scale));

    D3DFORMAT depth_format = D3DFMT_D24S8;
    D3DSURFACE_DESC depth_desc{};
    if (d3d_device->default_depth_surface_ != nullptr &&
        SUCCEEDED(d3d_device->default_depth_surface_->GetDesc(&depth_desc))) {
        depth_format = depth_desc.Format;
    }

    int samples = kMaxSamples;
    while (samples > 1 && static_cast<uint64_t>(width) * height * samples > kSampleBudget) {
        samples /= 2;
    }
    SurfacePtr color;
    SurfacePtr depth;
    for (;; samples /= 2) {
        const auto ms = samples > 1 ? static_cast<D3DMULTISAMPLE_TYPE>(samples) : D3DMULTISAMPLE_NONE;
        color = createTarget(d3d, width, height, ms);
        IDirect3DSurface9* raw_depth = nullptr;
        if (color != nullptr &&
            SUCCEEDED(d3d->CreateDepthStencilSurface(width, height, depth_format, ms, 0, TRUE, &raw_depth, nullptr))) {
            depth.reset(raw_depth);
            break;
        }
        color.reset();
        if (samples <= 1) {
            frame_capture.captureFailed(
                "FRAMECAPTURE_NO_RENDER_TARGET", {fmt::fromNumber(width), fmt::fromNumber(height)});
            return;
        }
    }
    const SurfacePtr black = createTarget(d3d, width, height, D3DMULTISAMPLE_NONE);
    const SurfacePtr white = createTarget(d3d, width, height, D3DMULTISAMPLE_NONE);
    const SurfacePtr strip = createTarget(d3d, width, kStripRows, D3DMULTISAMPLE_NONE);
    IDirect3DSurface9* raw_sys = nullptr;
    if (black == nullptr || white == nullptr || strip == nullptr ||
        FAILED(d3d->CreateOffscreenPlainSurface(
            width, kStripRows, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &raw_sys, nullptr))) {
        frame_capture.captureFailed(
            "FRAMECAPTURE_NO_READBACK_SURFACES", {fmt::fromNumber(width), fmt::fromNumber(height)});
        return;
    }
    const SurfacePtr sys{raw_sys};

    // bound straight through d3d: the engine keeps thinking it draws to the backbuffer, so its viewport and scissor
    // math keeps the backbuffer's Y orientation and nothing the engine tracks has to be restored
    IDirect3DSurface9* raw_saved_color = nullptr;
    IDirect3DSurface9* raw_saved_depth = nullptr;
    d3d->GetRenderTarget(0, &raw_saved_color);
    d3d->GetDepthStencilSurface(&raw_saved_depth);
    const SurfacePtr saved_color{raw_saved_color};
    const SurfacePtr saved_depth{raw_saved_depth};
    const Rectf saved_window = device->cur_window_;

    device->cur_window_.right = static_cast<float>(width);
    device->cur_window_.top = static_cast<float>(height);
    d3d->SetRenderTarget(0, color.get());
    d3d->SetDepthStencilSurface(depth.get());
    frame_capture.root = request.frame;
    top->markAllBatchesDirty();

    const DWORD clear_flags = D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | (hasStencil(depth_format) ? D3DCLEAR_STENCIL : 0);
    const auto render_pass = [&](D3DCOLOR clear_color, IDirect3DSurface9* resolved) {
        const D3DVIEWPORT9 full_viewport{0, 0, width, height, 0.0f, 1.0f};
        const RECT full_rect{0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
        d3d->SetViewport(&full_viewport);
        d3d->SetScissorRect(&full_rect);
        d3d->Clear(0, nullptr, clear_flags, clear_color, 1.0f, 0);
        device->viewport_dirty_ = 1;
        device->scissor_dirty_ = 1;
        top->onLayerRender();
        return SUCCEEDED(d3d->StretchRect(color.get(), nullptr, resolved, nullptr, D3DTEXF_NONE));
    };
    const bool rendered = render_pass(0xFF000000, black.get()) && render_pass(0xFFFFFFFF, white.get());

    frame_capture.root = nullptr;
    device->cur_window_ = saved_window;
    d3d->SetRenderTarget(0, saved_color.get());
    d3d->SetDepthStencilSurface(saved_depth.get());
    device->viewport_dirty_ = 1;
    device->scissor_dirty_ = 1;
    top->markAllBatchesDirty();

    if (!rendered) {
        frame_capture.captureFailed("FRAMECAPTURE_RENDER_FAILED");
        return;
    }
    const std::string path = capturePath(request.frame);
    const StripReader reader{.d3d = d3d, .strip = strip.get(), .sys = sys.get(), .width = width};
    Recti box{};
    switch (writeCapturePng(path, reader, black.get(), white.get(), height, box)) {
        case MatteResult::eSaved:
            break;
        case MatteResult::eEmpty:
            frame_capture.captureFailed("FRAMECAPTURE_NOTHING_DRAWN");
            return;
        case MatteResult::eReadFailed:
            frame_capture.captureFailed("FRAMECAPTURE_READBACK_FAILED");
            return;
        case MatteResult::eOutOfMemory:
            frame_capture.captureFailed("FRAMECAPTURE_OUT_OF_MEMORY");
            return;
        case MatteResult::eWriteFailed:
            frame_capture.captureFailed("FRAMECAPTURE_WRITE_FAILED", {fmt::fromString(path)});
            return;
    }
    const std::string clamped = wanted > scale + 0.001f
        ? fmt::fromFormat("FRAMECAPTURE_CLAMPED", {fmt::fromNumber(wanted)})
        : fmt::fromString("");
    const std::string msaa = samples > 1 ? fmt::fromFormat("FRAMECAPTURE_MSAA", {fmt::fromNumber(samples)})
                                         : fmt::fromFormat("FRAMECAPTURE_NO_MSAA");
    frame_capture.capturePrint(fmt::fromFormat("FRAMECAPTURE_SAVED",
        {fmt::fromString(path), fmt::fromNumber(box.max_x - box.min_x), fmt::fromNumber(box.max_y - box.min_y),
            fmt::fromNumber(scale), clamped, msaa}));
}

inline void updateCaptureHooks() {
    const bool wanted = frame_capture.pending.frame != nullptr;
    if (wanted == CSimpleTop::onLayerRender_hook::attached.load(std::memory_order_acquire)) { return; }

    hookkit::HookTransaction tx;
    if (!tx) { return; }
    if (wanted) {
        tx.attach(CSimpleTop::onLayerRender_hook{}, CSimpleFrame::onFrameRenderLayerBase_hook{},
            CRenderBatch::newCbNode_hook{}, CCooldown::onFrameRenderLayer_hook{});
    } else {
        tx.detach(CSimpleTop::onLayerRender_hook{}, CSimpleFrame::onFrameRenderLayerBase_hook{},
            CRenderBatch::newCbNode_hook{}, CCooldown::onFrameRenderLayer_hook{});
    }

    if (tx.commit() != NO_ERROR && wanted) {
        frame_capture.pending = {};
        frame_capture.captureFailed("FRAMECAPTURE_HOOKS_FAILED");
    }
}

HOOKKIT_BIND(CSimpleTop::onLayerRender_hook, [](CSimpleTop* self) {
    self->onLayerRender();
    if (frame_capture.pending.frame != nullptr) { captureFrame(self, std::exchange(frame_capture.pending, {})); }
    misc_capture_frame::frame_capture.flushCaptureMessages();
    updateCaptureHooks();
});
}  // namespace misc_capture_frame
