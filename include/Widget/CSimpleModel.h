#pragma once

#include <hookkit/hook.h>

#include <cstdint>

#include "BaseTypes.h"

#include "Widget/CSimpleFrame.h"

class CM2Model;
class CM2Scene;
class CCamera;

class CSimpleModel : public CSimpleFrame {
public:
    CM2Scene* scene_;
    CM2Model* model_;
    CCamera* camera_;
    int32_t pending_camera_index_;
    int32_t pending_camera_id_;
    unk_t _unk2B0[(0x368 - 0x2B0) / 4];

    HOOKKIT_HOOK_HANDLE(renderModel, 0x0095FC30, hookkit::Conv::eCdecl, void, CSimpleModel*);
};

static_assert(sizeof(CSimpleModel) == 0x368);
