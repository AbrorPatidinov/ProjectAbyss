#include "../include/symbols.h"
#include "../include/common.h"
#include <stdlib.h>
#include <string.h>

Symbol *globals = NULL;
int global_count = 0;
static int global_cap = 0;

Symbol *locals = NULL;
int local_count = 0;
int local_cap = 0;

FuncInfo *funcs = NULL;
int func_count = 0;
static int func_cap = 0;

StructInfo *structs = NULL;
int struct_count = 0;
static int struct_cap = 0;

EnumInfo *enums = NULL;
int enum_count = 0;
static int enum_cap = 0;

void symbols_init() {
  global_cap = INIT_CAP;
  global_count = 0;
  globals = malloc(global_cap * sizeof(Symbol));

  local_cap = INIT_CAP;
  local_count = 0;
  locals = malloc(local_cap * sizeof(Symbol));

  func_cap = INIT_CAP;
  func_count = 0;
  funcs = malloc(func_cap * sizeof(FuncInfo));

  struct_cap = INIT_CAP;
  struct_count = 0;
  structs = malloc(struct_cap * sizeof(StructInfo));

  enum_cap = INIT_CAP;
  enum_count = 0;
  enums = malloc(enum_cap * sizeof(EnumInfo));
}

int find_local(const char *name) {
  for (int i = local_count - 1; i >= 0; i--)
    if (!strcmp(locals[i].name, name))
      return i;
  return -1;
}
int find_global(const char *name) {
  for (int i = 0; i < global_count; i++)
    if (!strcmp(globals[i].name, name))
      return i;
  return -1;
}
int find_func(const char *name) {
  for (int i = 0; i < func_count; i++)
    if (!strcmp(funcs[i].name, name))
      return i;
  return -1;
}
int find_struct(const char *name) {
  for (int i = 0; i < struct_count; i++)
    if (!strcmp(structs[i].name, name))
      return i;
  return -1;
}

void add_local(char *name, DataType type, int sid, int ad) {
  if (local_count >= local_cap) {
    local_cap = local_cap ? local_cap * 2 : INIT_CAP;
    locals = realloc(locals, local_cap * sizeof(Symbol));
  }
  locals[local_count].name = name;
  locals[local_count].type = type;
  locals[local_count].struct_id = sid;
  locals[local_count].array_depth = ad;
  locals[local_count].offset = local_count;
  local_count++;
}

void add_global(char *name, DataType type, int sid, int ad) {
  if (global_count >= global_cap) {
    global_cap = global_cap ? global_cap * 2 : INIT_CAP;
    globals = realloc(globals, global_cap * sizeof(Symbol));
  }
  globals[global_count].name = name;
  globals[global_count].type = type;
  globals[global_count].struct_id = sid;
  globals[global_count].array_depth = ad;
  global_count++;
}

int add_struct(char *name) {
  if (struct_count >= struct_cap) {
    struct_cap = struct_cap ? struct_cap * 2 : INIT_CAP;
    structs = realloc(structs, struct_cap * sizeof(StructInfo));
  }
  int sid = struct_count;
  structs[sid].name = name;
  structs[sid].is_interface = 0;
  memset(structs[sid].fields, 0, sizeof(structs[sid].fields));
  struct_count++;
  return sid;
}

int add_func(char *name, uint32_t addr) {
  if (func_count >= func_cap) {
    func_cap = func_cap ? func_cap * 2 : INIT_CAP;
    funcs = realloc(funcs, func_cap * sizeof(FuncInfo));
  }
  funcs[func_count].name = name;
  funcs[func_count].addr = addr;
  return func_count++;
}

int add_enum(char *name) {
  if (enum_count >= enum_cap) {
    enum_cap = enum_cap ? enum_cap * 2 : INIT_CAP;
    enums = realloc(enums, enum_cap * sizeof(EnumInfo));
  }
  int eid = enum_count;
  enums[eid].name = name;
  enums[eid].value_count = 0;
  enum_count++;
  return eid;
}

void add_enum_value(int eid, char *name, int value) {
  int vc = enums[eid].value_count;
  enums[eid].values[vc].name = name;
  enums[eid].values[vc].value = value;
  enums[eid].value_count++;
}

int find_enum(const char *name) {
  for (int i = 0; i < enum_count; i++)
    if (!strcmp(enums[i].name, name))
      return i;
  return -1;
}

int get_enum_value(int eid, const char *name) {
  for (int i = 0; i < enums[eid].value_count; i++)
    if (!strcmp(enums[eid].values[i].name, name))
      return enums[eid].values[i].value;
  return -1;
}
