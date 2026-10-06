#pragma once

#include "Widget/CSimpleRender.h"

struct CFrameStrataNode {
    TSExplicitList<CSimpleFrame> pending_frame_list;
    TSExplicitList<CSimpleFrame> all_frame_list;
    CSimpleFrame* pending_frame;
    CRenderBatch nodes[5];
    uint32_t batch_dirty;
    TSExplicitList<CRenderBatch> nodes_list;

    HOOKKIT_HOOK(updateBatches, 0x00494AF0, hookkit::Conv::eThiscall, uint32_t, CFrameStrataNode*);
};

static_assert(sizeof(CFrameStrataNode) == 0x11C);

struct CFrameStrata {
    int32_t batch_dirty;
    int32_t nodes_dirty;
    uint32_t top_level;
    TSBaseArray<CFrameStrataNode*> nodes;

    HOOKKIT_HOOK(buildBatches, 0x00494EE0, hookkit::Conv::eThiscall, int32_t, CFrameStrata*, int32_t);
    HOOKKIT_HOOK(renderBatch, 0x00494F30, hookkit::Conv::eThiscall, void, CFrameStrata*);
};

static_assert(sizeof(CFrameStrata) == 0x18);
