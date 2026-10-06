#pragma once

#include <hookkit/hook.h>

namespace net_client {
// login, passwd
HOOKKIT_HOOK_HANDLE(login, 0x004D8A30, hookkit::Conv::eCdecl, void, const char*, const char*);
}  // namespace net_client
