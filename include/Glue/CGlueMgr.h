#pragma once

#include <hookkit/trampoline.h>

#include "Common/Common.h"

namespace glue {
HOOKKIT_CALLSITE_HOOK(loadGlueXML_call, 0x004DA9A7, CStatusLog::dtor_hook);
HOOKKIT_HOOK_HANDLE(loadCharacters_site, 0x004E47E5, hookkit::Conv::eCdecl, void);
}  // namespace glue
