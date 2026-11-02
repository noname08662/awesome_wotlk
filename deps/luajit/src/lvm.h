/*
** $Id: lvm.h,v 2.5.1.1 2007/12/27 13:02:25 roberto Exp $
** Lua virtual machine
** See Copyright Notice in lua.h
*/

#ifndef lvm_h
#define lvm_h


#include "ldo.h"
#include "lobject.h"
#include "ltm.h"


#define tostring(L,o) ((ttype(o) == LUA_TSTRING) || (luaV_tostring(L, o)))

#define tonumber(o,n)	(ttype(o) == LUA_TNUMBER || \
                         (((o) = luaV_tonumber(o,n)) != NULL))

#define equalobj(L,o1,o2) \
	(ttype(o1) == ttype(o2) && luaV_equalval(L, o1, o2))


LUAI_FUNC int luaV_lessthan (lua_State *L, const TValue *l, const TValue *r);
LUAI_FUNC int luaV_equalval (lua_State *L, const TValue *t1, const TValue *t2);
LUAI_FUNC const TValue *luaV_tonumber (const TValue *obj, TValue *n);
LUAI_FUNC int luaV_tostring (lua_State *L, StkId obj);
LUAI_FUNC void luaV_gettable (lua_State *L, const TValue *t, TValue *key,
                                            StkId val);
LUAI_FUNC void luaV_settable (lua_State *L, const TValue *t, TValue *key,
                                            StkId val);
LUAI_FUNC void luaV_execute (lua_State *L, int nexeccalls);
/* the JIT's copies of the interpreter's cases (lvm.c) */
LUAI_FUNC int luaV_lessequal (lua_State *L, const TValue *l, const TValue *r);
LUAI_FUNC void luaV_arith (lua_State *L, StkId ra, const TValue *rb, const TValue *rc, TMS op);
LUAI_FUNC void luaV_len (lua_State *L, StkId ra, const TValue *rb);
LUAI_FUNC void luaV_concatop (lua_State *L, int a, int b, int c);
LUAI_FUNC void luaV_forprep (lua_State *L, StkId ra);
LUAI_FUNC int luaV_tforloop (lua_State *L, int a, int nresults);
LUAI_FUNC void luaV_setlist (lua_State *L, int a, int n, int c);
LUAI_FUNC void luaV_vararg (lua_State *L, int a, int b);
LUAI_FUNC void *luaV_tailcall (lua_State *L, StkId ra);
LUAI_FUNC void luaV_execute_profile (lua_State *L, int nexeccalls);
LUAI_FUNC void luaV_concat (lua_State *L, int total, int last);

#endif
