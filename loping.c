#define LUA_LIB

#include <lua.h>
#include <lauxlib.h>

#include <oping.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define LUA_OPING_NAME "oping"
#define MT_OPING "oping mt"
#define MT_OPING_ITER "oping iter mt"

#define OPTIONAL 0
#define REQUIRED 1

struct lping_ud {
  pingobj_t *po;
};

struct lping_iter_ud {
  pingobj_iter_t *iter;
};

#define print_error(fmt, ...){ \
  char err[256]; \
  int pos = snprintf (err, 256, "[%s:%d in %s] ", __FILE__, __LINE__, __func__); \
  snprintf (err+pos, 256-pos, fmt, ##__VA_ARGS__); \
  lua_pushstring (L, err); \
  lua_error (L); \
}

static void
parse_integer (lua_State *L, int index, int is_required, lua_Integer *var)
{
  int top = lua_gettop (L);

  if (top < index) {
    if (REQUIRED == is_required)
      print_error ("expected more arguments");
  } else {
    if (!lua_isnumber (L, index))
      print_error ("this function requires an integer");
    *var = lua_tointeger (L, index);
  }
}

static void
parse_number (lua_State *L, int index, int is_required, lua_Number *var)
{
  int top = lua_gettop (L);

  if (top < index) {
    if (REQUIRED == is_required)
      print_error ("expected more arguments");
  } else {
    if (!lua_isnumber (L, index))
      print_error ("this function requires a number");
    *var = lua_tonumber (L, index);
  }
}

static void
parse_string (lua_State *L, int index, int is_required, char **str)
{
  int top = lua_gettop (L);

  if (top < index) {
    if (REQUIRED == is_required)
      print_error ("expected more arguments");
  } else {
    if (!lua_isstring (L, index)) 
      print_error ("this function requires a string");
    *str = (char *) lua_tostring (L, index);
  }
}

static void
parse_userdata (lua_State *L, int index, int is_required, void **var)
{
  int top = lua_gettop (L);

  if (top < index) {
    if (REQUIRED == is_required)
      print_error ("expected more arguments");
  } else {
    if (!lua_isuserdata (L, index))
      print_error ("this function requires userdata");
    *var = lua_touserdata (L, index);
  }
}

static int
lping_construct (lua_State *L)
{
  int top = lua_gettop (L);
  struct lping_ud *ud = NULL;

  if (0 != top)
    print_error ("this function takes no arguments");

  ud = lua_newuserdata (L, sizeof *ud);
  if (NULL == ud)
    print_error ("out of memory");

  ud->po = ping_construct ();
  if (NULL == ud->po)
    print_error ("out of memory");

  luaL_getmetatable (L, MT_OPING);
  lua_setmetatable (L, -2);

  return 1;
}

static int
lping_setopt (lua_State *L)
{
  int top = lua_gettop (L);
  struct lping_ud *ud = NULL;
  lua_Integer opt = -1;
  ptrdiff_t val;

  if (3 != top)
    print_error ("this function takes exactly 3 arguments");

  parse_userdata (L, 1, REQUIRED, (void *) &ud);
  parse_integer (L, 2, REQUIRED, &opt);

  if (NULL == ud || NULL == ud->po)
    print_error ("invalid userdata object");

  switch (opt) {
    case PING_OPT_TIMEOUT:
      parse_number (L, 3, REQUIRED, (lua_Number *) &val);
      break;
    case PING_OPT_TTL:
      parse_integer (L, 3, REQUIRED, (lua_Integer *) &val);
      break;
    case PING_OPT_AF:
      parse_integer (L, 3, REQUIRED, (lua_Integer *) &val);
      break;
    case PING_OPT_DATA:
      parse_string (L, 3, REQUIRED, (char **) &val);
      break;
    case PING_OPT_SOURCE:
      parse_string (L, 3, REQUIRED, (char **) &val);
      break;
    case PING_OPT_DEVICE:
      parse_string (L, 3, REQUIRED, (char **) &val);
      break;
    default:
      print_error ("invalid option");
  }

  if (0 != ping_setopt (ud->po, opt, (void *) &val))
    print_error ("ping_setopt");

  return 0;
}

static int
lping_host_add (lua_State *L)
{
  int top = lua_gettop (L);
  struct lping_ud *ud = NULL;
  char *host;

  if (2 != top)
    print_error ("this function takes exactly 2 arguments");

  parse_userdata (L, 1, REQUIRED, (void *) &ud);
  parse_string (L, 2, REQUIRED, &host);

  if (NULL == ud || NULL == ud->po)
    print_error ("invalid userdata object");

  if (NULL == host)
    print_error ("invalid host string");

  if (0 != ping_host_add (ud->po, host))
    print_error ("%s", ping_get_error (ud->po));

  return 0;
}

static int
lping_host_remove (lua_State *L)
{
  int top = lua_gettop (L);
  struct lping_ud *ud = NULL;
  char *host;

  if (2 != top)
    print_error ("this function takes exactly 2 arguments");

  parse_userdata (L, 1, REQUIRED, (void *) &ud);
  parse_string (L, 2, REQUIRED, &host);

  if (NULL == ud || NULL == ud->po)
    print_error ("invalid userdata object");

  if (NULL == host)
    print_error ("invalid host string");

  if (0 != ping_host_remove (ud->po, host))
    print_error ("%s", ping_get_error (ud->po));

  return 0;
}

static int
lping_send (lua_State *L)
{
  int top = lua_gettop (L);
  struct lping_ud *ud = NULL;
  int rv;

  if (1 != top)
    print_error ("this function takes exactly 1 argument");

  parse_userdata (L, 1, REQUIRED, (void *) &ud);

  if (NULL == ud || NULL == ud->po)
    print_error ("invalid userdata object");

  rv = ping_send (ud->po);
  if (rv < 0)
    print_error ("%s", ping_get_error (ud->po));

  lua_pushinteger (L, rv);

  return 1;
}

static int
lping_iterator_get (lua_State *L)
{
  int top = lua_gettop (L);
  struct lping_ud *ud = NULL;
  struct lping_iter_ud *iter_ud = NULL;

  if (1 != top)
    print_error ("this function takes exactly 1 argument");

  parse_userdata (L, 1, REQUIRED, (void *) &ud);

  if (NULL == ud || NULL == ud->po)
    print_error ("invalid userdata object");

  iter_ud = lua_newuserdata (L, sizeof *iter_ud);
  if (NULL == iter_ud)
    print_error ("out of memory");

  iter_ud->iter = ping_iterator_get (ud->po);
  if (NULL == iter_ud->iter)
    print_error ("no host is associated with this ping object");

  luaL_getmetatable (L, MT_OPING_ITER);
  lua_setmetatable (L, -2);

  return 1;
}

static int
lping_iterator_next (lua_State *L)
{
  int top = lua_gettop (L);
  struct lping_iter_ud *ud = NULL;

  if (1 != top)
    print_error ("this function takes exactly 1 argument");

  parse_userdata (L, 1, REQUIRED, (void *) &ud);

  if (NULL == ud || NULL == ud->iter)
    print_error ("invalid userdata object");

  ud->iter = ping_iterator_next (ud->iter);

  if (NULL == ud->iter)
    lua_pushnil (L);

  return 1;
}

static int
lping_iterator_get_info (lua_State *L)
{
  int top = lua_gettop (L);
  struct lping_iter_ud *ud = NULL;
  lua_Integer info;
  char stack_buffer[8];
  char *buffer = stack_buffer;
  size_t buffer_len = 8;
  int rv;

  if (2 != top)
    print_error ("this function takes exactly 2 arguments");

  parse_userdata (L, 1, REQUIRED, (void *) &ud);
  parse_integer (L, 2, REQUIRED, &info);

  if (NULL == ud || NULL == ud->iter)
    print_error ("invalid userdata object");

  rv = ping_iterator_get_info (ud->iter, info, buffer, &buffer_len);
  if (EINVAL == rv) {
    print_error ("the valued passed as info is unknown");
  } else if (ENOMEM == rv) {
    buffer = malloc (buffer_len);
    if (NULL == buffer)
      print_error ("out of memory");

    rv = ping_iterator_get_info (ud->iter, info, buffer, &buffer_len);
    if (0 != rv)
      print_error ("nothing will make this oping happy");
  } else if (0 != rv) {
    print_error ("unknown return value... probably some new oping error");
  }

  switch (info) {
    case PING_INFO_USERNAME:
    case PING_INFO_HOSTNAME:
    case PING_INFO_ADDRESS:
      lua_pushstring (L, buffer);
      break;
    case PING_INFO_FAMILY:
    case PING_INFO_SEQUENCE:
    case PING_INFO_IDENT:
    case PING_INFO_RECV_TTL:
      if (sizeof (int) != buffer_len)
        print_error ("buffer size mismatch");
      lua_pushinteger (L, *((int *) buffer));
      break;
    case PING_INFO_LATENCY:
      if (sizeof (double) != buffer_len)
        print_error ("buffer size mismatch");
      lua_pushnumber (L, *((double *) buffer));
      break;
    case PING_INFO_DROPPED:
      if (sizeof (uint32_t) != buffer_len)
        print_error ("buffer size mismatch");
      lua_pushinteger (L, *((uint32_t *) buffer));
      break;
    default:
      print_error ("the value passed as info is strangely unknown");
  }

  if (stack_buffer != buffer)
    free (buffer);

  return 1;
}

static const luaL_Reg opinglib[] = {
  {"new", lping_construct},
  {NULL, NULL}
};

static const luaL_Reg oping_methods[] = {
  {"setopt", lping_setopt},
  {"host_add", lping_host_add},
  {"host_remove", lping_host_remove},
  {"send", lping_send},
  {"iterator_get", lping_iterator_get},
  {NULL, NULL}
};

static const luaL_Reg oping_iter_methods[] = {
  {"next", lping_iterator_next},
  {"get_info", lping_iterator_get_info},
  {NULL, NULL}
};

#define SET_OPING_CONST(s) lua_pushinteger (L, PING_##s); lua_setfield (L, -2, #s)
#define SET_OPING_STRING(s) lua_pushstring (L, PING_##s); lua_setfield (L, -2, #s)

LUALIB_API int luaopen_oping (lua_State *L)
{
  luaL_register (L, LUA_OPING_NAME, opinglib);

  SET_OPING_CONST (OPT_TIMEOUT);
  SET_OPING_CONST (OPT_TTL);
  SET_OPING_CONST (OPT_AF);
  SET_OPING_CONST (OPT_DATA);
  SET_OPING_CONST (OPT_SOURCE);
  SET_OPING_CONST (OPT_DEVICE);

  SET_OPING_CONST (DEF_TIMEOUT);
  SET_OPING_CONST (DEF_TTL);
  SET_OPING_CONST (DEF_AF);
  SET_OPING_STRING (DEF_DATA);

  SET_OPING_CONST (INFO_HOSTNAME);
  SET_OPING_CONST (INFO_ADDRESS);
  SET_OPING_CONST (INFO_FAMILY);
  SET_OPING_CONST (INFO_LATENCY);
  SET_OPING_CONST (INFO_SEQUENCE);
  SET_OPING_CONST (INFO_IDENT);
  SET_OPING_CONST (INFO_DATA);
  SET_OPING_CONST (INFO_USERNAME);
  SET_OPING_CONST (INFO_DROPPED);
  SET_OPING_CONST (INFO_RECV_TTL);

  if (!luaL_newmetatable (L, MT_OPING))
    print_error ("unable to create oping metatable");
  lua_createtable (L, 0, sizeof (oping_methods) / sizeof (luaL_Reg) - 1);
  luaL_register (L, NULL, oping_methods);
  lua_setfield (L, -2, "__index");

  if (!luaL_newmetatable (L, MT_OPING_ITER))
    print_error ("unable to create oping iter metatable");
  lua_createtable (L, 0, sizeof (oping_iter_methods) / sizeof (luaL_Reg) - 1);
  luaL_register (L, NULL, oping_iter_methods);
  lua_setfield (L, -2, "__index");

  return 1;
}
