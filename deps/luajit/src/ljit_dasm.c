/*
** Wrapper for architecture-specific DynASM encoder.
** Copyright (C) 2005-2012 Mike Pall. See Copyright Notice in luajit.h
*/

#define ljit_dasm_c
#define LUA_CORE


#include "lua.h"

#include "ljit.h"
#include "ljit_dasm.h"
#include "lmem.h"
#include "ldo.h"

#include <stdlib.h>


/* Glue macros for DynASM memory allocation. */
/* the CRT heap, not the Lua allocator, so compiling doesn't move totalbytes (the GC's pacing) or the
** pool. */
#define DASM_M_GROW(J, t, p, sz, need)   do {     size_t _sz = (sz), _need = (need);     if (_sz < _need) {       void *_p;       if (_sz < 16) _sz = 16;       while (_sz < _need) _sz += _sz;       _p = realloc((p), _sz);       if (_p == NULL) luaD_throw(J->L, LUA_ERRMEM);       (p) = (t *)_p;       (sz) = _sz;     }   } while(0)

#define DASM_M_FREE(J, p, sz)	free(p)

/* Embed architecture-specific DynASM encoder. */
#if defined(__i386) || defined(__i386__) || defined(_M_IX86)
#include "dasm_x86.h"
#else
#error "No support for this architecture (yet)"
#endif


