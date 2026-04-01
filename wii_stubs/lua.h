#ifndef WII_LUA_STUB_H
#define WII_LUA_STUB_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct lua_State { int dummy; } lua_State;
typedef int (*lua_CFunction)(lua_State* L);
typedef double lua_Number;
typedef long lua_Integer;

enum {
    LUA_TNONE = -1,
    LUA_TNIL = 0,
    LUA_TBOOLEAN = 1,
    LUA_TLIGHTUSERDATA = 2,
    LUA_TNUMBER = 3,
    LUA_TSTRING = 4,
    LUA_TTABLE = 5,
    LUA_TFUNCTION = 6,
    LUA_TUSERDATA = 7,
    LUA_TTHREAD = 8
};

#define LUA_MULTRET (-1)
#define LUA_GLOBALSINDEX (-10002)

static inline lua_State* lua_open(void) { return (lua_State*)0x1; }
static inline lua_State* luaL_newstate(void) { return lua_open(); }
static inline void lua_close(lua_State* L) { (void)L; }
static inline int lua_atpanic(lua_State* L, lua_CFunction panicf) { (void)L; (void)panicf; return 0; }

static inline int lua_gettop(lua_State* L) { (void)L; return 0; }
static inline void lua_settop(lua_State* L, int idx) { (void)L; (void)idx; }
static inline void lua_pushvalue(lua_State* L, int idx) { (void)L; (void)idx; }
static inline void lua_remove(lua_State* L, int idx) { (void)L; (void)idx; }
static inline void lua_insert(lua_State* L, int idx) { (void)L; (void)idx; }
static inline void lua_replace(lua_State* L, int idx) { (void)L; (void)idx; }

static inline int lua_type(lua_State* L, int idx) { (void)L; (void)idx; return LUA_TNIL; }
static inline const char* lua_typename(lua_State* L, int tp) { (void)L; (void)tp; return "nil"; }

static inline int lua_isnumber(lua_State* L, int idx) { (void)L; (void)idx; return 0; }
static inline int lua_isstring(lua_State* L, int idx) { (void)L; (void)idx; return 0; }
static inline int lua_iscfunction(lua_State* L, int idx) { (void)L; (void)idx; return 0; }
static inline int lua_isuserdata(lua_State* L, int idx) { (void)L; (void)idx; return 0; }

static inline int lua_toboolean(lua_State* L, int idx) { (void)L; (void)idx; return 0; }
static inline lua_Number lua_tonumber(lua_State* L, int idx) { (void)L; (void)idx; return 0.0; }
static inline lua_Integer lua_tointeger(lua_State* L, int idx) { (void)L; (void)idx; return 0; }
static inline const char* lua_tostring(lua_State* L, int idx) { (void)L; (void)idx; return ""; }
static inline void* lua_touserdata(lua_State* L, int idx) { (void)L; (void)idx; return 0; }
static inline lua_CFunction lua_tocfunction(lua_State* L, int idx) { (void)L; (void)idx; return 0; }

static inline void lua_pushnil(lua_State* L) { (void)L; }
static inline void lua_pushnumber(lua_State* L, lua_Number n) { (void)L; (void)n; }
static inline void lua_pushinteger(lua_State* L, lua_Integer n) { (void)L; (void)n; }
static inline void lua_pushlstring(lua_State* L, const char* s, unsigned long len) { (void)L; (void)s; (void)len; }
static inline void lua_pushstring(lua_State* L, const char* s) { (void)L; (void)s; }
static inline void lua_pushcclosure(lua_State* L, lua_CFunction fn, int n) { (void)L; (void)fn; (void)n; }
static inline void lua_pushlightuserdata(lua_State* L, void* p) { (void)L; (void)p; }

static inline int lua_pcall(lua_State* L, int nargs, int nresults, int errfunc) { (void)L; (void)nargs; (void)nresults; (void)errfunc; return 0; }
static inline void lua_call(lua_State* L, int nargs, int nresults) { (void)L; (void)nargs; (void)nresults; }

static inline int lua_next(lua_State* L, int idx) { (void)L; (void)idx; return 0; }

static inline void lua_gettable(lua_State* L, int idx) { (void)L; (void)idx; }
static inline void lua_settable(lua_State* L, int idx) { (void)L; (void)idx; }
static inline void lua_getfield(lua_State* L, int idx, const char* k) { (void)L; (void)idx; (void)k; }
static inline void lua_setfield(lua_State* L, int idx, const char* k) { (void)L; (void)idx; (void)k; }
static inline void lua_rawgeti(lua_State* L, int idx, int n) { (void)L; (void)idx; (void)n; }
static inline void lua_rawseti(lua_State* L, int idx, int n) { (void)L; (void)idx; (void)n; }

static inline void lua_createtable(lua_State* L, int narr, int nrec) { (void)L; (void)narr; (void)nrec; }
static inline void lua_newtable(lua_State* L) { (void)L; }

static inline int lua_objlen(lua_State* L, int idx) { (void)L; (void)idx; return 0; }

static inline void lua_getglobal(lua_State* L, const char* name) { (void)L; (void)name; }
static inline void lua_setglobal(lua_State* L, const char* name) { (void)L; (void)name; }
static inline void lua_register(lua_State* L, const char* n, lua_CFunction f) { (void)L; (void)n; (void)f; }

#define lua_pop(L,n) lua_settop((L), -(n)-1)
#define lua_pushcfunction(L,f) lua_pushcclosure((L), (f), 0)

#ifdef __cplusplus
}
#endif

#endif
