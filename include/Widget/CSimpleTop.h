#pragma once

#include <hookkit/hook.h>

#include "BaseTypes.h"

#include "Lib/Storm.h"
#include "UI/CFrameStrata.h"
#include "UI/CLayoutFrame.h"

struct CFrameStrata;
class CSimpleFrame;

class CSimpleTop : public CLayoutFrame {
public:
    enum StrataLayers {
        eUnknown,
        eBackground,
        eLow,
        eMedium,
        eHigh,
        eDialog,
        eFullscreen,
        eFullscreenDialog,
        eTooltip,
        eStrataCount
    };

    ~CSimpleTop() override;  // 0

    unk_t _pad;
    CSimpleFrame* mouseover_;
    unk_t _ukn[788];
    TSExplicitList<CSimpleFrame> frames_;
    unk_t _ukn_1[3];
    CFrameStrata* strata_layers_[eStrataCount];
    unk_t _ukn_2[327];
    float mouse_x_ndc_;
    float mouse_y_ndc_;
    unk_t _ukn_3[2];
    char* mouse_held_name_;
    unk_t _ukn_4[5];
    int protected_actions_allowed_;
    unk_t _ukn3;
    void (*on_taint_error_)(int);
    unk_t _ukn4;

    HOOKKIT_HOOK(onLayerRender, 0x00495410, hookkit::Conv::eThiscall, void, CSimpleTop*);

    static CSimpleTop* get() { return *reinterpret_cast<CSimpleTop**>(0x00B499A8); }

    void markAllBatchesDirty() {
        for (CFrameStrata* strata : strata_layers_) {
            if (strata == nullptr) { continue; }
            strata->batch_dirty = 1;
            for (uint32_t i = 0; i < strata->top_level && i < strata->nodes.count; ++i) {
                if (CFrameStrataNode* node = strata->nodes[i]) { node->batch_dirty |= 0x1F; }
            }
        }
        for (CSimpleFrame* frame = frames_.head(); frame != nullptr; frame = frames_.next(frame)) {
            if ((frame->state_flags_ & eFlagBeingScrolled) != 0) { frame->dirty_layers_ |= 0x1F; }
        }
    }

    void refreshTextureQuads() {
        for (CSimpleFrame* frame = this->frames_.head(); frame != nullptr; frame = this->frames_.next(frame)) {
            for (auto& layer : frame->draw_layers_) {
                for (CSimpleTexture* region : layer) {
                    if ((region->flag_ & CSimpleRegion::eFlagTransformDirty) != 0) { continue; }
                    region->updateGeometry();
                }
            }
        }
        this->markAllBatchesDirty();
    }
};
