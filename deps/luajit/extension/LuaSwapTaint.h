#pragma once

#define lua_tainted (*(const char**)0x00D4139C)
#define lua_taintexpected (*(int*)0x00D413A0)
#define lua_taintedclosure (*(int*)0x00D413A4)
#define lua_closuretaint (*(const char**)0x00D413A8)
#define lua_gctag (*(const char**)0x00D413AC)
#define lua_tainteventcallback (*(lua_TaintEventCallback*)0x00D413B0)
