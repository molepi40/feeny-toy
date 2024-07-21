#ifndef BYTECODE_H
#define BYTECODE_H

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

typedef enum {
  INT_VAL,
  NULL_VAL,
  STRING_VAL,
  METHOD_VAL,
  SLOT_VAL,
  CLASS_VAL
} ValTag;

typedef enum {
  LABEL_OP,
  LIT_OP,
  PRINTF_OP,
  ARRAY_OP,
  OBJECT_OP,
  SLOT_OP,
  SET_SLOT_OP,
  CALL_SLOT_OP,
  CALL_OP,
  SET_LOCAL_OP,
  GET_LOCAL_OP,
  SET_GLOBAL_OP,
  GET_GLOBAL_OP,
  BRANCH_OP,
  GOTO_OP,
  RETURN_OP,
  DROP_OP
} OpCode;

typedef struct {
  ValTag tag;
} Value;

typedef struct {
  ValTag tag;
  int value;
} IntValue;

typedef struct {
  ValTag tag;
  char* value;
} StringValue;

typedef struct {
  ValTag tag;
  int name;
  int nargs;
  int nlocals;
  Vector* code;
} MethodValue;

typedef struct {
  ValTag tag;
  int name;
} SlotValue;

typedef struct {
  ValTag tag;
  Vector* slots;
} ClassValue;

typedef struct {
  OpCode tag;  
} ByteIns;

typedef struct {
  OpCode tag;
  int name;
} LabelIns;

typedef struct {
  OpCode tag;
  int idx;
} LitIns;

typedef struct {
  OpCode tag;
  int format;
  int arity;
} PrintfIns;

typedef struct {
  OpCode tag;
  int class;
} ObjectIns;

typedef struct {
  OpCode tag;
  int name;
} SlotIns;

typedef struct {
  OpCode tag;
  int name;
} SetSlotIns;

typedef struct {
  OpCode tag;
  int name;
  int arity;
} CallSlotIns;

typedef struct {
  OpCode tag;
  int name;
  int arity;
} CallIns;

typedef struct {
  OpCode tag;
  int idx;
} SetLocalIns;

typedef struct {
  OpCode tag;
  int idx;
} GetLocalIns;

typedef struct {
  OpCode tag;
  int name;
} SetGlobalIns;

typedef struct {
  OpCode tag;
  int name;
} GetGlobalIns;

typedef struct {
  OpCode tag;
  int name;
} BranchIns;

typedef struct {
  OpCode tag;
  int name;
} GotoIns;

typedef struct {
  Vector* values;
  Vector* slots;
  int entry;
} Program;

Program* load_bytecode (char* filename);
void print_ins (ByteIns* ins);
void print_prog (Program* p);

Program* make_program();

Value* make_null_value();
IntValue* make_int_value(int value);
StringValue* make_string_value(char* value);
SlotValue* make_slot_value(int name);
MethodValue* make_method_value(int name, int nargs); // signature. name is index
ClassValue* make_class_value();
void add_class_value_slot(ClassValue* class_val, int slot_index);

LabelIns* make_label_ins(int name);
LitIns* make_lit_ins(int index);
PrintfIns* make_printf_ins(int format, int arity);
ObjectIns* make_object_ins(int class);
SlotIns* make_slot_ins(int name);
SetSlotIns* make_set_slot_ins(int name);
CallSlotIns* make_call_slot_ins(int name, int arity);
CallIns* make_call_ins(int name, int arity);
SetLocalIns* make_set_local_ins(int index);
GetLocalIns* make_get_local_ins(int index);
SetGlobalIns* make_set_global_ins(int name);
GetGlobalIns* make_get_global_ins(int name);
BranchIns* make_branch_ins(int name);
GotoIns* make_goto_ins(int name);
ByteIns* make_drop_ins();
ByteIns* make_array_ins();
ByteIns* make_return_ins();



#endif


