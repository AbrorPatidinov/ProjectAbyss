#ifndef SYMBOLS_H
#define SYMBOLS_H

#include "common.h"

typedef struct {
  char *name;
  DataType type;
  int struct_id;
  int array_depth;
  int offset;
} Symbol;

typedef struct {
  char *name;
  int offset;
  DataType type;
  int struct_id;
  int array_depth;

  // For interfaces
  int arg_count;
  int ret_count;
  DataType ret_types[8];
  int ret_sids[8];
  int ret_ads[8];
} Field;

typedef struct {
  char *name;
  Field fields[64];
  int field_count;
  int size;
  int is_interface;
} StructInfo;

typedef struct {
  char *name;
  uint32_t addr;
  DataType ret_types[8];
  int ret_struct_ids[8];
  int ret_array_depths[8];
  int ret_count;
  int arg_count;
  // --- Phase B: argument types for type checking at call sites ---
  DataType arg_types[32]; // up to 32 tracked arg types per function
  int arg_struct_ids[32];
  int arg_array_depths[32];
} FuncInfo;

typedef struct {
  char *name;
  int value;
} EnumValue;

typedef struct {
  char *name;
  EnumValue values[64];
  int value_count;
} EnumInfo;

extern Symbol *globals;
extern int global_count;
extern Symbol *locals;
extern int local_count;
extern int local_cap;
extern FuncInfo *funcs;
extern int func_count;
extern StructInfo *structs;
extern int struct_count;
extern EnumInfo *enums;
extern int enum_count;

void symbols_init();
int find_local(const char *name);
int find_global(const char *name);
int find_func(const char *name);
int find_struct(const char *name);
void add_local(char *name, DataType type, int sid, int ad);
void add_global(char *name, DataType type, int sid, int ad);
int add_struct(char *name);
int add_func(char *name, uint32_t addr);

int add_enum(char *name);
void add_enum_value(int eid, char *name, int value);
int find_enum(const char *name);
int get_enum_value(int eid, const char *name);

#endif
