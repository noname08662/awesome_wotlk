#pragma once

#include <cstdint>

#include "BaseTypes.h"

#include "FrameScript/CScriptRegion.h"
#include "FrameScript/FrameScript.h"
#include "UI/CBackdropGenerator.h"
#include "UI/UIEnums.h"
#include "Widget/CSimpleFont.h"
#include "Widget/CSimpleRender.h"

class CSimpleTop;
class CSimpleRegion;

class CSimpleFrame : public CScriptRegion {
public:
    ~CSimpleFrame() override;  // 0

    const char* getName() override;                                          // 1
    void* getMetaTable() override;                                           // 2
    ScriptIx* getScriptByName(const char* name, const char** out) override;  // 3
    bool isObjectType(int type) override;                                    // 4

    virtual void preLoadXML(XMLNode* node, int unused);  // 21
    virtual void getScriptTime(LuaState* l, double* out_total_time, uint32_t* out_call_count,
        int include_children);                                          // 22
    virtual void postLoadXML(XMLNode* node, CStatus* status);           // 23
    virtual void unregisterRegion(CSimpleRegion* region);               // 24
    virtual bool getBoundsRect(Rectf* out_rect);                        // 25
    virtual void resize1();                                             // 26
    virtual void resize2();                                             // 27
    virtual void onShow();                                              // 28
    virtual void onHide();                                              // 29
    virtual void onUpdate(float elapsed);                               // 30
    virtual int onInput(void* input_event);                             // 31
    virtual void onFrameRender();                                       // 32
    virtual void onFrameRenderLayer(CRenderBatch* batch, uint32_t layer);  // 33
    virtual void onScreenSizeChanged();                                 // 34
    virtual void onFrameSizeChanged(float width, float height);         // 35
    virtual void onLayerCursorEnter(int motion);                        // 36
    virtual void onLayerCursorExit(int motion, int clear_click_state);  // 37
    virtual int nullsub0();                                             // 38
    virtual int nullsub1();                                             // 39
    virtual int onChar(void* input_event);                              // 40
    virtual int onKeyDown(void* input_event);                           // 41
    virtual int onKeyUp(void* input_event);                             // 42
    virtual int onMouseDown(void* input_event, const char* button);     // 43
    virtual int onMouseUp(void* input_event, const char* button);       // 44
    virtual int onMouseWheel(void* input_event);                        // 45
    virtual int onDragStart(void* input_event);                         // 46
    virtual void onDragStop(void* input_event);                         // 47
    virtual void onReceiveDrag(void* input_event);                      // 48
    virtual void lockHighlight(CSimpleTop*);                            // 49
    virtual void hideThis();                                            // 50
    virtual void showThis();                                            // 51
    virtual void updateScale(int force);                                // 52
    virtual void updateRegions();                                       // 53
    virtual void updateDepth(int force);                                // 54
    virtual void parentFrame(CSimpleFrame* frame);                      // 55
    virtual void unparentFrame(CSimpleFrame* frame);                    // 56

    template <typename T>
    T* as() {
        return static_cast<T*>(this);
    }

    template <typename T>
    const T* as() const {
        return static_cast<const T*>(this);
    }

    // CLayoutFrame
    void loadXML(XMLNode* node, CStatus* status) override;  // 1
    float getWidth() override;                              // 10
    float getHeight() override;                             // 11
    int32_t stubReturnInt32One() override;                  // 17
    void onFrameSizeChanged(Rectf rect) override;           // 18

    struct SIMPLEFRAMENODE {
        TSLink<SIMPLEFRAMENODE> link;
        CSimpleFrame* frame;
    };

    static_assert(sizeof(SIMPLEFRAMENODE) == 0xC);

    struct FRAMEATTR {
        TSHashObject<FRAMEATTR> hash_obj;
        int lua_ref;
    };

    static_assert(sizeof(FRAMEATTR) == 0x1C);

    enum Scripts {
        eOnLoad = 0,
        eOnSizeChanged,
        eOnUpdate,
        eOnShow,
        eOnHide,
        eOnEnter,
        eOnLeave,
        eOnMouseDown,
        eOnMouseUp,
        eOnMouseWheel,
        eOnDragStart,
        eOnDragStop,
        eOnReceiveDrag,
        eOnChar,
        eOnKeyDown,
        eOnKeyUp,
        eOnAttributeChanged,
        eOnEnable,
        eOnDisable,
        eScriptsCount
    };

    enum InputEventMask : uint32_t {
        eEventMaskNone = 0x0,
        eEventMaskChar = 0x1,
        eEventMaskKey = 0x2,
        eEventMaskMouse = 0x4,
        eEventMaskMousewheel = 0x8,
    };

    CSimpleTop* simple_top_;
    CSimpleFrame* hover_frame_;
    CScriptRegion* title_region_;
    uint32_t _state_unk;
    uint32_t id_;
    FrameState state_flags_;
    float scale_;
    uint8_t alpha_self_;
    uint8_t alpha_parent_;
    uint8_t alpha_pending_;
    uint8_t flags_;
    unk_t _unk_C0;
    float depth_self_;
    float depth_parent_;
    uint32_t _unk_d3d;
    uint32_t strata_;
    uint32_t frame_level_;
    uint32_t registered_input_events_;
    uint32_t is_shown_;
    uint32_t is_drawn_;
    Rectf hit_rect_;
    Rectf hit_rect_insets_;
    Rectf clamp_rect_insets_;
    uint32_t lock_highlight_;
    uint32_t accepted_mouse_btn_mask_;
    uint32_t pushed_state_;
    uint32_t dragged_state_;
    uint32_t active_btn_mask_;
    float click_origin_x_;
    float click_origin_y_;
    uint32_t is_size_set_;
    ScriptIx scripts_[eScriptsCount];
    TSHashTable<FRAMEATTR> attributes_;
    uint32_t draw_layer_toggles_[4];
    uint32_t is_highlighted_;
    CBackdropGenerator* backdrop_;
    TSExplicitList<CSimpleTexture> vis_regions_;
    TSExplicitList<CSimpleTexture> draw_layers_[5];
    uint32_t dirty_layers_;
    CRenderBatch* render_batches_[5];
    TSExplicitList<CRenderBatch> batch_list_;
    TSExplicitList<SIMPLEFRAMENODE> children_;
    TSLink<CSimpleFrame> top_frames_link_;
    unk_t _unk;
    CSimpleFont* normal_font_;
    CSimpleFont* highlight_font_;
    CSimpleFont* disabled_font_;

    // depth/level, update children
    HOOKKIT_HOOK(setFrameDepth, 0x0048F5D0, hookkit::Conv::eThiscall, void, CSimpleFrame*, float, int);
    HOOKKIT_HOOK(setFrameLevel, 0x004910A0, hookkit::Conv::eThiscall, void, CSimpleFrame*, uint32_t, int);
    HOOKKIT_HOOK(setAlpha, 0x0048EA10, hookkit::Conv::eThiscall, void, CSimpleFrame*, uint8_t);
    HOOKKIT_HOOK(hide, 0x0048F620, hookkit::Conv::eThiscall, int, CSimpleFrame*);
    HOOKKIT_HOOK(show, 0x0048F660, hookkit::Conv::eThiscall, int, CSimpleFrame*);
    HOOKKIT_HOOK(setBeingScrolled, 0x00490F60, hookkit::Conv::eThiscall, void, CSimpleFrame*, int, int);
    HOOKKIT_HOOK(onFrameRenderLayerBase, 0x00490840, hookkit::Conv::eThiscall, void, CSimpleFrame*, CRenderBatch*,
        uint32_t);
};

static_assert(sizeof(CSimpleFrame) == 0x29C);
