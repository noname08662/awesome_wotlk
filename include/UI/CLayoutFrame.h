#pragma once

#include <cstdint>

#include "Math/Primitives.h"
#include "XML/XMLNode.h"

class CLayoutFrame {
public:
    struct CFramePoint {
        Vec2f pos;
        CLayoutFrame* layout;
        uint32_t flags;
    };

    static_assert(sizeof(CFramePoint) == 0x10);

    enum LayoutFlags : uint32_t {
        RETRY_COUNT_MASK = 0xFF,
        FLAG_RESIZE_PENDING = 0x2,
        FLAG_DEFERRED_RESIZE = 0x4,
        FLAG_VALID_RECT = 0x100,
        FLAG_BATCH_RESIZE = 0x200,
        FLAG_RESIZING = 0x400,
        FLAG_RESIZE_FAILED = 0x800,
        FLAG_CLAMP_TO_SCREEN = 0x1000,
        FLAG_PROTECTED_EXPLICIT = 0x10000,
        FLAG_PROTECTED_BY_CHILD = 0x20000,
        FLAG_PROTECTED_DIRTY = 0x40000,
        FLAG_PROTECTED_MASK = 0x70000,
        FLAG_PROTECT_UPDATING = 0x80000,
        REINIT_MASK = 0xFF0000FF,
    };

    enum AnchorPoint : uint32_t {
        eAnchorTopleft = 0x0,
        eAnchorTop = 0x1,
        eAnchorTopright = 0x2,
        eAnchorLeft = 0x3,
        eAnchorCenter = 0x4,
        eAnchorRight = 0x5,
        eAnchorBottomleft = 0x6,
        eAnchorBottom = 0x7,
        eAnchorBottomright = 0x8,
    };

    enum EvalMask : uint32_t {
        eEvalLeft = 0x1,
        eEvalTop = 0x2,
        eEvalRight = 0x4,
        eEvalBottom = 0x8,
        eEvalCenterx = 0x10,
        eEvalCentery = 0x20,
    };

    enum HitboxAnchor : uint32_t {
        eHbTop = 0,
        eHbCenter = 1,
        eHbBottom = 2,
    };

    struct FRAMENODE {
        TSLink<FRAMENODE> link;
        CLayoutFrame* target;
        AnchorPoint point_mask;
    };

    static_assert(sizeof(FRAMENODE) == 0x10);

    virtual ~CLayoutFrame();                                                                // 0
    virtual void loadXML(XMLNode*, CStatus*);                                               // 1
    virtual CLayoutFrame* getLayoutParent();                                                // 2
    virtual LayoutFlags propagateProtectedFlag(int flag_val);                               // 3
    virtual int areChildrenProtected(bool* any_updating);                                   // 4
    virtual bool setLayoutScale(float scale, char force_upd);                               // 5
    virtual bool setLayoutDepth(float depth, char force_upd);                               // 6
    virtual void setWidth(float width);                                                     // 7
    virtual void setHeight(float height);                                                   // 8
    virtual void setSize(float width, float height);                                        // 9
    virtual float getWidth();                                                               // 10
    virtual float getHeight();                                                              // 11
    virtual void getSize(float* width_out, float* height_out, bool check_resize);           // 12
    virtual void getClampRectInsets(float* left, float* right, float* top, float* bottom);  // 13
    virtual int stubReturnZero();                                                           // 14
    virtual bool canBeAnchorFor(CLayoutFrame* other);                                       // 15
    virtual CLayoutFrame* resolveRelativeFrame(const char* src);                            // 16
    virtual int32_t stubReturnInt32One();                                                   // 17
    virtual void onFrameSizeChanged(Rectf rect);                                            // 18

    TSLink<CLayoutFrame> link_resize_;
    CFramePoint* frame_points_[9];
    TSExplicitList<FRAMENODE>* frame_node_list_;
    TSLink<CLayoutFrame> link_protected_;
    EvalMask eval_mask_;
    LayoutFlags flags_;
    Rectf rect_;
    Vec2f dims_;
    float scale_;
    float depth_;
    Rectf min_resize_rect_;

    // point, relative_to, relative_point, x, y, resize
    HOOKKIT_HOOK(setPoint, 0x0048A260, hookkit::Conv::eThiscall, void, CLayoutFrame*, AnchorPoint, CLayoutFrame*,
        AnchorPoint, float, float, bool);
    HOOKKIT_HOOK(isAtTargetPos, 0x00489270, hookkit::Conv::eThiscall, bool, CLayoutFrame*, Vec3f*);

    HOOKKIT_HOOK_HANDLE(resizePending, 0x004898B0, hookkit::Conv::eCdecl, void);

    bool isAtTargetPosPerc(const Vec3f* pos, Vec2f percs, HitboxAnchor anchor) const {
        const Vec2f size = this->dims_ * this->scale_;
        float y_min, y_max;
        switch (anchor) {
            case eHbTop:
                y_max = this->rect_.top;
                y_min = this->rect_.top - size.height * percs.y;
                break;
            case eHbBottom:
                y_min = this->rect_.bottom;
                y_max = this->rect_.bottom + size.height * percs.y;
                break;
            case eHbCenter:
            default:
                const float mid_y = (this->rect_.bottom + this->rect_.top) * 0.5f;
                const float half_height = size.height * percs.y * 0.5f;
                y_min = mid_y - half_height;
                y_max = mid_y + half_height;
                break;
        }
        const float mid_x = (this->rect_.left + this->rect_.right) * 0.5f;
        const float half_width = (size.width * 0.5f) * percs.x;
        return (pos->x >= (mid_x - half_width) && pos->x <= (mid_x + half_width)) &&
            (pos->y >= y_min && pos->y <= y_max);
    }
};
