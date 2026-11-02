/*
** Interface to JIT engine.
** Copyright (C) 2005-2012 Mike Pall. See Copyright Notice in luajit.h
*/

#define ljit_core_c
#define LUA_CORE

#include <stdlib.h>
#include <string.h>

#include "lua.h"

#include "lobject.h"
#include "lstate.h"
#include "ldo.h"
#include "lstring.h"
#include "ltable.h"
#include "ldebug.h"
#include "lopcodes.h"

#include "ljit.h"
#include "ljit_hints.h"
#include "luajit.h"

const char luajit_ident[] =
  "$LuaJIT: " LUAJIT_VERSION " " LUAJIT_COPYRIGHT " " LUAJIT_URL " $\n";

/* ------------------------------------------------------------------------ */

/* Initialize JIT engine state. */
/* the JIT's own state lives in the CRT heap, so the Lua heap (the GC's pacing, the AddOn memory numbers)
** looks the same with the JIT on or off. With scriptProfile on, the backend never starts, the callgates stay
** NULL, and every function runs in the interpreter. */
void luaJIT_initstate(lua_State *L)
{
  jit_State *J = (jit_State *)calloc(1, sizeof(jit_State));
  if (J == NULL) luaD_throw(L, LUA_ERRMEM);
  G(L)->jit_state = J;
  G(L)->jit_gateLJ = NULL;
  G(L)->jit_gateJL = NULL;
  G(L)->jit_gateJC = NULL;
  J->frontwrap = NULL;
  J->flags = JIT_F_INIT_FAILED;
  if (!G(L)->profile && luaJIT_initbackend(L) == JIT_S_OK)
    J->flags = (J->flags & ~JIT_F_INIT_FAILED) | JIT_F_ON;
  J->L = NULL;  /* No compiler thread allocated, yet. */
}

/* Free JIT engine state. */
void luaJIT_freestate(lua_State *L)
{
  jit_State *J = G(L)->jit_state;
  if (J == NULL) return;
  if (J->jsub != NULL) luaJIT_freebackend(L);
  free(J);
  G(L)->jit_state = NULL;
}

/* ------------------------------------------------------------------------ */

/* Find relative PC (0 based) for a bytecode pointer or a JIT mcode address. */
int luaJIT_findpc(Proto *pt, const Instruction *savedpc)
{
  ptrdiff_t pcdiff = savedpc - pt->code;
  if (pcdiff >= 0 && pcdiff <= pt->sizecode) { /* Bytecode pointer? */
    return (int)pcdiff-1;
  } else {  /* Else translate JIT mcode address to PC. */
    char *addr = (char *)savedpc;
    jit_MCTrailer tr;
    tr.mcode = (char *)pt->jit_mcode;
    tr.sz = pt->jit_szmcode;
    /* Follow trailer chain until addr is part of an mcode block. */
    while (!((size_t)(addr - tr.mcode) < tr.sz)) {
      memcpy((void *)&tr, JIT_MCTRAILER(tr.mcode, tr.sz),
	     sizeof(jit_MCTrailer));
      if (tr.mcode == NULL) return -1;  /* Not in chain. */
    }
    {
      jit_Mfm *mfm = JIT_MCMFM(tr.mcode, tr.sz);
      int ofs = (int)(addr - tr.mcode);
      int isdeopt = !jit_mfm_ismain(mfm);
      int pc = 0;  /* Prologue fragment is at start of main mfm. */
      while (pc <= pt->sizecode) {
	int m = *mfm--;
	switch (m) {
	default:
	  if (m & JIT_MFM_MARK) {
	    m ^= JIT_MFM_MARK;
	    if (isdeopt) { pc = m; continue; }  /* Seek. */
	  }
	  ofs -= m;
	  if (ofs <= 0) return pc-1;  /* Found! */
	case JIT_MFM_COMBINE:
	case JIT_MFM_DEAD:
	  pc++;
	  break;
	case JIT_MFM_STOP:
	  jit_assert(0);  /* Premature STOP found. */
	  return -1;
	}
      }
      jit_assert(0);  /* Address is in .tail. */
      return -1;
    }
  }
}

/* Lookup mcode address for PC (1 based) in mfm. */
static void *jit_mfmlookup(jit_Mfm *mfm, char *addr, int mpc)
{
  int isdeopt = !jit_mfm_ismain(mfm);
  int pc = 0;  /* Prologue fragment is at start of main mfm. */
  while (pc != mpc) {
    int m = *mfm--;
    switch (m) {
    default:
      if (m & JIT_MFM_MARK) {
	m ^= JIT_MFM_MARK;
	if (isdeopt) { pc = m; continue; }  /* Seek. */
      }
      addr += m;
    case JIT_MFM_COMBINE:
    case JIT_MFM_DEAD:
      pc++;
      break;
    case JIT_MFM_STOP:
      return NULL;
    }
  }
  return (void *)addr;
}

/* Find mcode address for PC (1 based). */
void *luaJIT_findmcode(Proto *pt, int pc)
{
  void *addr = NULL;
  jit_Mfm *mfm;
  jit_MCTrailer tr;
  tr.mcode = (char *)pt->jit_mcode;
  tr.sz = pt->jit_szmcode;
  mfm = JIT_MCMFM(tr.mcode, tr.sz);
  jit_assert(pc >= 1 && pc <= pt->sizecode);
  while (mfm[-pc] == JIT_MFM_COMBINE) pc--;
  while (mfm[-pc] == JIT_MFM_DEAD) pc++;
  jit_assert(pc >= 1 && mfm[-pc] < (JIT_MFM_MARK|JIT_MFM_MAX)); /* Valid? */
  if (jit_mfm_isdeoptpc(mfm, pc)) {  /* Deoptimized instruction. */
    do {  /* Search through deopt mfm chain. */
      memcpy((void *)&tr, JIT_MCTRAILER(tr.mcode, tr.sz),
	     sizeof(jit_MCTrailer));
      if (tr.mcode == NULL) break;  /* Deopt ins missing in chain. */
      mfm = JIT_MCMFM(tr.mcode, tr.sz);
      if (jit_mfm_ismain(mfm)) break;  /* Old main mfm stops search, too. */
      addr = jit_mfmlookup(mfm, tr.mcode, pc);
    } while (addr == NULL);
  } else { /* Not deoptimized. Lookup in main mfm. */
    addr = jit_mfmlookup(mfm, tr.mcode, pc);
  }
  jit_assert(addr != NULL);  /* Corrupt mfm chain. */
  return addr;
}

/* ------------------------------------------------------------------------ */

/* Compile a prototype. */
/* straight into the backend (no frontend, state table or hints), which allocates nothing in the Lua heap
** and runs no Lua code, so nothing is stamped with the running code's taint or charged to its AddOn. */
static int jit_compile(lua_State *L, StkId func)
{
  jit_State *J = G(L)->jit_state;
  Closure *cl = clvalue(func);
  Proto *pt = cl->l.p;
  int status;

  /* Check if JIT engine is enabled and prevent recursive invocation. */
  if ((J->flags & JIT_F_INIT_FAILED) || !(J->flags & JIT_F_ON)) {
    status = JIT_S_ENGINE_OFF;
  } else if (J->flags & JIT_F_COMPILING) {
    status = JIT_S_DELAYED;
  } else if (pt->jit_szmcode != 0) {  /* Prevent recompile. */
    status = JIT_S_OK;
  } else {
    J->flags |= JIT_F_COMPILING;
    status = luaJIT_compileproto(L, pt);
    J->flags &= ~JIT_F_COMPILING;
  }

  /* Better sanity check what the frontend returns. */
  if ((status == JIT_S_OK && pt->jit_szmcode == 0) || status == JIT_S_NONE)
    status = JIT_S_COMPILER_ERROR;

  /* Update closure callgate and prototype status. */
  cl->l.jit_gate = (status == JIT_S_OK) ? (lua_CFunction)pt->jit_mcode :
					  G(L)->jit_gateJL;
  pt->jit_status = status;
  return status;
}

/* ------------------------------------------------------------------------ */

/* Compile and run a function. To be used by luaD_precall() only. */
int luaJIT_run(lua_State *L, StkId func, int nresults)
{
  ptrdiff_t funcr = savestack(L, func);
  int status = jit_compile(L, func);  /* Compile function. */
  func = restorestack(L, funcr);

  /* Run the compiled function on success. Fallback to bytecode on failure. */
  if (status == JIT_S_OK)
    return G(L)->jit_gateLJ(L, func, nresults);
  else
    return luaD_precall(L, func, nresults);
  /* Note: We are called from luaD_precall and we call it again. */
  /* So jit_compile makes sure pt->jit_status != JIT_S_NONE. */
}

/* for luaV_tailcall, which enters a compiled callee past its prologue. */
int luaJIT_prepare(lua_State *L, StkId func)
{
  Proto *pt = clvalue(func)->l.p;
  if (pt->jit_status == JIT_S_NONE)
    jit_compile(L, func);
  return pt->jit_status;
}

/* ------------------------------------------------------------------------ */

/* Deoptimize the current instruction. Return new mcode addr to continue. */
/* only the hinted paths deoptimize, and there are no hints. */
void *luaJIT_deoptimize(lua_State *L)
{
  luaG_runerror(L, "unexpected JIT deoptimization");
  return NULL;
}

/* ------------------------------------------------------------------------ */

/* Enforce (dynamic) linker error for version mismatches. See lua.c. */
LUA_API void LUAJIT_VERSION_SYM(void)
{
}

/* ------------------------------------------------------------------------ */

