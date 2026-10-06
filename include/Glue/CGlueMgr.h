#pragma once

#include <hookkit/hook.h>

namespace glue {
HOOKKIT_NAMED_HOOK(loadGlueXML_site, 0x004DA9A7, {"jmpback", 0x004DA9AC}, {"resumeStatusDtor", 0x004DA2D0});
HOOKKIT_HOOK_HANDLE(loadCharacters_site, 0x004E47E5, hookkit::Conv::eCdecl, void);
}  // namespace glue
