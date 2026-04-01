#ifndef LuaIncludeH
#define LuaIncludeH

#if defined(WII) || defined(__wii__)
extern "C"{
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}
#else
extern "C"{
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}
#endif

#endif
