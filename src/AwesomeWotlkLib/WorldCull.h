#pragma once

#include <hookkit/transaction.h>

#include <cstdint>

#include "Math/Primitives.h"

class CMapEntity;

namespace world_cull {
inline void assertEnginePrecision() {
    uint16_t cw = 0;
    __asm { fnstcw cw }
    assert(cw == 0x027F && "world_cull: the x87 control word left 0x027F, the cores no longer match the engine");
    assert((_mm_getcsr() & 0xFFC0) == 0x1F80 && "world_cull: MXCSR left round to nearest with every exception masked");
    static_cast<void>(cw);
}

void initialize(hookkit::HookTransaction& tx);
}  // namespace world_cull

namespace world_cull::views {
enum class Side : uint8_t {
    eFront,
    eBack,
    eLeft,
    eRight,
    eUp,
    eDown,
};

struct Spec {
    Side side = Side::eFront;
    Rectf rect = Rectf::ndc();
    bool horizon = true;
    bool screen = false;
    bool (*wants)(CMapEntity* entity) = nullptr;
    bool animates = false;
};

using Id = uint32_t;

// 0 for an empty window, an unknown side or no wants
Id open(const Spec& spec);
void close(Id id);
bool reached(Id id, const CMapEntity* entity);

void apply(bool on);
void initialize();
}  // namespace world_cull::views
