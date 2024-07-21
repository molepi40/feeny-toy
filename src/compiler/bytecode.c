#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "../utils/utils.h"
#include "./bytecode.h"

//============================================================
//==================== FILE READING ==========================
//============================================================

Vector* read_slots ();
Vector* read_values ();
Vector* read_code ();

static FILE* inputfile;

static char read_byte () {
  int i = fgetc(inputfile);
  if(i < 0) {
    printf("Unexpected end of file.\n");
    exit(-1);
  }
  return (char)i;
}
static int read_short () {
  unsigned char b1 = read_byte();
  unsigned char b2 = read_byte();
  return b1 + (b2 << 8);
}
static int read_int () {
  unsigned char b1 = read_byte();
  unsigned char b2 = read_byte();
  unsigned char b3 = read_byte();
  unsigned char b4 = read_byte();
  return (int)b1 + ((int)b2 << 8) + ((int)b3 << 16) + ((int)b4 << 24);
}
static char* read_string () {
  int len = read_int();
  char* str = malloc(len + 1);
  for(int i=0; i<len; i++)
    str[i] = read_byte();
  str[len] = 0;
  return str;
}

#define RETURN_NEW_INS0()                   \
{                                           \
   ByteIns* o = malloc(sizeof(ByteIns));    \
   o->tag = op;                             \
   return o;                                \
}
#define RETURN_NEW_INS1(TAG, x, xv) \
{                                   \
   TAG* o = malloc(sizeof(TAG));    \
   o->tag = op;                     \
   o->x = xv;                       \
   return (ByteIns*)o;              \
}
#define RETURN_NEW_INS2(TAG, x, xv, y, yv) \
{                                          \
   TAG* o = malloc(sizeof(TAG));           \
   o->tag = op;                            \
   o->x = xv;                              \
   o->y = yv;                              \
   return (ByteIns*)o;                     \
}

ByteIns* read_ins () {
  char op = read_byte();
  switch(op){
  case LABEL_OP:
    RETURN_NEW_INS1(LabelIns,
                    name, read_short());
  case LIT_OP:
    RETURN_NEW_INS1(LitIns,
                    idx, read_short());
  case PRINTF_OP:
    RETURN_NEW_INS2(PrintfIns,
                    format, read_short(),
                    arity, read_byte());
  case ARRAY_OP:
    RETURN_NEW_INS0();
  case OBJECT_OP:
    RETURN_NEW_INS1(ObjectIns,
                    class, read_short());
  case SLOT_OP:
    RETURN_NEW_INS1(SlotIns,
                    name, read_short());
  case SET_SLOT_OP:
    RETURN_NEW_INS1(SetSlotIns,
                    name, read_short());
  case CALL_SLOT_OP:
    RETURN_NEW_INS2(CallSlotIns,
                    name, read_short(),
                    arity, read_byte());
  case CALL_OP:
    RETURN_NEW_INS2(CallIns,
                    name, read_short(),
                    arity, read_byte());
  case SET_LOCAL_OP:
    RETURN_NEW_INS1(SetLocalIns,
                    idx, read_short());
  case GET_LOCAL_OP:
    RETURN_NEW_INS1(GetLocalIns,
                    idx, read_short());
  case SET_GLOBAL_OP:
    RETURN_NEW_INS1(SetGlobalIns,
                    name, read_short());
  case GET_GLOBAL_OP:
    RETURN_NEW_INS1(GetGlobalIns,
                    name, read_short());
  case BRANCH_OP:
    RETURN_NEW_INS1(BranchIns,
                    name, read_short());
  case GOTO_OP:
    RETURN_NEW_INS1(GotoIns,
                    name, read_short());
  case RETURN_OP:
    RETURN_NEW_INS0();
  case DROP_OP:
    RETURN_NEW_INS0();
  default:
    printf("Unrecognized Opcode: %d\n", op);
    exit(-1);
  }
}

#define RETURN_NEW_VAL0() \
{\
   Value* o = malloc(sizeof(Value));\
   o->tag = tag;\
   return o;\
}
#define RETURN_NEW_VAL1(TYPE, x, xv) \
{\
   TYPE* o = malloc(sizeof(TYPE));\
   o->tag = tag;\
   o->x = xv;\
   return (Value*)o;\
}
#define RETURN_NEW_VAL4(TYPE, w, wv, x, xv, y, yv, z, zv) \
{\
   TYPE* o = malloc(sizeof(TYPE));\
   o->tag = tag;\
   o->w = wv;\
   o->x = xv;\
   o->y = yv;\
   o->z = zv;\
   return (Value*)o;\
}
Value* read_value () {
  char tag = read_byte();
  switch(tag){
  case INT_VAL:
    RETURN_NEW_VAL1(IntValue,
                    value, read_int());
  case NULL_VAL:
    RETURN_NEW_VAL0();
  case STRING_VAL:
    RETURN_NEW_VAL1(StringValue,
                    value, read_string());
  case METHOD_VAL:
    RETURN_NEW_VAL4(MethodValue,
                    name, read_short(),
                    nargs, read_byte(),
                    nlocals, read_short(),
                    code, read_code());
  case SLOT_VAL:
    RETURN_NEW_VAL1(SlotValue,
                    name, read_short());
  case CLASS_VAL:
    RETURN_NEW_VAL1(ClassValue,
                    slots, read_slots());
  default:
    printf("Unrecognized value tag: %d\n", tag);
    exit(-1);
  }
}

Vector* read_slots () {
  Vector* v = make_vector();
  int n = read_short();
  for(int i=0; i<n; i++)
    vector_add(v, (void*)read_short());
  return v;
}

Vector* read_values () {
  Vector* v = make_vector();
  int n = read_short();
  for(int i=0; i<n; i++)
    vector_add(v, read_value());
  return v;
}

Vector* read_code () {
  Vector* v = make_vector();
  int n = read_int();
  for(int i=0; i<n; i++)
    vector_add(v, read_ins());
  return v;
}

Program* read_program () {
  Program* p = malloc(sizeof(Program));
  p->values = read_values();
  p->slots = read_slots();
  p->entry = read_short();
  return p;
}

Program* load_bytecode (char* filename) {
  inputfile = fopen(filename, "r");
  if(!inputfile){
    printf("Could not read file %s.\n", filename);
    exit(-1);
  }
  Program* p = read_program();
  fclose(inputfile);
  return p;
}

//============================================================
//===================== PRINTING =============================
//============================================================

void print_value (Value* v);

void print_value (Value* v) {
  switch(v->tag){
  case INT_VAL:{
    IntValue* v2 = (IntValue*)v;
    printf("Int(%d)", v2->value);
    break;
  }
  case NULL_VAL:{
    printf("Null");
    break;
  }
  case STRING_VAL:{
    StringValue* v2 = (StringValue*)v;
    printf("String(");
    print_string(v2->value);
    printf(")");
    break;
  }
  case METHOD_VAL:{
    MethodValue* v2 = (MethodValue*)v;
    printf("Method(#%d, nargs:%d, nlocals:%d) :", v2->name, v2->nargs, v2->nlocals);
    for(int i=0; i<v2->code->size; i++){
      printf("\n      ");
      print_ins(vector_get(v2->code, i));
    }
    break;
  }
  case SLOT_VAL:{
    SlotValue* v2 = (SlotValue*)v;
    printf("Slot(#%d)", v2->name);
    break;
  }
  case CLASS_VAL:{
    ClassValue* v2 = (ClassValue*)v;
    printf("Class(");
    for(int i=0; i<v2->slots->size; i++){
      if(i > 0) printf(", ");
      printf("#%d", (int)vector_get(v2->slots,i));
    }
    printf(")");
    break;
  }
  default:
    printf("Value with unknown tag: %d\n", v->tag);
    exit(-1);
  }
}

void print_ins (ByteIns* ins) {
  switch(ins->tag){
  case LABEL_OP:{
    LabelIns* i = (LabelIns*)ins;
    printf("label #%d", i->name);
    break;
  }
  case LIT_OP:{
    LitIns* i = (LitIns*)ins;
    printf("   lit #%d", i->idx);
    break;
  }
  case PRINTF_OP:{
    PrintfIns* i = (PrintfIns*)ins;
    printf("   printf #%d %d", i->format, i->arity);
    break;
  }
  case ARRAY_OP:{
    printf("   array");
    break;
  }
  case OBJECT_OP:{
    ObjectIns* i = (ObjectIns*)ins;
    printf("   object #%d", i->class);
    break;
  }
  case SLOT_OP:{
    SlotIns* i = (SlotIns*)ins;
    printf("   slot #%d", i->name);
    break;
  }
  case SET_SLOT_OP:{
    SetSlotIns* i = (SetSlotIns*)ins;
    printf("   set-slot #%d", i->name);
    break;
  }
  case CALL_SLOT_OP:{
    CallSlotIns* i = (CallSlotIns*)ins;
    printf("   call-slot #%d %d", i->name, i->arity);
    break;
  }
  case CALL_OP:{
    CallIns* i = (CallIns*)ins;
    printf("   call #%d %d", i->name, i->arity);
    break;
  }
  case SET_LOCAL_OP:{
    SetLocalIns* i = (SetLocalIns*)ins;
    printf("   set local %d", i->idx);
    break;
  }
  case GET_LOCAL_OP:{
    GetLocalIns* i = (GetLocalIns*)ins;
    printf("   get local %d", i->idx);
    break;
  }
  case SET_GLOBAL_OP:{
    SetGlobalIns* i = (SetGlobalIns*)ins;
    printf("   set global #%d", i->name);
    break;
  }
  case GET_GLOBAL_OP:{
    GetGlobalIns* i = (GetGlobalIns*)ins;
    printf("   get global #%d", i->name);
    break;
  }
  case BRANCH_OP:{
    BranchIns* i = (BranchIns*)ins;
    printf("   branch #%d", i->name);
    break;
  }
  case GOTO_OP:{
    GotoIns* i = (GotoIns*)ins;
    printf("   goto #%d", i->name);
    break;
  }
  case RETURN_OP:{
    printf("   return");
    break;
  }
  case DROP_OP:{
    printf("   drop");
    break;
  }
  default:{
    printf("Unknown instruction with tag: %u\n", ins->tag);
    exit(-1);
  }
  }
}

void print_prog (Program* p) {
  printf("Constants :");
  Vector* vs = p->values;
  for(int i=0; i<vs->size; i++){
    printf("\n   #%d: ", i);
    print_value(vector_get(vs, i));
  }
  printf("\nGlobals :");
  for(int i=0; i<p->slots->size; i++)
    printf("\n   #%d", (int)vector_get(p->slots, i));
  printf("\nEntry : #%d", p->entry);
}

Program* make_program() {
  Program* program = (Program*)malloc(sizeof(Program));
  program->slots = program->values = NULL;
  return program;
}

Value* make_null_value() {
  Value* null_value = (Value*)malloc(sizeof(Value));
  null_value->tag = NULL_VAL;
  return null_value;
}

IntValue* make_int_value(int value) {
  IntValue* int_value = (IntValue*)malloc(sizeof(IntValue));
  int_value->tag = INT_VAL;
  int_value->value = value;
  return int_value;
}

StringValue* make_string_value(char* value) {
  StringValue* string_value = (StringValue*)malloc(sizeof(StringValue));
  string_value->tag = STRING_VAL;
  string_value->value = value;
  return string_value;
}

SlotValue* make_slot_value(int name) {
  SlotValue* slot_value = (SlotValue*)malloc(sizeof(SlotValue));
  slot_value->tag = SLOT_VAL;
  slot_value->name = name;
  return slot_value;
}

MethodValue* make_method_value(int name, int nargs) {
  MethodValue* method_value = (MethodValue*)malloc(sizeof(MethodValue));
  method_value->tag = METHOD_VAL;
  method_value->name = name;
  method_value->nargs = nargs;
  method_value->nlocals = -1;
  method_value->code = make_vector();
  return method_value;
}

ClassValue* make_class_value() {
  ClassValue* class_value = (ClassValue*)malloc(sizeof(ClassValue));
  class_value->tag = CLASS_VAL;
  class_value->slots = make_vector();
  return class_value;
}

void add_class_value_slot(ClassValue* class_val, int slot_index) {
  vector_add(class_val->slots, (void*)slot_index);
}

LabelIns* make_label_ins(int name) {
  LabelIns* label_ins = (LabelIns*)malloc(sizeof(LabelIns));
  label_ins->tag = LABEL_OP;
  label_ins->name = name;
  return label_ins;
}

LitIns* make_lit_ins(int index) {
  LitIns* lit_ins = (LitIns*)malloc(sizeof(LitIns));
  lit_ins->tag = LIT_OP;
  lit_ins->idx = index;
  return lit_ins;
}

PrintfIns* make_printf_ins(int format, int arity) {
  PrintfIns* printf_ins = (PrintfIns*)malloc(sizeof(PrintfIns));
  printf_ins->tag = PRINTF_OP;
  printf_ins->format = format;
  printf_ins->arity = arity;
  return printf_ins;
}

ObjectIns* make_object_ins(int class) {
  ObjectIns* object_ins = (ObjectIns*)malloc(sizeof(ObjectIns));
  object_ins->tag = OBJECT_OP;
  object_ins->class = class;
  return object_ins;
}

SlotIns* make_slot_ins(int name) {
  SlotIns* slot_ins = (SlotIns*)malloc(sizeof(SlotIns));
  slot_ins->tag = SLOT_OP;
  slot_ins->name = name;
  return slot_ins;
}

SetSlotIns* make_set_slot_ins(int name) {
  SetSlotIns* set_slot_ins = (SetSlotIns*)malloc(sizeof(SetSlotIns));
  set_slot_ins->tag = SET_SLOT_OP;
  set_slot_ins->name = name;
  return set_slot_ins;
}

CallSlotIns* make_call_slot_ins(int name, int arity) {
  CallSlotIns* call_slot_ins = (CallSlotIns*)malloc(sizeof(CallSlotIns));
  call_slot_ins->tag = CALL_SLOT_OP;
  call_slot_ins->name = name;
  call_slot_ins->arity = arity;
  return call_slot_ins;
}

CallIns* make_call_ins(int name, int arity) {
  CallIns* call_ins = (CallIns*)malloc(sizeof(CallIns));
  call_ins->tag = CALL_OP;
  call_ins->name = name;
  call_ins->arity = arity;
  return call_ins;
}

SetLocalIns* make_set_local_ins(int index) {
  SetLocalIns* set_local_ins = (SetLocalIns*)malloc(sizeof(SetLocalIns));
  set_local_ins->tag = SET_LOCAL_OP;
  set_local_ins->idx = index;
  return set_local_ins;
}

GetLocalIns* make_get_local_ins(int index) {
  GetLocalIns* get_local_ins = (GetLocalIns*)malloc(sizeof(GetGlobalIns));
  get_local_ins->tag = GET_LOCAL_OP;
  get_local_ins->idx = index;
  return get_local_ins;
}

SetGlobalIns* make_set_global_ins(int name) {
  SetGlobalIns* set_global_ins = (SetGlobalIns*)malloc(sizeof(SetGlobalIns));
  set_global_ins->tag = SET_GLOBAL_OP;
  set_global_ins->name = name;
  return set_global_ins;
}

GetGlobalIns* make_get_global_ins(int name) {
  GetGlobalIns* get_global_ins = (GetGlobalIns*)malloc(sizeof(GetGlobalIns));
  get_global_ins->tag = GET_GLOBAL_OP;
  get_global_ins->name = name;
  return get_global_ins;
}

BranchIns* make_branch_ins(int name) {
  BranchIns* branch_ins = (BranchIns*)malloc(sizeof(BranchIns));
  branch_ins->tag = BRANCH_OP;
  branch_ins->name = name;
  return branch_ins;
}

GotoIns* make_goto_ins(int name) {
  GotoIns* goto_ins = (GotoIns*)malloc(sizeof(GotoIns));
  goto_ins->tag = GOTO_OP;
  goto_ins->name = name;
  return goto_ins;
}

ByteIns* make_drop_ins() {
  ByteIns* drop_ins = (ByteIns*)malloc(sizeof(ByteIns));
  drop_ins->tag = DROP_OP;
  return drop_ins;
}

ByteIns* make_array_ins() {
  ByteIns* array_ins = (ByteIns*)malloc(sizeof(ByteIns));
  array_ins->tag = ARRAY_OP;
  return array_ins;
}

ByteIns* make_return_ins() {
  ByteIns* return_ins = (ByteIns*)malloc(sizeof(ByteIns));
  return_ins->tag = RETURN_OP;
  return return_ins;
}