#pragma once

#include <hookkit/hook.h>

#include <cstddef>
#include <cstdint>

#include "Math/Primitives.h"
#include "Widget/CSimpleFrame.h"

struct CRenderBatch;

class CCooldown : public CSimpleFrame {
public:
    uint32_t start_time_;      
    uint32_t duration_;        
    uint32_t elapsed_time_;    
    uint32_t flag_flash_;        
    uint32_t reverse_;           
    CTexture* texture_;        
    Vec3f swipe_pos_[10];       
    Vec2f swipe_uv_[10];        
    uint32_t swipe_color_;    
    Vec3f quad_pos_[4];         
    Vec2f quad_uv_[4];          
    uint32_t quad_color_;     
    CTexture* edge_texture_;    
    uint32_t draw_edge_;      
    Vec3f edge_pos_[4];         
    Vec2f edge_uv_[4];          
    uint32_t edge_color_;     

    HOOKKIT_HOOK(onFrameRenderLayer, 0x005EBD20, hookkit::Conv::eThiscall, void, CCooldown*, CRenderBatch*, uint32_t);
    HOOKKIT_HOOK(onLayoutFrameSizeChanged, 0x005EC8D0, hookkit::Conv::eThiscall, int, CLayoutFrame*, const Rectf*);
    HOOKKIT_HOOK(resetVerts, 0x005EBF20, hookkit::Conv::eThiscall, int, CCooldown*);
    HOOKKIT_HOOK(animateVerts, 0x005EC1E0, hookkit::Conv::eThiscall, void, CCooldown*);
};

static_assert(sizeof(CCooldown) == 0x430);
