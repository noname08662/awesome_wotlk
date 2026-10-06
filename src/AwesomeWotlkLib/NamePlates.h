#pragma once

#include <hookkit/transaction.h>

#include "include/BaseTypes.h"

namespace name_plates {
guid_t getTokenGuid(int id);
int getTokenId(guid_t guid);
void initialize(hookkit::HookTransaction& tx);
}  // namespace name_plates
