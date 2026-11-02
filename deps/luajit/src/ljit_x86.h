/*
** This file has been pre-processed with DynASM.
** http://luajit.org/dynasm.html
** DynASM version 1.1.4, DynASM x86 version 1.1.4
** DO NOT EDIT! The original file is in "ljit_x86.dasc".
*/

#if DASM_VERSION != 10104
#error "Version mismatch between DynASM and included encoding engine"
#endif

/*
** Bytecode to machine code translation for x86 CPUs.
** Copyright (C) 2005-2012 Mike Pall. See Copyright Notice in luajit.h
*/

#define DASM_SECTION_CODE	0
#define DASM_SECTION_DEOPT	1
#define DASM_SECTION_TAIL	2
#define DASM_SECTION_MFMAP	3
#define DASM_MAXSECTION		4
#define Dt1(_V) (int)&(((lua_State *)0)_V)
#define Dt2(_V) (int)&(((TValue *)0)_V)
#define Dt3(_V) (int)&(((TValue *)0)_V)
#define Dt4(_V) (int)&(((CallInfo *)0)_V)
#define Dt5(_V) (int)&(((LClosure *)0)_V)
#define Dt6(_V) (int)&(((global_State *)0)_V)
#define Dt7(_V) (int)&(((TValue *)0)_V)
#define Dt8(_V) (int)&(((Value *)0)_V)
#define Dt9(_V) (int)&(((CallInfo *)0)_V)
#define DtA(_V) (int)&(((GCObject *)0)_V)
#define DtB(_V) (int)&(((TString *)0)_V)
#define DtC(_V) (int)&(((Table *)0)_V)
#define DtD(_V) (int)&(((CClosure *)0)_V)
#define DtE(_V) (int)&(((Proto *)0)_V)
#define DtF(_V) (int)&(((UpVal *)0)_V)
#define Dt10(_V) (int)&(((Node *)0)_V)
static const unsigned char jit_actionlist[3174] = {
  156,90,137,209,129,252,242,0,0,32,0,82,157,156,90,49,192,57,209,15,132,245,
  247,64,83,15,162,91,137,208,249,1,195,255,254,0,251,15,249,10,141,68,36,4,
  195,251,15,249,11,85,137,229,131,252,236,8,137,93,252,252,139,93,12,137,117,
  12,139,117,8,137,125,252,248,139,190,235,139,131,235,139,142,235,139,85,16,
  252,255,144,235,184,239,139,125,252,248,139,93,252,252,139,117,12,137,252,
  236,93,195,251,15,249,12,139,136,235,129,185,235,241,15,133,245,247,139,137,
  235,137,136,235,139,142,235,252,255,160,235,249,1,139,142,235,233,245,13,
  251,15,249,13,255,131,252,236,12,137,84,36,8,128,184,235,0,15,132,245,251,
  252,246,134,235,237,15,133,245,251,59,142,235,15,132,245,251,141,151,235,
  59,150,235,15,131,245,251,139,145,235,137,150,235,129,193,241,137,142,235,
  137,153,235,141,147,235,137,150,235,137,145,235,141,151,235,255,137,145,235,
  139,84,36,8,137,145,235,137,190,235,43,158,235,139,128,235,137,52,36,252,
  255,208,193,224,4,137,92,36,8,139,190,235,137,252,251,41,195,232,245,14,139,
  92,36,8,184,1,0,0,0,233,245,249,249,5,139,129,235,137,142,235,137,190,235,
  137,134,235,137,92,36,4,43,158,235,137,52,36,232,244,133,192,15,133,245,248,
  137,52,36,199,68,36,4,1,0,0,0,232,244,49,192,233,245,249,249,2,255,129,252,
  248,241,15,148,208,15,182,192,249,3,131,196,12,3,158,235,139,190,235,195,
  251,15,249,15,41,252,248,193,252,248,4,137,190,235,43,158,235,82,81,80,86,
  232,244,131,196,8,89,90,3,158,235,139,190,235,139,131,235,131,196,12,252,
  255,160,235,251,15,249,16,137,190,235,137,84,36,4,137,52,36,232,244,139,84,
  36,4,141,136,235,139,131,235,137,142,235,131,196,12,252,255,160,235,255,249,
  17,90,233,245,18,249,19,137,190,235,249,18,137,150,235,137,52,36,232,244,
  139,158,235,139,190,235,252,255,224,251,15,255,137,190,235,255,232,245,20,
  255,251,15,249,20,252,246,134,235,237,15,132,245,248,252,255,142,235,15,132,
  245,247,252,246,134,235,237,15,132,245,248,249,1,139,4,36,131,252,236,12,
  137,52,36,137,68,36,4,232,244,131,196,12,139,158,235,139,190,235,249,2,195,
  255,250,255,233,246,255,250,243,255,254,1,233,245,18,254,0,250,254,2,250,
  251,1,252,255,252,255,254,3,242,0,0,0,0,0,0,0,0,0,254,0,141,249,9,186,239,
  254,0,249,9,186,239,233,245,19,254,0,139,142,235,139,129,235,191,247,253,
  59,129,235,15,131,245,21,249,7,255,251,15,249,21,137,52,36,232,244,139,158,
  235,252,255,231,255,141,135,235,131,252,236,12,59,134,235,15,131,245,15,59,
  142,235,141,137,235,15,132,245,16,49,192,137,153,235,129,195,241,137,142,
  235,137,145,235,255,141,147,235,57,215,255,137,223,255,15,71,252,250,255,
  141,147,235,137,129,235,137,145,235,137,150,235,137,158,235,137,153,235,255,
  15,130,245,251,249,4,254,2,249,5,137,135,235,252,255,53,239,143,135,235,129,
  199,241,57,215,15,130,245,5,233,245,4,254,0,137,190,235,137,185,235,137,129,
  235,255,252,243,15,126,131,235,139,139,235,139,147,235,102,15,214,135,235,
  137,143,235,133,210,15,68,21,239,137,151,235,131,61,239,0,15,69,21,239,131,
  61,239,0,15,68,21,239,137,21,239,137,131,235,139,21,239,137,147,235,255,139,
  142,235,255,137,252,251,141,147,235,141,187,235,137,145,235,137,150,235,255,
  139,13,239,255,137,135,235,137,143,235,255,249,2,137,135,235,137,143,235,
  137,135,235,137,143,235,129,199,241,57,215,15,130,245,2,255,139,139,235,255,
  139,142,235,139,137,235,139,137,235,255,139,145,235,131,252,250,1,25,201,
  65,137,13,239,133,210,15,68,21,239,131,61,239,0,15,68,21,239,137,21,239,255,
  199,5,239,0,0,0,0,139,134,235,133,192,15,132,245,247,57,152,235,15,130,245,
  247,137,52,36,137,92,36,4,232,244,249,1,255,141,187,235,255,141,155,235,232,
  245,14,137,199,139,142,235,139,153,235,131,196,12,49,192,195,255,251,15,249,
  14,252,246,134,235,237,15,133,245,255,139,142,235,141,145,235,137,150,235,
  139,130,235,137,134,235,139,130,235,137,134,235,139,129,235,139,145,235,86,
  133,210,15,136,245,250,15,132,245,252,193,226,4,141,52,16,249,2,57,252,251,
  15,131,245,249,252,243,15,126,131,235,255,139,139,235,139,147,235,102,15,
  214,128,235,137,136,235,133,210,15,68,21,239,137,144,235,131,61,239,0,15,
  69,21,239,131,61,239,0,15,68,21,239,137,21,239,129,192,241,129,195,241,57,
  252,240,15,130,245,2,233,245,252,249,3,199,128,235,0,0,0,0,139,21,239,137,
  144,235,129,192,241,57,252,240,15,130,245,3,255,233,245,252,249,4,57,252,
  251,15,131,245,252,252,243,15,126,131,235,139,139,235,139,147,235,102,15,
  214,128,235,137,136,235,133,210,15,68,21,239,137,144,235,131,61,239,0,15,
  69,21,239,131,61,239,0,15,68,21,239,137,21,239,129,192,241,129,195,241,233,
  245,4,249,6,94,137,134,235,255,195,249,9,137,190,235,131,252,236,12,137,52,
  36,137,92,36,4,232,244,131,196,12,139,134,235,195,255,139,13,239,252,247,
  217,25,201,35,13,239,137,76,36,8,199,5,239,0,0,0,0,255,129,195,241,255,139,
  142,235,131,187,235,6,255,139,131,235,186,239,137,145,235,15,132,245,247,
  232,245,22,249,1,255,251,15,249,22,131,252,236,12,137,150,235,137,190,235,
  137,52,36,137,92,36,4,232,244,131,196,12,137,195,139,190,235,139,131,235,
  139,142,235,195,255,186,239,252,255,144,235,255,129,252,235,241,255,137,158,
  235,133,192,139,84,36,8,255,15,68,145,235,131,252,250,1,25,201,65,137,13,
  239,133,210,15,68,21,239,131,61,239,0,15,68,21,239,137,21,239,255,141,187,
  235,137,190,235,255,137,190,235,199,134,235,239,141,131,235,137,52,36,137,
  68,36,4,232,244,131,252,248,1,15,132,245,247,133,192,15,132,245,248,139,158,
  235,252,255,224,249,2,139,142,235,139,153,235,139,190,235,131,196,12,49,192,
  195,249,1,139,158,235,139,84,36,8,131,252,250,1,25,201,65,137,13,239,133,
  210,15,68,21,239,131,61,239,0,15,68,21,239,137,21,239,139,190,235,255,252,
  243,15,126,131,235,139,139,235,139,147,235,102,15,214,131,235,137,139,235,
  133,210,15,68,21,239,137,147,235,131,61,239,0,15,69,21,239,131,61,239,0,15,
  68,21,239,137,21,239,255,199,131,235,0,0,0,0,255,199,131,235,1,0,0,0,255,
  217,252,238,255,217,232,255,221,5,239,255,221,155,235,199,131,235,3,0,0,0,
  255,199,131,235,239,199,131,235,4,0,0,0,255,199,131,235,239,186,239,131,61,
  239,0,15,69,21,239,131,61,239,0,15,68,21,239,137,21,239,255,49,192,139,13,
  239,255,137,131,235,137,139,235,255,141,147,235,141,187,235,249,1,137,130,
  235,137,138,235,129,194,241,57,252,250,15,134,245,1,255,186,1,0,0,0,137,147,
  235,137,147,235,255,199,131,235,0,0,0,0,199,131,235,1,0,0,0,255,139,131,235,
  255,139,142,235,139,185,235,139,135,235,255,139,136,235,139,185,235,252,243,
  15,126,135,235,139,143,235,139,151,235,102,15,214,131,235,137,139,235,133,
  210,15,68,21,239,137,147,235,131,61,239,0,15,69,21,239,131,61,239,0,15,68,
  21,239,137,21,239,255,139,136,235,139,185,235,252,243,15,126,131,235,139,
  131,235,139,147,235,102,15,214,135,235,137,135,235,137,151,235,133,210,15,
  68,21,239,131,61,239,0,15,69,21,239,131,61,239,0,15,68,21,239,137,21,239,
  139,147,235,131,187,235,4,15,131,245,251,249,4,254,2,249,5,252,246,130,235,
  237,15,132,245,4,252,246,129,235,237,15,132,245,4,232,245,23,233,245,4,254,
  0,251,15,249,23,137,84,36,12,137,76,36,8,137,116,36,4,233,244,255,251,15,
  249,24,131,61,239,0,15,133,245,254,139,142,235,139,185,235,139,135,235,139,
  184,235,15,182,143,235,184,1,0,0,0,211,224,72,139,138,235,35,129,235,107,
  192,40,3,135,235,249,1,131,184,235,4,15,133,245,248,57,136,235,15,133,245,
  248,131,184,235,0,15,132,245,249,249,4,255,252,243,15,126,128,235,139,136,
  235,139,144,235,102,15,214,131,235,137,139,235,133,210,15,68,21,239,137,147,
  235,131,61,239,0,15,69,21,239,131,61,239,0,15,68,21,239,137,21,239,139,158,
  235,195,249,2,139,128,235,133,192,15,133,245,1,184,239,249,3,139,143,235,
  133,201,15,132,245,4,252,246,129,235,237,255,15,133,245,4,249,8,139,12,36,
  131,252,236,12,137,142,235,137,52,36,137,92,36,4,137,84,36,8,232,244,131,
  196,12,139,158,235,195,255,137,52,36,199,68,36,4,239,199,68,36,8,239,232,
  244,137,131,235,199,131,235,5,0,0,0,139,21,239,137,147,235,255,186,239,255,
  232,245,24,255,186,239,141,139,235,199,134,235,239,137,52,36,137,76,36,4,
  137,84,36,8,232,244,139,158,235,255,185,239,255,141,139,235,255,141,147,235,
  255,199,134,235,239,83,81,82,86,232,244,131,196,16,139,158,235,255,199,134,
  235,239,82,81,83,86,232,244,131,196,16,139,158,235,255,104,239,104,239,104,
  239,86,232,244,131,196,16,139,158,235,255,184,239,255,131,187,235,3,15,133,
  245,255,252,242,15,16,131,235,252,242,15,44,192,252,242,15,42,200,102,15,
  46,193,15,133,245,255,15,138,245,255,255,131,187,235,3,15,133,245,255,252,
  242,15,16,131,235,252,242,15,44,200,252,242,15,42,201,102,15,46,193,15,133,
  245,255,15,138,245,255,133,201,15,142,245,255,255,153,252,247,252,249,133,
  210,15,137,245,247,1,202,249,1,252,242,15,42,194,252,242,15,17,131,235,255,
  139,21,239,137,147,235,249,4,254,2,249,9,255,131,187,235,3,139,131,235,15,
  133,245,255,133,192,15,136,245,255,255,221,131,235,221,5,239,255,221,5,239,
  221,131,235,255,139,131,235,193,224,4,11,131,235,131,252,248,51,139,131,235,
  15,133,245,255,11,131,235,15,136,245,255,221,131,235,221,131,235,255,249,
  1,217,252,248,223,224,158,15,138,245,1,221,217,255,131,187,235,3,15,133,245,
  255,221,131,235,255,216,200,255,217,192,216,200,255,220,201,255,222,201,255,
  199,4,36,239,199,68,36,4,239,199,68,36,8,239,131,187,235,3,15,133,245,255,
  219,44,36,220,139,235,217,192,217,252,252,220,233,217,201,217,252,240,217,
  232,222,193,217,252,253,221,217,255,251,15,249,25,217,232,221,68,36,8,217,
  252,241,139,68,36,4,219,56,195,255,131,187,235,3,15,133,245,255,255,131,187,
  235,3,255,139,131,235,193,224,4,11,131,235,131,252,248,51,255,216,192,255,
  220,131,235,255,220,163,235,255,220,171,235,255,220,139,235,255,220,179,235,
  255,220,187,235,255,131,252,236,16,221,28,36,221,131,235,221,92,36,8,232,
  244,131,196,16,255,131,252,236,16,221,92,36,8,221,131,235,221,28,36,232,244,
  131,196,16,255,217,224,255,223,233,221,216,255,218,233,223,224,158,255,15,
  138,246,255,15,130,246,255,15,134,246,255,15,135,246,255,15,131,246,255,221,
  155,235,255,161,239,137,131,235,255,199,134,235,239,137,52,36,137,76,36,4,
  137,84,36,8,232,244,133,192,139,158,235,255,15,133,246,255,15,132,246,255,
  199,134,235,239,199,4,36,239,82,81,83,86,232,244,131,196,16,139,158,235,255,
  199,134,235,239,137,52,36,137,92,36,4,137,76,36,8,232,244,139,158,235,255,
  139,131,235,139,139,235,186,1,0,0,0,33,193,209,232,9,193,49,192,57,209,17,
  192,137,147,235,137,131,235,139,13,239,137,139,235,255,199,134,235,239,104,
  239,104,239,104,239,86,232,244,131,196,16,139,158,235,255,131,187,235,0,255,
  139,131,235,139,139,235,72,73,9,200,255,139,131,235,72,11,131,235,255,131,
  187,235,3,15,133,246,221,131,235,221,5,239,255,131,187,235,4,15,133,246,129,
  187,235,239,255,139,131,235,59,131,235,15,133,246,255,131,252,248,3,15,133,
  245,9,221,131,235,221,131,235,255,131,252,248,4,15,133,245,9,139,139,235,
  59,139,235,255,141,147,235,141,139,235,199,134,235,239,137,52,36,137,76,36,
  4,137,84,36,8,232,244,72,139,158,235,255,139,131,235,139,139,235,137,194,
  33,202,141,20,80,209,234,255,15,132,245,247,255,15,133,245,247,255,252,243,
  15,126,131,235,139,139,235,139,147,235,102,15,214,131,235,137,139,235,133,
  210,15,68,21,239,137,147,235,131,61,239,0,15,69,21,239,131,61,239,0,15,68,
  21,239,137,21,239,233,246,249,1,255,141,131,235,199,134,235,239,137,52,36,
  137,68,36,4,232,244,233,246,255,252,242,15,16,131,235,252,242,15,16,139,235,
  252,242,15,88,193,102,15,87,210,102,15,46,202,252,242,15,16,139,235,15,135,
  245,247,102,15,46,193,233,245,248,249,1,102,15,46,200,249,2,15,130,245,249,
  252,242,15,17,131,235,161,239,137,131,235,252,242,15,17,131,235,199,131,235,
  3,0,0,0,161,239,137,131,235,233,246,249,3,255,199,134,235,239,137,52,36,199,
  68,36,4,239,199,68,36,8,239,232,244,139,158,235,133,192,15,133,246,255,141,
  131,235,137,68,36,4,255,137,92,36,4,255,137,52,36,232,244,255,139,187,235,
  255,139,142,235,139,185,235,139,191,235,255,139,151,235,137,52,36,199,68,
  36,4,239,137,84,36,8,232,244,199,128,235,239,137,131,235,199,131,235,6,0,
  0,0,139,13,239,137,139,235,255,139,151,235,137,144,235,255,137,199,255,137,
  52,36,232,244,137,135,235,255,199,134,235,239,137,52,36,199,68,36,4,239,199,
  68,36,8,239,232,244,139,158,235,255
};

enum {
  JSUB_STACKPTR,
  JSUB_GATE_LJ,
  JSUB_GATE_JL,
  JSUB_GATE_JC,
  JSUB_POSCALL,
  JSUB_GROW_STACK,
  JSUB_GROW_CI,
  JSUB_DEOPTIMIZE_CALLER,
  JSUB_DEOPTIMIZE,
  JSUB_DEOPTIMIZE_OPEN,
  JSUB_HOOKINS,
  JSUB_GCSTEP,
  JSUB_METACALL,
  JSUB_BARRIERF,
  JSUB_GETGLOBAL,
  JSUB_LOG2_TWORD,
  JSUB__MAX
};

/* ------------------------------------------------------------------------ */

/* Arch string. */
const char luaJIT_arch[] = "x86";

/* The taint globals (lua.h, or the host's own copies), as displacements for the TAINT* operands of ljit_x86.dash. */
#define JIT_TAINTED		((int)&lua_tainted)
#define JIT_TAINTEXPECTED	((int)&lua_taintexpected)
#define JIT_TAINTEDCLOSURE	((int)&lua_taintedclosure)
#define JIT_TAINTCALLBACK	((int)&lua_tainteventcallback)

/* Forward declarations for C functions called from jsubs. */
static void jit_hookins(lua_State *L, const Instruction *newpc);
static void jit_getglobal(lua_State *L, StkId dest, TValue *key);

/* the interpreter's pow (luai_numpow); &pow is an overload set in C++. */
static double jit_pow(double a, double b)
{
  return luai_numpow(a, b);
}

/* a number constant holding an int32 value, for OP_MOD's integer path. */
static int jit_int32k(const TValue *k, int *out)
{
  lua_Number n = nvalue(k);
  if (!(n >= -2147483648.0 && n <= 2147483647.0) || (lua_Number)(int)n != n)
    return 0;
  *out = (int)n;
  return 1;
}

/* ------------------------------------------------------------------------ */

/* Detect CPU features and set JIT flags. */
static int jit_cpudetect(jit_State *J)
{
  void *mcode;
  size_t sz;
  int status;
  /* Some of the jsubs need the flags. So compile this separately. */
  unsigned int feature;
  dasm_setup(Dst, jit_actionlist);
  dasm_put(Dst, 0);
  (void)dasm_checkstep(Dst, DASM_SECTION_CODE);
  status = luaJIT_link(J, &mcode, &sz);
  if (status != JIT_S_OK)
    return status;
  /* Check feature bits. See the Intel/AMD manuals for the bit definitions. */
  feature = ((unsigned int (*)(void))mcode)();
  if (feature & (1<<15)) J->flags |= JIT_F_CPU_CMOV;
  if (feature & (1<<26)) J->flags |= JIT_F_CPU_SSE2;
  luaJIT_freemcode(J, mcode, sz);  /* We don't need this code anymore. */
  return JIT_S_OK;
}

/* Check some assumptions. Should compile to nop. */
static int jit_consistency_check(jit_State *J)
{
  do {
    /* Force a compiler error for inconsistent structure sizes. */
    /* Check LUA_TVALUE_ALIGN in luaconf.h, too. */
    int check_TVALUE_SIZE_in_ljit_x86_dash[1+16-sizeof(TValue)];
    int check_TVALUE_SIZE_in_ljit_x86_dash_[1+sizeof(TValue)-16];
    ((void)check_TVALUE_SIZE_in_ljit_x86_dash[0]);
    ((void)check_TVALUE_SIZE_in_ljit_x86_dash_[0]);
    if (LUA_TNIL != 0 || LUA_TBOOLEAN != 1 || PCRLUA != 0) break;
    if ((int)&(((Node *)0)->i_val) != (int)&(((StkId)0)->value)) break;
    if (sizeof(Node) != 40) break;  /* Nodemul in ljit_x86.dash */
    return JIT_S_OK;
  } while (0);
  J->dasmstatus = 999999999;  /* Recognizable error. */
  return JIT_S_COMPILER_ERROR;
}

/* Compile JIT subroutines (once). */
static int jit_compile_jsub(jit_State *J)
{
  int status = jit_consistency_check(J);
  if (status != JIT_S_OK) return status;
  status = jit_cpudetect(J);
  if (status != JIT_S_OK) return status;
  /* the taint macros use cmov and movq. */
  if (!(J->flags & JIT_F_CPU_CMOV) || !(J->flags & JIT_F_CPU_SSE2))
    return JIT_S_COMPILER_ERROR;
  dasm_setup(Dst, jit_actionlist);
  dasm_put(Dst, 34);
  dasm_put(Dst, 36, Dt1(->top), Dt2(->value), Dt1(->ci), Dt5(->jit_gate), PCRJIT, Dt5(->p), DtE(->jit_status), JIT_S_OK, DtE(->jit_mcode), Dt5(->jit_gate), Dt1(->ci), Dt5(->jit_gate), Dt1(->ci));
  dasm_put(Dst, 148, Dt5(->isC), Dt1(->hookmask), LUA_MASKCALL|LUA_MASKRET, Dt1(->end_ci), Dt3([LUA_MINSTACK]), Dt1(->stack_last), Dt4(->savedpc), Dt1(->savedpc), sizeof(CallInfo), Dt1(->ci), Dt4(->func), Dt2([1]), Dt1(->base), Dt4(->base), Dt3([LUA_MINSTACK]));
  dasm_put(Dst, 218, Dt4(->top), Dt4(->nresults), Dt1(->top), Dt1(->stack), DtD(->f), Dt1(->top), Dt4(->savedpc), Dt1(->ci), Dt1(->top), Dt1(->savedpc), Dt1(->stack), (ptrdiff_t)(luaD_precall), (ptrdiff_t)(luaV_execute));
  dasm_put(Dst, 326, PCRC, Dt1(->stack), Dt1(->top), Dt1(->top), Dt1(->stack), (ptrdiff_t)(luaD_growstack), Dt1(->stack), Dt1(->top), Dt2(->value), Dt5(->jit_gate), Dt1(->top), (ptrdiff_t)(luaD_growCI), Dt9([-1]), Dt2(->value), Dt1(->ci), Dt5(->jit_gate));
  dasm_put(Dst, 470, Dt1(->hookmask), LUA_MASKLINE|LUA_MASKCOUNT, Dt1(->hookcount), Dt1(->hookmask), LUA_MASKLINE, (ptrdiff_t)(jit_hookins), Dt1(->base), Dt1(->top));
  dasm_put(Dst, 605, (ptrdiff_t)(luaC_step), Dt1(->base));
  dasm_put(Dst, 955, Dt1(->hookmask), LUA_MASKRET, Dt1(->ci), Dt4([-1]), Dt1(->ci), Dt9(->base), Dt1(->base), Dt9(->savedpc), Dt1(->savedpc), Dt4(->func), Dt4(->nresults), Dt2([0].value));
  dasm_put(Dst, 1028, Dt2([0].tt), Dt2([0].taint), Dt7([0].value), Dt7([0].tt), JIT_TAINTED, Dt7([0].taint), JIT_TAINTEDCLOSURE, JIT_TAINTED, JIT_TAINTEXPECTED, JIT_TAINTED, JIT_TAINTED, sizeof(TValue), sizeof(TValue), Dt7(->tt), JIT_TAINTED, Dt7([0].taint), sizeof(TValue));
  dasm_put(Dst, 1112, Dt2([0].value), Dt2([0].tt), Dt2([0].taint), Dt7([0].value), Dt7([0].tt), JIT_TAINTED, Dt7([0].taint), JIT_TAINTEDCLOSURE, JIT_TAINTED, JIT_TAINTEXPECTED, JIT_TAINTED, JIT_TAINTED, sizeof(TValue), sizeof(TValue), Dt1(->top));
  dasm_put(Dst, 1188, Dt1(->top), (ptrdiff_t)(luaD_poscall), Dt1(->top));
  dasm_put(Dst, 1268, Dt1(->savedpc), Dt1(->top), (ptrdiff_t)(luaD_tryfuncTM), Dt1(->top), Dt2(->value), Dt1(->ci));
  dasm_put(Dst, 1820, (ptrdiff_t)(luaC_barrierf));
  dasm_put(Dst, 1839, JIT_TAINTCALLBACK, Dt1(->ci), Dt4(->func), Dt3(->value), Dt5(->env), DtC(->lsizenode), Dt7(->value), DtB(->tsv.hash), DtC(->node), Dt10(->i_key.nk.tt), Dt10(->i_key.nk.value), Dt10(->i_val.tt));
  dasm_put(Dst, 1915, Dt7([0].value), Dt7([0].tt), Dt7([0].taint), Dt2([0].value), Dt2([0].tt), JIT_TAINTED, Dt2([0].taint), JIT_TAINTEDCLOSURE, JIT_TAINTED, JIT_TAINTEXPECTED, JIT_TAINTED, JIT_TAINTED, Dt1(->base), Dt10(->i_key.nk.next), (ptrdiff_t)(luaO_nilobject), DtC(->metatable), DtC(->flags), 1<<TM_INDEX);
  dasm_put(Dst, 1997, Dt1(->savedpc), (ptrdiff_t)(jit_getglobal), Dt1(->base));
      dasm_put(Dst, 2440);
  dasm_put(Dst, 429, Dt1(->top), Dt1(->savedpc), (ptrdiff_t)(luaJIT_deoptimize), Dt1(->base), Dt1(->top));

  (void)dasm_checkstep(Dst, DASM_SECTION_CODE);
  status = luaJIT_link(J, &J->jsubmcode, &J->szjsubmcode);
  if (status != JIT_S_OK)
    return status;

  /* Copy the callgates from the globals to the global state. */
  G(J->L)->jit_gateLJ = (luaJIT_GateLJ)J->jsub[JSUB_GATE_LJ];
  G(J->L)->jit_gateJL = (lua_CFunction)J->jsub[JSUB_GATE_JL];
  G(J->L)->jit_gateJC = (lua_CFunction)J->jsub[JSUB_GATE_JC];
  return JIT_S_OK;
}

/* no luaJIT_debugnotify. Wow.exe sets no debug hooks and has no debug library, so the JIT_F_DEBUG
** modes, which are LuaJIT's only hook support, are never switched on. */

/* Patch a jmp into existing mcode. */
static void jit_patch_jmp(jit_State *J, void *mcode, void *to)
{
  unsigned char *patch = (unsigned char *)mcode;
  int rel = ((unsigned char *)to)-(patch+5);
  patch[0] = 0xe9;  /* jmp */
  memcpy((void *)(patch+1), &rel, 4);  /* Relative addr. */
}

/* ------------------------------------------------------------------------ */

/* Call line/count hook. */
static void jit_hookins(lua_State *L, const Instruction *newpc)
{
  Proto *pt = ci_func(L->ci)->l.p;
  int pc = luaJIT_findpc(pt, newpc);  /* Sloooow with mcode addrs. */
  const Instruction *savedpc = L->savedpc;
  L->savedpc = pt->code + pc + 1;
  if (L->hookmask > LUA_MASKLINE && L->hookcount == 0) {
    resethookcount(L);
    luaD_callhook(L, LUA_HOOKCOUNT, -1);
  }
  if (L->hookmask & LUA_MASKLINE) {
    int newline = getline(pt, pc);
    if (pc != 0) {
      int oldpc = luaJIT_findpc(pt, savedpc);
      if (!(pc <= oldpc || newline != getline(pt, oldpc))) return;
    }
    luaD_callhook(L, LUA_HOOKLINE, newline);
  }
}

/* Insert hook check for each instruction in full debug mode. */
static void jit_ins_debug(jit_State *J, int openop)
{
  if (openop) {
    dasm_put(Dst, 462, Dt1(->top));
  }
  dasm_put(Dst, 466);

}

/* Called before every instruction. */
static void jit_ins_start(jit_State *J)
{
  dasm_put(Dst, 531, J->nextpc);
}

/* Chain to another instruction. */
static void jit_ins_chainto(jit_State *J, int pc)
{
  dasm_put(Dst, 533, pc);
}

/* Set PC label. */
static void jit_ins_setpc(jit_State *J, int pc, void *target)
{
  dasm_put(Dst, 536, pc, (ptrdiff_t)(target));
}

/* Called after the last instruction has been encoded. */
static void jit_ins_last(jit_State *J, int lastpc, int sizemfm)
{
  if (J->tflags & JIT_TF_USED_DEOPT) {  /* Deopt section has been used? */
    dasm_put(Dst, 539);
    dasm_put(Dst, 541);
  }
  dasm_put(Dst, 546, lastpc+1);
  dasm_put(Dst, 549, lastpc+2);
  dasm_put(Dst, 558, sizemfm);
}

/* Add a deoptimize target for the current instruction. */
static void jit_deopt_target(jit_State *J, int nargs)
{
  if (nargs != -1) {
    dasm_put(Dst, 539);
    dasm_put(Dst, 570, (ptrdiff_t)(J->nextins));
    J->tflags |= JIT_TF_USED_DEOPT;
  } else {
    dasm_put(Dst, 547);
    dasm_put(Dst, 577, (ptrdiff_t)(J->nextins));
  }
}

/* luaC_checkGC() inlined. Destroys caller-saves + TOP (edi). Uses label 7:. */
/* Use this only at the _end_ of an instruction. */
static void jit_checkGC(jit_State *J)
{
  dasm_put(Dst, 586, Dt1(->l_G), Dt6(->totalbytes), Dt6(->GCthreshold));

}

/* ------------------------------------------------------------------------ */





#ifdef LUA_COMPAT_VARARG
static void jit_vararg_table(lua_State *L)
{
  Table *tab;
  StkId base, func;
  int i, num, numparams;
  luaC_checkGC(L);
  base = L->base;
  func = L->ci->func;
  numparams = clvalue(func)->l.p->numparams;
  num = base - func - numparams - 1;
  tab = luaH_new(L, num, 1);
  for (i = 0; i < num; i++)
    setobj2n(L, luaH_setnum(L, tab, i+1), base - num + i);
  setnvalue(luaH_setstr(L, tab, luaS_newliteral(L, "n")), (lua_Number)num);
  sethvalue(L, base + numparams, tab);
}
#endif

/* Encode JIT function prologue. */
static void jit_prologue(jit_State *J)
{
  Proto *pt = J->pt;
  int numparams = pt->numparams;
  int stacksize = pt->maxstacksize;

  dasm_put(Dst, 621, Dt3([stacksize]), Dt1(->stack_last), Dt1(->end_ci), Dt4([1]), Dt4(->func), sizeof(TValue), Dt1(->ci), Dt4(->nresults));

  if (numparams > 0) {
    dasm_put(Dst, 660, Dt2([numparams]));
  }

  if (!pt->is_vararg) {  /* Fixarg function. */
    /* Must cap L->top at L->base+numparams because 1st LOADNIL is omitted. */
    if (numparams == 0) {
      dasm_put(Dst, 666);
    } else {
      dasm_put(Dst, 669);
    }
    dasm_put(Dst, 674, Dt2([stacksize]), Dt4(->tailcalls), Dt4(->top), Dt1(->top), Dt1(->base), Dt4(->base));
  } else {  /* Vararg function. */
    int i;
    /* adjust_varargs (ldo.c): the missing fixargs become stamped nils, then each fixarg moves up with
    ** setobjs2s and leaves a stamped nil behind, in that order (a move may change lua_tainted). */
    if (numparams > 0) {
      dasm_put(Dst, 693);
      dasm_put(Dst, 701, Dt3(->tt), JIT_TAINTED, Dt3([0].taint), sizeof(TValue));
    }
    dasm_put(Dst, 727, Dt1(->base), Dt4(->base), Dt4(->tailcalls));
    for (i = 0; i < numparams; i++) {  /* Move/clear fixargs. */
      dasm_put(Dst, 737, Dt2([i].value), Dt2([i].tt), Dt2([i].taint), Dt3([i].value), Dt3([i].tt), JIT_TAINTED, Dt3([i].taint), JIT_TAINTEDCLOSURE, JIT_TAINTED, JIT_TAINTEXPECTED, JIT_TAINTED, JIT_TAINTED, Dt2([i].tt), JIT_TAINTED, Dt2([i].taint));
    }
    if (numparams > 0) {
      dasm_put(Dst, 795, Dt1(->ci));
    }
    dasm_put(Dst, 799, Dt2([stacksize]), Dt2([numparams]), Dt4(->top), Dt1(->top));
    stacksize -= numparams;		/* Fixargs are already cleared. */
  }

  /* Clear undefined args and all vars. Still assumes eax = LUA_TNIL = 0. */
  /* Note: cannot clear only args because L->top has grown. */
  /* setnilvalue, so each takes lua_tainted (ecx). */
  dasm_put(Dst, 815, JIT_TAINTED);
  if (stacksize <= EXTRA_STACK) {  /* Loopless clear. May use EXTRA_STACK. */
    int i;
    for (i = 0; i < stacksize; i++) {
      dasm_put(Dst, 819, Dt3([i].tt), Dt3([i].taint));
    }
  } else {  /* Standard loop. */
    dasm_put(Dst, 826, Dt3([0].tt), Dt3([0].taint), Dt3([1].tt), Dt3([1].taint), 2*sizeof(TValue));
  }

  dasm_put(Dst, 531, pt->sizecode+3);
  if (!pt->is_vararg) {
    dasm_put(Dst, 850, Dt2([-1].value));
  } else {
    dasm_put(Dst, 854, Dt1(->ci), Dt9(->func), Dt7(->value));
  }
  dasm_put(Dst, 864, Dt5(->taint), JIT_TAINTEDCLOSURE, JIT_TAINTED, JIT_TAINTEXPECTED, JIT_TAINTED, JIT_TAINTED);
}

/* Check if we can combine 'return const'. */
static int jit_return_k(jit_State *J)
{
  /* only a COMBINE hint asks for this, and there are no hints. */
  (void)J;
  return 0;
}

/* OP_RETURN as the interpreter does it. luaD_poscall moves only the results the caller wants
** (ci->nresults, from the call's edx), with setobjs2s, and fills the rest with stamped nils. */
static void jit_op_return(jit_State *J, int rbase, int nresults)
{
  dasm_put(Dst, 895, JIT_TAINTEDCLOSURE, Dt1(->openupval), DtF(->v), (ptrdiff_t)(luaF_close));
  if (nresults >= 0) {
    dasm_put(Dst, 930, Dt2([rbase+nresults]));
  }  /* Otherwise the previous op was open and set TOP. */
  dasm_put(Dst, 934, Dt2([rbase]), Dt1(->ci), Dt4([1].func));

}

static void jit_op_call(jit_State *J, int func, int nargs, int nresults)
{
  dasm_put(Dst, 1215, JIT_TAINTEDCLOSURE, JIT_TAINTED, JIT_TAINTEDCLOSURE);

  if (func) {
  dasm_put(Dst, 1238, func*sizeof(TValue));
  }
  dasm_put(Dst, 1242, Dt1(->ci), Dt2([0].tt));
  if (nargs >= 0) {  /* Previous op was not open and did not set TOP. */
    dasm_put(Dst, 930, Dt2([1+nargs]));
  }
  dasm_put(Dst, 1250, Dt2(->value), (ptrdiff_t)(J->nextins), Dt4(->savedpc));


  dasm_put(Dst, 1307, nresults, Dt5(->jit_gate));
  if (func) {
  dasm_put(Dst, 1314, func*sizeof(TValue));
  }
  dasm_put(Dst, 1319, Dt1(->base));
  if (!J->pt->is_vararg) {
    dasm_put(Dst, 850, Dt2([-1].value));
  } else {
    dasm_put(Dst, 854, Dt1(->ci), Dt9(->func), Dt7(->value));
  }
  dasm_put(Dst, 1329, Dt5(->taint), JIT_TAINTEDCLOSURE, JIT_TAINTED, JIT_TAINTEXPECTED, JIT_TAINTED, JIT_TAINTED);

  if (nresults >= 0) {  /* Not an open ins. Restore L->top. */
    dasm_put(Dst, 1361, Dt2([J->pt->maxstacksize]), Dt1(->top));
  }  /* Otherwise keep TOP for next instruction. */
}

/* OP_TAILCALL is luaV_tailcall (lvm.c), the interpreter's precall and frame move. LuaJIT's own moves only
** the arguments before the callee's prologue fills its nils, so a tail call left a different lua_tainted, and its
** callee started with ci->tailcalls 0, which drops the "(tail call)" lines from debugstack(). */
static void jit_op_tailcall(jit_State *J, int func, int nargs)
{
  dasm_put(Dst, 1215, JIT_TAINTEDCLOSURE, JIT_TAINTED, JIT_TAINTEDCLOSURE);
  if (nargs >= 0) {  /* Previous op was not open and did not set TOP. */
    dasm_put(Dst, 930, Dt2([func+1+nargs]));
  }
  dasm_put(Dst, 1368, Dt1(->top), Dt1(->savedpc), (ptrdiff_t)(J->nextins), Dt2([func]), (ptrdiff_t)(luaV_tailcall), Dt1(->base), Dt1(->ci), Dt4([1].func), Dt1(->top), Dt1(->base), JIT_TAINTEDCLOSURE, JIT_TAINTED, JIT_TAINTEXPECTED, JIT_TAINTED, JIT_TAINTED, Dt1(->top));
  jit_op_return(J, func, -1);
  J->combine++;  /* Combine with following return instruction. */
}

/* ------------------------------------------------------------------------ */

static void jit_op_move(jit_State *J, int dest, int src)
{
  dasm_put(Dst, 1464, Dt2([src].value), Dt2([src].tt), Dt2([src].taint), Dt2([dest].value), Dt2([dest].tt), JIT_TAINTED, Dt2([dest].taint), JIT_TAINTEDCLOSURE, JIT_TAINTED, JIT_TAINTEXPECTED, JIT_TAINTED, JIT_TAINTED);
}

static void jit_op_loadk(jit_State *J, int dest, int kidx)
{
  const TValue *kk = &J->pt->k[kidx];
  int rk = jit_return_k(J);
  if (rk) dest = 0;
  switch (ttype(kk)) {
  case 0:
  dasm_put(Dst, 1513, Dt2([dest].tt));
    break;
  case 1:
    if (bvalue(kk)) {
  dasm_put(Dst, 1521, Dt2([dest].value));
    } else {
  dasm_put(Dst, 1513, Dt2([dest].value));
    }
  dasm_put(Dst, 1521, Dt2([dest].tt));
    break;
  case 3: {
  if ((&(kk)->value)->na[0] == 0 && (&(kk)->value)->na[1] == 0) {  /* +0 only. -0 == 0, and fldz would drop its sign. */
  dasm_put(Dst, 1529);
  } else if ((&(kk)->value)->n == (lua_Number)1) {
  dasm_put(Dst, 1533);
  } else {
  dasm_put(Dst, 1536, &(kk)->value);
  }
  dasm_put(Dst, 1540, Dt2([dest].value), Dt2([dest].tt));
    break;
  }
  case 4:
  dasm_put(Dst, 1551, Dt2([dest].value), (ptrdiff_t)(gcvalue(kk)), Dt2([dest].tt));
    break;
  default: lua_assert(0); break;
  }
  if ((kk)->taint) {
  dasm_put(Dst, 1563, Dt2([dest].taint), (ptrdiff_t)((kk)->taint), (ptrdiff_t)((kk)->taint), JIT_TAINTEDCLOSURE, JIT_TAINTED, JIT_TAINTEXPECTED, JIT_TAINTED, JIT_TAINTED);
  } else {
  dasm_put(Dst, 788, JIT_TAINTED, Dt2([dest].taint));
  }
  if (rk) {
    dasm_put(Dst, 32);
  }
}

static void jit_op_loadnil(jit_State *J, int first, int last)
{
  int idx, num = last - first + 1;
  dasm_put(Dst, 1589, JIT_TAINTED);
  if (num <= 8) {
    for (idx = first; idx <= last; idx++) {
      dasm_put(Dst, 1595, Dt2([idx].tt), Dt2([idx].taint));
    }
  } else {
    dasm_put(Dst, 1602, Dt2([first]), Dt2([last]), Dt7(->tt), Dt7(->taint), sizeof(TValue));
  }
}

static void jit_op_loadbool(jit_State *J, int dest, int b, int dojump)
{
  int rk = jit_return_k(J);
  if (rk) dest = 0;
  if (b) {  /* true */
  dasm_put(Dst, 1627, Dt2([dest].value), Dt2([dest].tt));
  } else {  /* false */
  dasm_put(Dst, 1639, Dt2([dest].value), Dt2([dest].tt));
  }
  dasm_put(Dst, 788, JIT_TAINTED, Dt2([dest].taint));
  if (rk) {
    dasm_put(Dst, 32);
  } else if (dojump) {
    const TValue *h = hint_getpc(J, COMBINE, J->nextpc);
    if (!(ttisboolean(h) && bvalue(h) == 0)) {  /* Avoid jmp around dead ins. */
      dasm_put(Dst, 533, J->nextpc+1);
    }
  }
}

/* ------------------------------------------------------------------------ */

static void jit_op_getupval(jit_State *J, int dest, int uvidx)
{
  if (!J->pt->is_vararg) {
  dasm_put(Dst, 1654, Dt2([-1].value));
  } else {
  dasm_put(Dst, 1658, Dt1(->ci), Dt4(->func), Dt3(->value));
  }
  dasm_put(Dst, 1668, Dt5(->upvals[uvidx]), DtF(->v), Dt3([0].value), Dt3([0].tt), Dt3([0].taint), Dt2([dest].value), Dt2([dest].tt), JIT_TAINTED, Dt2([dest].taint), JIT_TAINTEDCLOSURE, JIT_TAINTED, JIT_TAINTEXPECTED, JIT_TAINTED, JIT_TAINTED);
}

static void jit_op_setupval(jit_State *J, int src, int uvidx)
{
  if (!J->pt->is_vararg) {
  dasm_put(Dst, 1654, Dt2([-1].value));
  } else {
  dasm_put(Dst, 1658, Dt1(->ci), Dt4(->func), Dt3(->value));
  }
  dasm_put(Dst, 1723, Dt5(->upvals[uvidx]), DtF(->v), Dt2([src].value), Dt2([src].tt), Dt2([src].taint), Dt3([0].value), Dt3([0].tt), Dt3([0].taint), JIT_TAINTED, JIT_TAINTEDCLOSURE, JIT_TAINTED, JIT_TAINTEXPECTED, JIT_TAINTED, JIT_TAINTED, Dt2([src].value), Dt2([src].tt));
  dasm_put(Dst, 1792, DtA(->gch.marked), WHITEBITS, DtF(->marked), bitmask(BLACKBIT));

}

/* ------------------------------------------------------------------------ */

/* OP_GETGLOBAL/OP_SETGLOBAL as the interpreter does them: the globals table in a temporary that takes
** lua_tainted, the key the constant itself (its taint reaches __index/__newindex, and luaH_set copies it into a
** new node's key). LuaJIT's own table paths (the TYPE-hinted GETTABLE/SETTABLE jsubs, its __index/__newindex
** fallbacks) are gone: without hints only GETGLOBAL used them, and they stamped or skipped where the
** interpreter copies. */
static void jit_getglobal(lua_State *L, StkId dest, TValue *key)
{
  TValue g;
  sethvalue(L, &g, clvalue(L->ci->func)->l.env);
  luaV_gettable(L, &g, key, dest);

}

static void jit_setglobal(lua_State *L, StkId val, TValue *key)
{
  TValue g;
  sethvalue(L, &g, clvalue(L->ci->func)->l.env);
  luaV_settable(L, &g, key, val);
}

/* ------------------------------------------------------------------------ */

static void jit_op_newtable(jit_State *J, int dest, int lnarray, int lnhash)
{
  dasm_put(Dst, 2034, luaO_fb2int(lnarray), luaO_fb2int(lnhash), (ptrdiff_t)(luaH_new), Dt2([dest].value), Dt2([dest].tt), JIT_TAINTED, Dt2([dest].taint));
  jit_checkGC(J);
}

static void jit_op_getglobal(jit_State *J, int dest, int kidx)
{
  const TValue *kk = &J->pt->k[kidx];
  jit_assert(ttisstring(kk));
  dasm_put(Dst, 2066, (ptrdiff_t)(kk));
  if (dest) {
  dasm_put(Dst, 1238, dest*sizeof(TValue));
  }
  dasm_put(Dst, 2069);
}

static void jit_op_setglobal(jit_State *J, int rval, int kidx)
{
  const TValue *kk = &J->pt->k[kidx];
  jit_assert(ttisstring(kk));
  dasm_put(Dst, 2073, (ptrdiff_t)(kk), Dt2([rval]), Dt1(->savedpc), (ptrdiff_t)(J->nextins), (ptrdiff_t)(jit_setglobal), Dt1(->base));
}

static void jit_op_gettable(jit_State *J, int dest, int tab, int rkey)
{
  if (ISK(rkey)) {
    dasm_put(Dst, 2099, (ptrdiff_t)(&J->pt->k[INDEXK(rkey)]));
  } else {
    dasm_put(Dst, 2102, Dt2([rkey]));
  }
  dasm_put(Dst, 2106, Dt2([tab]));
  if (dest) {
  dasm_put(Dst, 1238, dest*sizeof(TValue));
  }
  dasm_put(Dst, 2110, Dt1(->savedpc), (ptrdiff_t)(J->nextins), (ptrdiff_t)(luaV_gettable), Dt1(->base));
}

static void jit_op_settable(jit_State *J, int tab, int rkey, int rval)
{
  if (ISK(rkey)) {
    dasm_put(Dst, 2099, (ptrdiff_t)(&J->pt->k[INDEXK(rkey)]));
  } else {
    dasm_put(Dst, 2102, Dt2([rkey]));
  }
  if (ISK(rval)) {
    dasm_put(Dst, 2066, (ptrdiff_t)(&J->pt->k[INDEXK(rval)]));
  } else {
    dasm_put(Dst, 2106, Dt2([rval]));
  }
  if (tab) {
  dasm_put(Dst, 1238, tab*sizeof(TValue));
  }
  dasm_put(Dst, 2127, Dt1(->savedpc), (ptrdiff_t)(J->nextins), (ptrdiff_t)(luaV_settable), Dt1(->base));
}

static void jit_op_self(jit_State *J, int dest, int tab, int rkey)
{
  dasm_put(Dst, 1464, Dt2([tab].value), Dt2([tab].tt), Dt2([tab].taint), Dt2([dest+1].value), Dt2([dest+1].tt), JIT_TAINTED, Dt2([dest+1].taint), JIT_TAINTEDCLOSURE, JIT_TAINTED, JIT_TAINTEXPECTED, JIT_TAINTED, JIT_TAINTED);
  jit_op_gettable(J, dest, tab, rkey);
}

/* ------------------------------------------------------------------------ */

/* OP_SETLIST is luaV_setlist (lvm.c). It stores from the last value down with setobj2t, like the
** interpreter, so the last tainted read is the lowest one's. */
static void jit_op_setlist(jit_State *J, int ra, int num, int batch)
{
  if (batch == 0) { batch = (int)(*J->nextins); J->combine++; }
  if (num == 0) {  /* Previous op was open and set TOP: {f()} or {...}. */
    dasm_put(Dst, 462, Dt1(->top));
  }
  dasm_put(Dst, 2144, batch, num, ra, (ptrdiff_t)(luaV_setlist), Dt1(->base));
}

/* ------------------------------------------------------------------------ */

static void jit_op_arith(jit_State *J, int dest, int rkb, int rkc, int ev)
{
  const TValue *kkb = ISK(rkb) ? &J->pt->k[INDEXK(rkb)] : NULL;
  const TValue *kkc = ISK(rkc) ? &J->pt->k[INDEXK(rkc)] : NULL;
  const Value *kval;
  int idx, rev;
  int target = (ev == TM_LT || ev == TM_LE) ? jit_jmp_target(J) : 0;
  int hastail = 0;

  /* The bytecode compiler already folds constants except for: k/0, k%0, */
  /* NaN results, k1<k2, k1<=k2. No point in optimizing these cases. */
  if (ISK(rkb&rkc)) goto fallback;

  /* Avoid optimization when non-numeric constants are present. */
  if (kkb ? !ttisnumber(kkb) : (kkc && !ttisnumber(kkc))) goto fallback;

  /* x % y is a - floor(a/b)*b in the interpreter (luai_nummod); LuaJIT's fprem is exact, so it
  ** differs in the last bits. When a and b both hold int32 values and b > 0, that formula is exactly the floor
  ** modulo, which idiv gives: a/b lies at least 1/b from the next integer, more than half an ulp for |a| < 2^31,
  ** so floor sees the true quotient, and f*b and a - f*b are exact. A zero result is +0 both ways (a - a),
  ** a = -0 included. Anything else (fractions, NaN, b <= 0, |a| >= 2^31) takes luaV_arith. */
  if (ev == TM_MOD) {
    int ka = 0, kc = 0;
    if (kkb && !jit_int32k(kkb, &ka)) goto fallback;
    if (kkc && (!jit_int32k(kkc, &kc) || kc <= 0)) goto fallback;
    hastail = 1;
    if (kkb) {
      dasm_put(Dst, 2160, ka);
    } else {
      dasm_put(Dst, 2163, Dt2([rkb].tt), Dt2([rkb].value));
    }
    if (kkc) {
      dasm_put(Dst, 2099, kc);
    } else {
      dasm_put(Dst, 2200, Dt2([rkc].tt), Dt2([rkc].value));
    }
    dasm_put(Dst, 2243, Dt2([dest].value));
    if (dest != rkb && dest != rkc) {
      dasm_put(Dst, 1543, Dt2([dest].tt));
    }
    dasm_put(Dst, 2270, JIT_TAINTED, Dt2([dest].taint));
    dasm_put(Dst, 2280);
    goto fallback;
  }

  /* The TYPE hint selects numeric inlining and/or fallback encoding. */
  switch (ttype(hint_get(J, TYPE))) {
  case LUA_TNIL: hastail = 1; break;  /* No hint: numeric + fallback. */
  case LUA_TNUMBER: break;	      /* Numbers: numeric + deoptimization. */
  default: goto fallback;	      /* Mixed/other types: fallback only. */
  }

  /* The checks above ensure: at most one of the operands is a constant. */
  /* Reverse operation and swap operands so the 2nd operand is a variable. */
  if (kkc) { kval = &kkc->value; idx = rkb; rev = 1; }
  else { kval = kkb ? &kkb->value : NULL; idx = rkc; rev = 0; }

  /* Special handling for some operators. */
  switch (ev) {
  case TM_MOD:
    /* Check for modulo with positive numbers, so we can use fprem. */
    if (kval) {
      if (kval->na[1] < 0) { hastail = 0; goto fallback; }  /* x%-k, -k%x */
      dasm_put(Dst, 2283, Dt2([idx].tt), Dt2([idx].value.na[1]));
      if (kkb) {
	dasm_put(Dst, 2301, Dt2([rkc].value), kval);
      } else {
	dasm_put(Dst, 2308, kval, Dt2([rkb].value));
      }
    } else {
      dasm_put(Dst, 2315, Dt2([rkb].tt), Dt2([rkc].tt), Dt2([rkb].value.na[1]), Dt2([rkc].value.na[1]), Dt2([rkc].value), Dt2([rkb].value));
    }
    dasm_put(Dst, 2349);
    goto fpstore;
  case TM_POW:
    if (hastail || !kval) break;  /* Avoid this if not optimizing. */
    if (rev) {  /* x^k for k > 0, k integer. */
      lua_Number n = kval->n;
      int k;
      lua_number2int(k, n);
      /* All positive integers would work. But need to limit code explosion. */
      if (k > 0 && k <= 65536 && (lua_Number)k == n) {
	dasm_put(Dst, 2364, Dt2([idx].tt), Dt2([idx]));
	for (; (k & 1) == 0; k >>= 1) {  /* Handle leading zeroes (2^k). */
	  dasm_put(Dst, 2376);
	}
	if ((k >>= 1) != 0) {  /* Handle trailing bits. */
	  dasm_put(Dst, 2379);
	  for (; k != 1; k >>= 1) {
	    if (k & 1) {
	      dasm_put(Dst, 2384);
	    }
	    dasm_put(Dst, 2376);
	  }
	  dasm_put(Dst, 2387);
	}
	goto fpstore;
      }
    } else if (kval->n > (lua_Number)0) {  /* k^x for k > 0. */
      int log2kval[3];  /* Enough storage for a tword (80 bits). */
      log2kval[2] = 0;  /* Avoid leaking garbage. */
      /* Double precision log2(k) doesn't cut it (3^x != 3 for x = 1). */
      ((void (*)(int *, double))J->jsub[JSUB_LOG2_TWORD])(log2kval, kval->n);
      dasm_put(Dst, 2390, log2kval[0], log2kval[1], log2kval[2], Dt2([idx].tt), Dt2([idx].value));

      goto fpstore;
    }
    break;
  }

  /* Check number type and load 1st operand. */
  if (kval) {
    dasm_put(Dst, 2461, Dt2([idx].tt));
    if ((kval)->na[0] == 0 && (kval)->na[1] == 0) {  /* +0 only. -0 == 0, and fldz would drop its sign. */
    dasm_put(Dst, 1529);
    } else if ((kval)->n == (lua_Number)1) {
    dasm_put(Dst, 1533);
    } else {
    dasm_put(Dst, 1536, kval);
    }
  } else {
    if (rkb == rkc) {
      dasm_put(Dst, 2470, Dt2([rkb].tt));
    } else {
      dasm_put(Dst, 2475, Dt2([rkb].tt), Dt2([rkc].tt));
    }
    dasm_put(Dst, 2368, Dt2([rkb].value));
  }

  /* Encode arithmetic operation with 2nd operand. */
  switch ((ev<<1)+rev) {
  case TM_ADD<<1: case (TM_ADD<<1)+1:
    if (rkb == rkc) {
      dasm_put(Dst, 2489);
    } else {
      dasm_put(Dst, 2492, Dt2([idx].value));
    }
    break;
  case TM_SUB<<1:
    dasm_put(Dst, 2496, Dt2([idx].value));
    break;
  case (TM_SUB<<1)+1:
    dasm_put(Dst, 2500, Dt2([idx].value));
    break;
  case TM_MUL<<1: case (TM_MUL<<1)+1:
    if (rkb == rkc) {
      dasm_put(Dst, 2376);
    } else {
      dasm_put(Dst, 2504, Dt2([idx].value));
    }
    break;
  case TM_DIV<<1:
    dasm_put(Dst, 2508, Dt2([idx].value));
    break;
  case (TM_DIV<<1)+1:
    dasm_put(Dst, 2512, Dt2([idx].value));
    break;
  case TM_POW<<1:
    dasm_put(Dst, 2516, Dt2([idx].value), (ptrdiff_t)(jit_pow));
    break;
  case (TM_POW<<1)+1:
    dasm_put(Dst, 2536, Dt2([idx].value), (ptrdiff_t)(jit_pow));
    break;
  case TM_UNM<<1: case (TM_UNM<<1)+1:
    dasm_put(Dst, 2556);
    break;
  default:  /* TM_LT or TM_LE. */
    dasm_put(Dst, 2311, Dt2([idx].value));
    if (J->flags & JIT_F_CPU_CMOV) {
    dasm_put(Dst, 2559);
    } else {
    dasm_put(Dst, 2564);
    }
    dasm_put(Dst, 2570, dest?(J->nextpc+1):target);
    jit_assert(dest == 0 || dest == 1);  /* Really cond. */
    switch (((rev^dest)<<1)+(dest^(ev == TM_LT))) {
    case 0:
      dasm_put(Dst, 2574, target);
      break;
    case 1:
      dasm_put(Dst, 2578, target);
      break;
    case 2:
      dasm_put(Dst, 2582, target);
      break;
    case 3:
      dasm_put(Dst, 2586, target);
      break;
    }
    goto skipstore;
  }
fpstore:
  /* Store result and set result type (if necessary). */
  dasm_put(Dst, 2590, Dt2([dest].value));
  if (dest != rkb && dest != rkc) {
    dasm_put(Dst, 1543, Dt2([dest].tt));
  }
  dasm_put(Dst, 2594, JIT_TAINTED, Dt2([dest].taint));

skipstore:
  if (!hastail) {
    jit_deopt_target(J, 0);
    return;
  }

  dasm_put(Dst, 697);
  dasm_put(Dst, 2280);

fallback:
  /* Generic fallback for arithmetic ops. */
  if (kkb) {
    dasm_put(Dst, 2099, (ptrdiff_t)(kkb));
  } else {
    dasm_put(Dst, 2102, Dt2([rkb]));
  }
  if (kkc) {
    dasm_put(Dst, 2066, (ptrdiff_t)(kkc));
  } else {
    dasm_put(Dst, 2106, Dt2([rkc]));
  }
  if (target) {  /* TM_LT or TM_LE. */
    dasm_put(Dst, 2600, Dt1(->savedpc), (ptrdiff_t)((J->nextins+1)), (ptrdiff_t)(ev==TM_LT?luaV_lessthan:luaV_lessequal), Dt1(->base));
    if (dest) {  /* cond */
      dasm_put(Dst, 2623, target);
    } else {
      dasm_put(Dst, 2627, target);
    }
  } else {
    if (dest) {
    dasm_put(Dst, 1238, dest*sizeof(TValue));
    }
    dasm_put(Dst, 2631, Dt1(->savedpc), (ptrdiff_t)(J->nextins), ev, (ptrdiff_t)(luaV_arith), Dt1(->base));
  }

  if (hastail) {
    dasm_put(Dst, 722);
  }
}

/* ------------------------------------------------------------------------ */

/* OP_LEN is luaV_len (lvm.c). LuaJIT's fallback called __len with one argument; the interpreter's
** call_binTM passes two (the operand and nil). */
static void jit_op_len(jit_State *J, int dest, int rb)
{
  dasm_put(Dst, 2102, Dt2([rb]));
  if (dest) {
  dasm_put(Dst, 1238, dest*sizeof(TValue));
  }
  dasm_put(Dst, 2652, Dt1(->savedpc), (ptrdiff_t)(J->nextins), (ptrdiff_t)(luaV_len), Dt1(->base));
}

static void jit_op_not(jit_State *J, int dest, int rb)
{
  /* l_isfalse() without a branch -- truly devious. */
  /* ((value & tt) | (tt>>1)) is only zero for nil/false. */
  /* Assumes: LUA_TNIL == 0, LUA_TBOOLEAN == 1, bvalue() == 0/1 */
  dasm_put(Dst, 2673, Dt2([rb].tt), Dt2([rb].value), Dt2([dest].tt), Dt2([dest].value), JIT_TAINTED, Dt2([dest].taint));
}

/* ------------------------------------------------------------------------ */

/* OP_CONCAT is luaV_concatop (lvm.c): luaV_concat, the GC check, then setobjs2s to the destination, which
** stamps or taints even when the destination is the first operand. */
static void jit_op_concat(jit_State *J, int dest, int first, int last)
{
  dasm_put(Dst, 2709, Dt1(->savedpc), (ptrdiff_t)(J->nextins), last, first, dest, (ptrdiff_t)(luaV_concatop), Dt1(->base));
}

/* ------------------------------------------------------------------------ */

static void jit_op_eq(jit_State *J, int cond, int rkb, int rkc)
{
  int target = jit_jmp_target(J);
  int condtarget = cond ? (J->nextpc+1) : target;
  jit_assert(cond == 0 || cond == 1);

  /* Comparison of two constants. Evaluate at compile time. */
  if (ISK(rkb&rkc)) {
    if ((rkb == rkc) == cond) {  /* Constants are already unique. */
      dasm_put(Dst, 533, target);
    }
    return;
  }

  if (ISK(rkb|rkc)) {  /* Compare a variable and a constant. */
    const TValue *kk;
    if (ISK(rkb)) { int t = rkc; rkc = rkb; rkb = t; }  /* rkc holds const. */
    kk = &J->pt->k[INDEXK(rkc)];
    switch (ttype(kk)) {
    case LUA_TNIL:
      dasm_put(Dst, 2729, Dt2([rkb].tt));
      break;
    case LUA_TBOOLEAN:
      if (bvalue(kk)) {
	dasm_put(Dst, 2734, Dt2([rkb].tt), Dt2([rkb].value));
      } else {
	dasm_put(Dst, 2745, Dt2([rkb].tt), Dt2([rkb].value));
      }
      break;
    case LUA_TNUMBER:
      dasm_put(Dst, 2753, Dt2([rkb].tt), condtarget, Dt2([rkb].value), &kk->value);
      if (J->flags & JIT_F_CPU_CMOV) {
      dasm_put(Dst, 2559);
      } else {
      dasm_put(Dst, 2564);
      }
      dasm_put(Dst, 2570, condtarget);
      break;
    case LUA_TSTRING:
      dasm_put(Dst, 2767, Dt2([rkb].tt), condtarget, Dt2([rkb].value), (ptrdiff_t)(rawtsvalue(kk)));
      break;
    default: jit_assert(0); break;
    }
  } else {  /* Compare two variables. */
    dasm_put(Dst, 2779, Dt2([rkb].tt), Dt2([rkc].tt), condtarget);
    switch (ttype(hint_get(J, TYPE))) {
    case LUA_TNUMBER:
      jit_deopt_target(J, 0);
      dasm_put(Dst, 2789, Dt2([rkb].value), Dt2([rkc].value));
      if (J->flags & JIT_F_CPU_CMOV) {
      dasm_put(Dst, 2559);
      } else {
      dasm_put(Dst, 2564);
      }
      dasm_put(Dst, 2570, condtarget);
      break;
    case LUA_TSTRING:
      jit_deopt_target(J, 0);
      dasm_put(Dst, 2804, Dt2([rkb].value), Dt2([rkc].value));
      break;
    default:
      dasm_put(Dst, 2819, Dt2([rkc]), Dt2([rkb]), Dt1(->savedpc), (ptrdiff_t)(J->nextins), (ptrdiff_t)(luaV_equalval), Dt1(->base));
      break;
    }
  }
  if (cond) {
    dasm_put(Dst, 2627, target);
  } else {
    dasm_put(Dst, 2623, target);
  }
}

/* ------------------------------------------------------------------------ */

static void jit_op_test(jit_State *J, int cond, int dest, int src)
{
  int target = jit_jmp_target(J);

  /* l_isfalse() without a branch. But this time preserve tt/value. */
  /* (((value & tt) * 2 + tt) >> 1) is only zero for nil/false. */
  /* Assumes: 3*tt < 2^32, LUA_TNIL == 0, LUA_TBOOLEAN == 1, bvalue() == 0/1 */
  dasm_put(Dst, 2847, Dt2([src].tt), Dt2([src].value));

  /* Check if we can omit the stack copy. */
  if (dest == src) {  /* Yes, invert branch condition. */
    if (cond) {
      dasm_put(Dst, 2623, target);
    } else {
      dasm_put(Dst, 2627, target);
    }
  } else {  /* No, jump around copy code. */
    if (cond) {
      dasm_put(Dst, 2863);
    } else {
      dasm_put(Dst, 2868);
    }
    dasm_put(Dst, 2873, Dt2([src].value), Dt2([src].tt), Dt2([src].taint), Dt2([dest].value), Dt2([dest].tt), JIT_TAINTED, Dt2([dest].taint), JIT_TAINTEDCLOSURE, JIT_TAINTED, JIT_TAINTEXPECTED, JIT_TAINTED, JIT_TAINTED, target);
  }
}

static void jit_op_jmp(jit_State *J, int target)
{
  dasm_put(Dst, 533, target);
}

/* ------------------------------------------------------------------------ */

enum { FOR_IDX, FOR_LIM, FOR_STP, FOR_EXT };

/* OP_FORPREP is luaV_forprep (lvm.c), then the jump to OP_FORLOOP, as in the interpreter. LuaJIT's own
** tests the first iteration itself and never stores ra, so the loop variable was (init) where the interpreter's is
** (init - step) + step, and a coerced string kept its taint. */
static void jit_op_forprep(jit_State *J, int ra, int target)
{
  dasm_put(Dst, 2926, Dt2([ra]), Dt1(->savedpc), (ptrdiff_t)(J->nextins), (ptrdiff_t)(luaV_forprep), target);
}

/* the interpreter's OP_FORLOOP, in SSE2 like the interpreter's arithmetic: idx = ra + step, and while
** (0 < step ? idx <= limit : limit <= idx) both ra and ra+3 take idx, stamped. LuaJIT tested step's sign bit,
** which runs a step of 0 forever, and stored before the test. */
static void jit_op_forloop(jit_State *J, int ra, int target)
{
  dasm_put(Dst, 2945, Dt2([ra+FOR_IDX].value), Dt2([ra+FOR_STP].value), Dt2([ra+FOR_LIM].value), Dt2([ra+FOR_IDX].value), JIT_TAINTED, Dt2([ra+FOR_IDX].taint), Dt2([ra+FOR_EXT].value), Dt2([ra+FOR_EXT].tt), JIT_TAINTED, Dt2([ra+FOR_EXT].taint), target);
}

/* ------------------------------------------------------------------------ */

/* OP_TFORLOOP is luaV_tforloop (lvm.c). The interpreter calls the iterator with luaD_call, not like an
** OP_CALL, so after a Lua iterator lua_taintedclosure stays clear until the next call returns. */
static void jit_op_tforloop(jit_State *J, int ra, int nresults)
{
  int target = jit_jmp_target(J);
  dasm_put(Dst, 3033, Dt1(->savedpc), (ptrdiff_t)(J->nextins), ra, nresults, (ptrdiff_t)(luaV_tforloop), Dt1(->base), target);
}

/* ------------------------------------------------------------------------ */

static void jit_op_close(jit_State *J, int ra)
{
  if (ra) {
    dasm_put(Dst, 3061, Dt2([ra]));
  } else {
    dasm_put(Dst, 3069);
  }
  dasm_put(Dst, 3074, (ptrdiff_t)(luaF_close));
}

static void jit_op_closure(jit_State *J, int dest, int ptidx)
{
  Proto *npt = J->pt->p[ptidx];
  int nup = npt->nups;
  if (!J->pt->is_vararg) {
  dasm_put(Dst, 3080, Dt2([-1].value));
  } else {
  dasm_put(Dst, 3084, Dt1(->ci), Dt4(->func), Dt3(->value));
  }
  dasm_put(Dst, 3094, Dt5(->env), nup, (ptrdiff_t)(luaF_newLclosure), Dt5(->p), (ptrdiff_t)(npt), Dt2([dest].value), Dt2([dest].tt), JIT_TAINTED, Dt2([dest].taint));
  /* Process pseudo-instructions for upvalues. */
  if (nup > 0) {
    const Instruction *uvcode = J->nextins;
    int i, uvuv;
    /* Check which of the two types we need. */
    for (i = 0, uvuv = 0; i < nup; i++)
      if (GET_OPCODE(uvcode[i]) == OP_GETUPVAL) uvuv++;
    /* Copy upvalues from parent first. */
    if (uvuv) {
      /* LCL:eax->upvals (new closure) <-- LCL:edi->upvals (own closure). */
      for (i = 0; i < nup; i++)
	if (GET_OPCODE(uvcode[i]) == OP_GETUPVAL) {
	  dasm_put(Dst, 3132, Dt5(->upvals[GETARG_B(uvcode[i])]), Dt5(->upvals[i]));
	}
    }
    /* Next find or create upvalues for our own stack slots. */
    if (nup > uvuv) {
      dasm_put(Dst, 3139);
      /* LCL:edi->upvals (new closure) <-- upvalue for stack slot. */
      for (i = 0; i < nup; i++)
	if (GET_OPCODE(uvcode[i]) == OP_MOVE) {
	  int rb = GETARG_B(uvcode[i]);
	  if (rb) {
	    dasm_put(Dst, 3061, Dt2([rb]));
	  } else {
	    dasm_put(Dst, 3069);
	  }
	  dasm_put(Dst, 3142, (ptrdiff_t)(luaF_findupval), Dt5(->upvals[i]));
	}
    }
    J->combine += nup;  /* Skip pseudo-instructions. */
  }
  jit_checkGC(J);
}

/* ------------------------------------------------------------------------ */

/* OP_VARARG is luaV_vararg (lvm.c): setobjs2s for each vararg, stamped nils for the rest. */
static void jit_op_vararg(jit_State *J, int dest, int num)
{
  dasm_put(Dst, 3151, Dt1(->savedpc), (ptrdiff_t)(J->nextins), dest, num, (ptrdiff_t)(luaV_vararg), Dt1(->base));
  if (num < 0) {  /* Open ins: the next one finds the end in TOP. */
    dasm_put(Dst, 1460, Dt1(->top));
  }
}

/* ------------------------------------------------------------------------ */

