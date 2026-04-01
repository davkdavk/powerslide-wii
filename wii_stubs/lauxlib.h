#ifndef WII_LAUXLIB_STUB_H
#define WII_LAUXLIB_STUB_H

#include "lua.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct luaL_Reg {
    const char* name;
    lua_CFunction func;
} luaL_Reg;

static inline void luaL_openlibs(lua_State* L) { (void)L; }
static inline int luaL_loadfile(lua_State* L, const char* filename) { (void)L; (void)filename; return 0; }
static inline int luaL_loadbuffer(lua_State* L, const char* buff, unsigned long sz, const char* name) { (void)L; (void)buff; (void)sz; (void)name; return 0; }
static inline int luaL_loadstring(lua_State* L, const char* s) { (void)L; (void)s; return 0; }
static inline int luaL_dofile(lua_State* L, const char* filename) { (void)L; (void)filename; return 0; }
static inline int luaL_dostring(lua_State* L, const char* str) { (void)L; (void)str; return 0; }

static inline const char* luaL_checkstring(lua_State* L, int narg) { (void)L; (void)narg; return ""; }
static inline lua_Number luaL_checknumber(lua_State* L, int narg) { (void)L; (void)narg; return 0.0; }
static inline lua_Integer luaL_checkinteger(lua_State* L, int narg) { (void)L; (void)narg; return 0; }
static inline void luaL_checktype(lua_State* L, int narg, int t) { (void)L; (void)narg; (void)t; }

static inline int luaL_ref(lua_State* L, int t) { (void)L; (void)t; return 0; }
static inline void luaL_unref(lua_State* L, int t, int ref) { (void)L; (void)t; (void)ref; }
static inline int luaL_getn(lua_State* L, int t) { (void)L; (void)t; return 0; }
static inline void luaL_setn(lua_State* L, int t, int n) { (void)L; (void)t; (void)n; }
static inline const char* luaL_findtable(lua_State* L, int idx, const char* fname, int szhint)
{
    (void)L;
    (void)idx;
    (void)fname;
    (void)szhint;
    return 0;
}

static inline int luaL_error(lua_State* L, const char* fmt, ...) { (void)L; (void)fmt; return 0; }

static inline void* luaL_checkudata(lua_State* L, int ud, const char* tname) { (void)L; (void)ud; (void)tname; return 0; }
static inline int luaL_newmetatable(lua_State* L, const char* tname) { (void)L; (void)tname; return 1; }
static inline void luaL_register(lua_State* L, const char* libname, const luaL_Reg* l) { (void)L; (void)libname; (void)l; }

#ifdef __cplusplus
}
#endif

#endif
