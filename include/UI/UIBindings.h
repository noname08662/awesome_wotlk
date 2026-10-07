#pragma once

#include "Lib/Lua.h"
#include "Lib/Storm.h"
#include "Widget/CSimpleRegion.h"

class CGUIBindings {
public:
    enum ModifierKeyMask {
        eModifierMaskLshift = 0x1,
        eModifierMaskRshift = 0x2,
        eModifierMaskLctrl = 0x4,
        eModifierMaskRctrl = 0x8,
        eModifierMaskLalt = 0x10,
        eModifierMaskRalt = 0x20,
        eModifierMaskShift = 0x3,
        eModifierMaskCtrl = 0xC,
        eModifierMaskAlt = 0x30,
        eModifierMaskAny = 0x3F,
    };

    enum BindingSet : uint32_t {
        eBindingSetDefault = 0,
        eBindingSetAccount = 1,
        eBindingSetCharacter = 2,
        eBindingSetActive = 3,
        eBindingSetResolve = 4
    };

    struct KEYCOMMAND {
        TSHashObject<KEYCOMMAND> hash_obj;
        int32_t index;
        int32_t lua_ref;
        int32_t run_on_up;
        int32_t pressure;
        int32_t angle;
    };

    struct KEYBINDING {
        struct KeyBindingSlot {
            int index;
            RCString command;
        };

        TSHashObject<KEYBINDING> hash_obj;
        uint32_t account_or_global;
        KeyBindingSlot bindings[4];  // BindingSet
    };

    struct OVERRIDEKEYBINDING {
        struct OVERRIDEKEYBINDINGENTRY {
            TSLink<OVERRIDEKEYBINDINGENTRY> link;
            CSimpleRegion* owner;
            KEYBINDING binding;
        };

        TSHashObject<OVERRIDEKEYBINDING> hash_obj;
        TSExplicitList<OVERRIDEKEYBINDINGENTRY> normal_prio_list;
        TSExplicitList<OVERRIDEKEYBINDINGENTRY> high_prio_list;
        TSExplicitList<OVERRIDEKEYBINDINGENTRY> free_list;
    };

    struct MODIFIEDCLICK {
        struct ModifiedClickSlot {
            uint32_t flags;  // bit 0 = is default
            ModifierKeyMask modifier_mask;
            const char* button_name;
        };

        TSHashObject<MODIFIEDCLICK> hash_obj;
        ModifiedClickSlot slots[4];
    };

    uint32_t visible_binding_count_;
    uint32_t hidden_binding_count_;
    uint32_t modified_click_count_;
    TSHashTable<KEYCOMMAND> keycmd_tsht_;
    TSHashTable<KEYBINDING> keybind_tsht_[4];
    TSHashTable<OVERRIDEKEYBINDING> override_bindings_entry_tsht_;
    TSHashTable<MODIFIEDCLICK> modclick_tsht_;
    uint32_t current_binding_set_;  // 1 = account, 2 = character
    uint32_t binding_set_loaded_;
    uint32_t override_modifiers_active_;
    uint32_t override_modifier_mask_;
    uint32_t binding_mode_mask_;
    uint32_t binding_mode_override_mask_;

    static CGUIBindings* get() { return *reinterpret_cast<CGUIBindings**>(0x00BEADD8); }

    bool bindgingExists(const char* name) {
        if (name && name[0]) {
            if (keycmd_tsht_.get(storm::str::hashUtf8{}(name)) != nullptr) { return true; }
        }

        return false;
    }

    bool headerExists(const char* header) {
        if (header && header[0]) {
            std::string header_key = "HEADER_" + std::string(header);
            if (keycmd_tsht_.get(storm::str::hashUtf8{}(header_key.c_str())) != nullptr) { return true; }
        }

        return false;
    }

    void registerBinding(const char* binds_set, const char* binding_name, const char* binding_text,
        const char* binding_header_name, const char* binding_header_text, const char* lua_script) {
        LuaState* l = lua::getLuaState();
        if (!l) { return; }

        std::string upper_name = binding_name ? binding_name : "";
        std::ranges::transform(upper_name, upper_name.begin(), [](unsigned char c) { return std::toupper(c); });

        if (bindgingExists(upper_name.c_str())) { return; }

        std::string upper_header = binding_header_name ? binding_header_name : "";
        std::ranges::transform(upper_header, upper_header.begin(), [](unsigned char c) { return std::toupper(c); });

        if (headerExists(upper_header.c_str())) { return; }

        if (!upper_header.empty()) {
            lua::pushString(l, binding_header_text ? binding_header_text : "");
            std::string key = "BINDING_HEADER_" + upper_header;
            lua::setField(l, lua::kGlobalsindex, key.c_str());
        }

        if (!upper_name.empty()) {
            lua::pushString(l, binding_text ? binding_text : "");
            std::string key = "BINDING_NAME_" + upper_name;
            lua::setField(l, lua::kGlobalsindex, key.c_str());
        }

        XMLNode node;
        node.ctor(0, "Binding");
        node.setValue("name", upper_name.c_str());
        node.setValue("header", binding_header_name ? binding_header_name : "");
        node.body_text_ = const_cast<char*>(lua_script ? lua_script : "");

        loadBinding(binds_set, &node, CStatus::get());
    }

    // src, node, status
    HOOKKIT_HOOK(
        loadBinding, 0x00564470, hookkit::Conv::eThiscall, KEYBINDING*, CGUIBindings*, const char*, XMLNode*, CStatus*);
    // mode index, enabled
    HOOKKIT_HOOK(setBindingModeState, 0x0055E550, hookkit::Conv::eThiscall, void, CGUIBindings*, int, int);
};
