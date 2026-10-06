#include "D3D.h"

#include <d3dcompiler.h>
#include <hookkit/accessor.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <map>
#include <span>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "include/Graphics/CGxDevice.h"
#include "include/Graphics/CGxDeviceD3d.h"
#include "include/Lib/Storm.h"

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dcompiler.lib")

bool d3d::bls::load(
    const Bls& bls, GxShaderType type, const CGxDevice* device, std::span<CGxShader*> shaders, uint32_t first) {
    if (type >= eGxShaderCount || shaders.empty()) { return false; }
    const uint64_t begin = static_cast<uint64_t>(bls.record) + first;
    const uint64_t end = begin + shaders.size();
    for (uint32_t profile = device->caps_.shader_targets[type]; profile > 0;
        profile = CGxDevice::nextShaderProfile(type, profile)) {
        const char* profile_name = CGxDevice::shaderProfileName(type, profile);
        if (profile_name == nullptr) { continue; }
        const std::string file = bls.path + '\\' + profile_name + '\\' + bls.name + ".bls";
        void* handle = nullptr;
        if (storm::file::open{}(file.c_str(), &handle) == 0 || handle == nullptr) { continue; }
        CGxShaderFileHeader header{};
        bool holds = storm::file::read{}(handle, &header, sizeof(header), nullptr, nullptr, nullptr) &&
            header.magic == CGxShaderFileHeader::kMagic && header.version == CGxShaderFileHeader::kVersion &&
            end <= header.record_count;
        for (uint64_t i = 0; holds && i < end; ++i) {
            CGxShader* shader = shaders[i < begin ? 0 : static_cast<size_t>(i - begin)];
            holds = shader->load(handle);
            shader->is_created_ = 0;
            shader->_unk[0] = 0;
        }
        storm::file::close{}(handle);
        if (holds) { return true; }
    }
    return false;
}

namespace {
struct Definition {
    explicit Definition(CGxShaderExt::Source src) : source(std::move(src)) {}

    Definition(const Definition&) = delete;
    Definition& operator=(const Definition&) = delete;
    Definition(Definition&&) = delete;
    Definition& operator=(Definition&&) = delete;
    ~Definition() = default;

    CGxShaderExt::Source source;
    // {defines, mutation, profile}; empty: failed
    std::map<std::tuple<std::string, uint32_t, uint32_t>, std::vector<uint8_t>> bytecode;
    bool force_fallback = false;

    [[nodiscard]]
    const d3d::bls::Bls* metaSource() const {
        if (source.meta_from) { return &*source.meta_from; }
        return source.fallback ? &*source.fallback : nullptr;
    }
};

inline constexpr utils::Accessor<std::map<std::pair<GxShaderType, std::string>, Definition>,
    struct ShaderDefinitionsTag>
    kDefinitions;

// ShaderCreate asks for "name" with count mutations, IShaderReload for a single node's own name ("name" or "name:k")
std::pair<Definition*, uint32_t> findDefinition(GxShaderType type, std::string_view name) {
    if (const auto it = kDefinitions->find({type, std::string(name)}); it != kDefinitions->end()) {
        return {&it->second, 0};
    }
    const size_t colon = name.rfind(':');
    if (colon == std::string_view::npos) { return {nullptr, 0}; }
    const std::string_view suffix = name.substr(colon + 1);
    if (suffix.empty() || suffix.size() > 4) { return {nullptr, 0}; }
    uint32_t mutation = 0;
    for (const char c : suffix) {
        if (c < '0' || c > '9') { return {nullptr, 0}; }
        mutation = mutation * 10 + static_cast<uint32_t>(c - '0');
    }
    const auto it = kDefinitions->find({type, std::string(name.substr(0, colon))});
    if (it == kDefinitions->end()) { return {nullptr, 0}; }
    return {&it->second, mutation};
}

struct Defines {
    explicit Defines(const CGxShaderExt::Source& source) {
        if (source.defines) { list = source.defines(); }
        for (const auto& [name, value] : list) {
            macros.push_back({.Name = name.c_str(), .Definition = value.c_str()});
            key.append(name).append(1, '=').append(value).append(1, ';');
        }
        macros.push_back({.Name = nullptr, .Definition = nullptr});
    }

    ~Defines() = default;
    Defines(const Defines&) = delete;
    Defines& operator=(const Defines&) = delete;
    Defines(Defines&&) = delete;
    Defines& operator=(Defines&&) = delete;

    std::vector<CGxShaderExt::Define> list;
    std::vector<D3D_SHADER_MACRO> macros;
    std::string key;
};

// steps down from the engine's .bls profile until D3DCompile accepts one; capped at SM3, failures are cached too
const std::vector<uint8_t>* compile(
    Definition& def, const Defines& defines, GxShaderType type, uint32_t mutation, const CGxDevice* device) {
    if (def.source.hlsl.empty() || mutation >= def.source.entries.size() || type >= eGxShaderCount) { return nullptr; }
    for (uint32_t profile = std::min(device->caps_.shader_targets[type], CGxDeviceD3d::maxShaderProfile(type));
        profile > 0; --profile) {
        const auto [it, fresh] = def.bytecode.try_emplace({defines.key, mutation, profile});
        std::vector<uint8_t>& stored = it->second;
        if (fresh) {
            const char* target = CGxDevice::shaderProfileName(type, profile);
            const std::string& entry = def.source.entries[mutation];
            ID3DBlob* code = nullptr;
            ID3DBlob* error = nullptr;
            const HRESULT hr = target != nullptr
                ? D3DCompile(def.source.hlsl.data(), def.source.hlsl.size(), nullptr, defines.macros.data(), nullptr,
                      entry.c_str(), target, 0, 0, &code, &error)
                : E_INVALIDARG;
            if (error != nullptr) { error->Release(); }
            if (code != nullptr) {
                if (SUCCEEDED(hr)) {
                    const std::span bytes(static_cast<const uint8_t*>(code->GetBufferPointer()), code->GetBufferSize());
                    stored.assign(bytes.begin(), bytes.end());
                }
                code->Release();
            }
        }
        if (!stored.empty()) { return &stored; }
    }
    return nullptr;
}

HOOKKIT_BIND(CGxDevice::iShaderLoad_hook,
    [](CGxDevice* self, CGxShader** shaders, GxShaderType type, const char* path, const char* name,
        uint32_t count) -> uint32_t {
        const auto [def, first] = name != nullptr ? findDefinition(type, name) : std::pair<Definition*, uint32_t>{};
        if (def == nullptr) { return CGxDevice::iShaderLoad_hook{}(self, shaders, type, path, name, count); }

        const std::span nodes{shaders, count};
        const CGxShaderExt::Source& src = def->source;
        const auto loadFallback = [&]() -> uint32_t {
            return src.fallback && d3d::bls::load(*src.fallback, type, self, nodes, first) ? count : 0;
        };
        if (def->force_fallback || (src.use_fallback && src.use_fallback())) { return loadFallback(); }

        const Defines defines(src);
        std::vector<const std::vector<uint8_t>*> compiled(count);
        for (uint32_t i = 0; i < count; ++i) {
            compiled[i] = compile(*def, defines, type, first + i, self);
            if (compiled[i] == nullptr) { return loadFallback(); }
        }
        const d3d::bls::Bls* meta_source = def->metaSource();
        const bool has_meta = meta_source != nullptr && d3d::bls::load(*meta_source, type, self, nodes, first);
        for (uint32_t i = 0; i < count; ++i) {
            CGxShaderExt* ext = CGxShaderExt::of(nodes[i]);
            if (!has_meta) { ext->clearMeta(); }
            ext->setBytecode(*compiled[i]);
        }
        return count;
    });

int texProc(CGxTex::ProcOp op, int32_t, int32_t, int32_t face, int32_t mip_level, int32_t param, uint32_t* out_pitch,
    void** out_pixels) {
    auto* source = reinterpret_cast<CGxTexExt::Source*>(param);
    if (source == nullptr) { return 1; }
    switch (op) {
        case CGxTex::eGenerate: {
            uint32_t pitch = 0;
            const void* pixels = source->pixels(face, mip_level, pitch);
            *out_pitch = pitch;
            *out_pixels = const_cast<void*>(pixels);
            break;
        }
        case CGxTex::eRecreate:
            source->onRecreate();
            break;
        default:
            break;
    }
    return 1;
}
}  // namespace

void CGxShaderExt::define(GxShaderType type, const Name& name, Source&& source) {
    std::string name_z = name.str();
    assert(!name_z.empty() && name_z != kPrefix && "CGxShaderExt::define: empty shader name");
    [[maybe_unused]]
    const bool claimed = kDefinitions->try_emplace({type, std::move(name_z)}, std::move(source)).second;
    assert(claimed && "CGxShaderExt::define: shader name defined twice for one type");
}

bool CGxShaderExt::acquire(GxShaderType type, const Name& name, std::span<CGxShader*> out) {
    CGxDeviceD3dExt* device = CGxDeviceD3dExt::of();
    if (device == nullptr || out.empty()) { return false; }
    const std::string name_z = name.str();
    Definition* def = findDefinition(type, name_z).first;
    while (true) {
        device->shaderCreate(out.data(), type, kPath, name_z.c_str(), out.size());
        if (std::ranges::all_of(out, [](CGxShader* shader) { return shader != nullptr && of(shader)->usable(); })) {
            return true;
        }
        for (CGxShader*& shader : out) {
            if (shader != nullptr) { device->shaderDestroy(&shader); }
        }
        if (device->d3d() == nullptr || def == nullptr || def->force_fallback || !def->source.fallback) {
            return false;
        }
        def->force_fallback = true;
    }
}

CGxShaderExt* CGxShaderExt::acquire(GxShaderType type, const Name& name) {
    CGxShader* shader = nullptr;
    return acquire(type, name, {&shader, 1}) ? of(shader) : nullptr;
}

bool CGxShaderExt::replace(std::initializer_list<Binding> bindings, const Accept& accept) {
    CGxDeviceD3dExt* device = CGxDeviceD3dExt::of();
    if (device == nullptr || bindings.size() == 0) { return false; }
    size_t total = 0;
    for (const Binding& binding : bindings) {
        if (binding.slots.empty()) { return false; }
        total += binding.slots.size();
    }

    std::vector<CGxShader*> fresh(total);
    bool staged = true;
    size_t offset = 0;
    for (const auto& [type, name, slots] : bindings) {
        if (!acquire(type, name, std::span{fresh}.subspan(offset, slots.size()))) {
            staged = false;
            break;
        }
        offset += slots.size();
    }
    if (!staged || (accept && !accept(fresh))) {
        for (CGxShader*& shader : fresh) {
            if (shader != nullptr) { device->shaderDestroy(&shader); }
        }
        return false;
    }

    offset = 0;
    for (const Binding& binding : bindings) {
        for (CGxShader*& slot : binding.slots) {
            if (CGxShader* old = std::exchange(slot, fresh[offset++])) { device->shaderDestroy(&old); }
        }
    }
    return true;
}

bool CGxShaderExt::replace(GxShaderType type, std::span<CGxShader*> slots, const Name& name, const Accept& accept) {
    return replace({Binding{.type = type, .name = name, .slots = slots}}, accept);
}

std::vector<CGxShaderExt*> CGxShaderExt::loaded(
    GxShaderType type, const std::function<bool(const CGxShaderExt*)>& filter) {
    std::vector<CGxShaderExt*> nodes;
    CGxDeviceD3dExt* device = CGxDeviceD3dExt::of();
    if (device == nullptr || type >= std::size(device->shader_tables_)) { return nodes; }
    const auto& list = device->shader_tables_[type].fulllist;
    for (CGxShader* shader = list.head(); shader != nullptr; shader = list.next(shader)) {
        if (!filter || filter(of(shader))) { nodes.push_back(of(shader)); }
    }
    return nodes;
}

size_t CGxShaderExt::reloadAll() {
    size_t reloaded = 0;
    for (const GxShaderType type : {eGxShaderVertex, eGxShaderPixel}) {
        const auto owned = loaded(type, [type](const CGxShaderExt* shader) {
            return shader->name_ != nullptr && findDefinition(type, shader->name_).first != nullptr;
        });
        for (CGxShaderExt* shader : owned) {
            reloaded += shader->reload() ? 1 : 0;
        }
    }
    return reloaded;
}

bool CGxShaderExt::reload() {
    CGxDeviceD3dExt* device = CGxDeviceD3dExt::of();
    if (device == nullptr || name_ == nullptr) { return false; }
    Definition* def = findDefinition(type_, name_).first;
    if (def == nullptr) { return false; }
    const auto reloadOnce = [&] {
        if (IUnknown* object = std::exchange(shader_, nullptr)) { object->Release(); }
        is_created_ = 0;
        device->shaderReload(this, kPath, name_);
    };
    reloadOnce();
    if (usable()) { return true; }
    if (device->d3d() == nullptr || def->force_fallback || !def->source.fallback) { return false; }
    def->force_fallback = true;
    reloadOnce();
    return usable();
}

CGxTexExt* CGxTexExt::create(const Desc& desc, Source* source) {
    CGxDeviceD3dExt* device = CGxDeviceD3dExt::of();
    if (device == nullptr || desc.width <= 0 || desc.height <= 0) { return nullptr; }
    CGxTex* tex = nullptr;
    device->texCreate(desc.target, desc.width, desc.height, 0, desc.format, desc.data_format, desc.flags.packed, source,
        &texProc, "AwesomeWotlk", &tex);
    return of(tex);
}

void CGxTexExt::destroy(CGxTexExt*& tex) {
    if (CGxTexExt* doomed = std::exchange(tex, nullptr)) {
        if (CGxDeviceD3dExt* device = CGxDeviceD3dExt::of()) { device->texDestroy(doomed); }
    }
}

void CGxTexExt::markDirty(std::initializer_list<Region> regions, bool upload) {
    Region box{.x0 = INT32_MAX, .y0 = INT32_MAX, .x1 = INT32_MIN, .y1 = INT32_MIN};
    const auto add = [&box](const Region& region) {
        if (region.x0 >= region.x1 || region.y0 >= region.y1) { return; }
        box = {
            .x0 = std::min(box.x0, region.x0),
            .y0 = std::min(box.y0, region.y0),
            .x1 = std::max(box.x1, region.x1),
            .y1 = std::max(box.y1, region.y1)
        };
    };
    std::ranges::for_each(regions, add);
    if (regions.size() != 0) {
        if (box.x0 >= box.x1) { return; }
        if (needs_update_ != 0) {
            add({.x0 = dirty_rect_.min_x, .y0 = dirty_rect_.min_y, .x1 = dirty_rect_.max_x, .y1 = dirty_rect_.max_y});
        }
    } else {
        box = {};  // an empty rect makes TexMarkForUpdate dirty the whole texture
    }
    CGxTex::markDirty{}(this, box.x0, box.y0, box.x1, box.y1, upload ? 1 : 0);
}

IDirect3DTexture9* CGxTexExt::realize() {
    CGxDeviceD3dExt* device = CGxDeviceD3dExt::of();
    if (device == nullptr || device->d3d() == nullptr) { return nullptr; }
    device->texMarkAsUpdated(this);
    return needs_create_ == 0 ? d3d_tex_ : nullptr;
}

void d3d::initialize(hookkit::HookTransaction& tx) { tx.attach(CGxDevice::iShaderLoad_hook{}); }
