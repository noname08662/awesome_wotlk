#pragma once

#include <d3d9.h>
#include <hookkit/transaction.h>

#include <cassert>
#include <cstdint>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "include/Graphics/CGxDevice.h"
#include "include/Graphics/CGxDeviceD3d.h"
#include "include/Graphics/CGxShader.h"
#include "include/Graphics/CGxTex.h"

namespace d3d::bls {
// path\<profile>\name.bls, from its record-th record on
struct Bls {
    std::string path;  // e.g. "Shaders\\Pixel"
    std::string name;
    uint32_t record = 0;
};

bool load(
    const Bls& bls, GxShaderType type, const CGxDevice* device, std::span<CGxShader*> shaders, uint32_t first = 0);
}  // namespace d3d::bls

class CGxDeviceD3dExt : public CGxDeviceD3d {
public:
    [[nodiscard]]
    static CGxDeviceD3dExt* of(CGxDevice* device = CGxDevice::getDevice()) {
        return reinterpret_cast<CGxDeviceD3dExt*>(CGxDeviceD3d::of(device));
    }

    [[nodiscard]]
    IDirect3DDevice9* d3d() const {
        return has_context_ != 0 ? d3d_device_ : nullptr;
    }

    void setConstants(GxShaderType type, uint32_t reg, std::span<const float> data) {
        assert(data.size() % 4 == 0 && "CGxDeviceD3dExt::setConstants: partial float4 register");
        shaderConstantsSet(type, reg, data.data(), data.size() / 4);
    }

    void dsSet(GxD3dDsState state, uint32_t value) {
        if (d3d_device_ != nullptr) { CGxDeviceD3d::dsSet(state, value); }
    }
};

static_assert(sizeof(CGxDeviceD3dExt) == sizeof(CGxDeviceD3d));

class CGxShaderExt : public CGxShader {
public:
    static constexpr auto kPath = "AwesomeWotlk";
    static constexpr std::string_view kPrefix = "AwesomeWotlk_";

    enum class Naming : uint8_t {
        eOwned,
        eOverride,
    };

    class Name {
    public:
        explicit constexpr Name(std::string_view base) : base_(base), naming_(Naming::eOwned) {}

        static constexpr Name engine(std::string_view engine_name) { return {engine_name, Naming::eOverride}; }

        [[nodiscard]]
        std::string str() const {
            return naming_ == Naming::eOwned ? std::string(kPrefix).append(base_) : std::string(base_);
        }

        [[nodiscard]]
        constexpr Naming naming() const {
            return naming_;
        }

    private:
        constexpr Name(std::string_view base, Naming naming) : base_(base), naming_(naming) {}

        std::string_view base_;
        Naming naming_;
    };

    struct Define {
        std::string name;
        std::string value;
    };

    struct Source {
        std::string hlsl;
        std::vector<std::string> entries = {"main"};
        std::function<std::vector<Define>()> defines;
        std::optional<d3d::bls::Bls> fallback;
        std::function<bool()> use_fallback;
        std::optional<d3d::bls::Bls> meta_from;
    };

    [[nodiscard]]
    static CGxShaderExt* of(CGxShader* shader) {
        return reinterpret_cast<CGxShaderExt*>(shader);
    }

    static void define(GxShaderType type, const Name& name, Source&& source);
    static bool acquire(GxShaderType type, const Name& name, std::span<CGxShader*> out);
    [[nodiscard]]
    static CGxShaderExt* acquire(GxShaderType type, const Name& name);

    struct Binding {
        GxShaderType type{};
        Name name;
        std::span<CGxShader*> slots;  // filled with slots.size() mutations of name
    };

    // all-or-nothing: every binding is acquired (and accepted) before any slot changes, the old refs are destroyed
    using Accept = std::function<bool(std::span<CGxShader* const> acquired)>;
    static bool replace(std::initializer_list<Binding> bindings, const Accept& accept = {});
    static bool replace(GxShaderType type, std::span<CGxShader*> slots, const Name& name, const Accept& accept = {});

    [[nodiscard]]
    static std::vector<CGxShaderExt*> loaded(
        GxShaderType type, const std::function<bool(const CGxShaderExt*)>& filter = {});
    static size_t reloadAll();
    bool reload();

    // also false while the device has no context
    [[nodiscard]]
    bool usable() {
        return byte_code_.count != 0 && this->isValid() != 0;
    }

    void clearMeta() {
        this->sampler_types_ = 0;
        this->constant_count_ = 0;
        this->active_sampler_mask_ = 0;
        this->param_mask_ = 0;
    }

    void setBytecode(const std::vector<uint8_t>& bytecode) {
        if (!this->byte_code_.setCount(bytecode.size())) { return; }  // keeps the record's bytecode
        std::memcpy(this->byte_code_.data, bytecode.data(), bytecode.size());
        this->is_created_ = 0;
        this->_unk[0] = 0;
    }
};

static_assert(sizeof(CGxShaderExt) == sizeof(CGxShader));

class CGxTexExt : public CGxTex {
public:
    struct Desc {
        int32_t width = 0;
        int32_t height = 0;
        GxTexFormat format = eGxTexArgb8888;
        GxTexFormat data_format = eGxTexArgb8888;  // format of the pixels Source::pixels returns
        GxTexFlags flags{};                        // is_render_target: default pool, recreated on every reset
        GxTexTarget target = eGxTexTarget2d;
    };

    class Source {
    public:
        Source() = default;
        virtual ~Source() = default;

        // face, mip, pitch
        virtual const void* pixels(int32_t, int32_t, uint32_t&) { return nullptr; }

        // the d3d texture was dropped by a device reset; recreated and fully re-requested on next use
        virtual void onRecreate() {}

        Source(const Source&) = delete;
        Source& operator=(const Source&) = delete;
        Source(Source&&) = delete;
        Source& operator=(Source&&) = delete;
    };

    struct Region {
        int32_t x0 = 0;
        int32_t y0 = 0;
        int32_t x1 = 0;
        int32_t y1 = 0;
    };

    [[nodiscard]]
    static CGxTexExt* of(CGxTex* tex) {
        return reinterpret_cast<CGxTexExt*>(tex);
    }

    // a null source uploads nothing
    [[nodiscard]]
    static CGxTexExt* create(const Desc& desc, Source* source);
    static void destroy(CGxTexExt*& tex);
    void markDirty(std::initializer_list<Region> regions = {}, bool upload = true);

    [[nodiscard]]
    IDirect3DTexture9* realize();
};

static_assert(sizeof(CGxTexExt) == sizeof(CGxTex));

namespace d3d {
void initialize(hookkit::HookTransaction& tx);
}  // namespace d3d
