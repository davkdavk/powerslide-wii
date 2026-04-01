#ifndef WII_LUALIB_STUB_H
#define WII_LUALIB_STUB_H

#include "lua.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline void luaopen_base(lua_State* L) { (void)L; }
static inline void luaopen_table(lua_State* L) { (void)L; }
static inline void luaopen_string(lua_State* L) { (void)L; }
static inline void luaopen_math(lua_State* L) { (void)L; }

#ifdef __cplusplus
}
#endif

#endif
