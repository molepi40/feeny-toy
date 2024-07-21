#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<assert.h>
#include "../utils/utils.h"
#include "./ast.h"
#include "./bytecode.h"
#include "./pool.h"
#include "./compiler.h"


ByteIns* byte_drop_ins;
ByteIns* byte_array_ins;
ByteIns* byte_return_ins;
int label_index;
const char* this_str = "this";

int alloc_label_index() {
  return label_index++;
}

Program* compile (ScopeStmt* stmt) {
  // printf("Compiling Program:\n");
  // print_scopestmt(stmt);
  // printf("\n");
  // exit(-1);

  // create program, pool, global table and frame, involving some global resource
  Program* program = make_program();
  Pool* gp = make_pool();
  GlobalTable* gt = make_global_table();
  Frame* gf = make_frame(NULL);
  byte_drop_ins = make_drop_ins();
  byte_array_ins = make_array_ins();
  byte_return_ins = make_return_ins();
  label_index = 1;
  Vector* code = make_vector();

  // compile stmt
  compile_func_stmt(gp, gt, gf, code, stmt, 1);
  vector_add(code, byte_drop_ins);
  LitIns* lit_ins = make_lit_ins(get_pool_null(gp));
  vector_add(code, lit_ins);
  vector_add(code, byte_return_ins);

  // create entry name
  char* entry_name = alloc_entry(alloc_label_index());
  int entry_index = get_global_method(gt, entry_name);
  while (entry_index >= 0) {
    free(entry_name);
    entry_name = alloc_entry(alloc_label_index());
    entry_index = get_global_method(gt, entry_name);
  }
  int entry_name_index = get_pool_str(gp, entry_name);

  // create entry method value
  MethodValue* entry_val = make_method_value(entry_name_index, 0);
  entry_val->nlocals = 0;
  entry_val->code = code;
  entry_index = add_pool_method(gp, entry_val);

  // set program
  program->entry = entry_index;
  program->slots = gt->slots;
  program->values = gp->pool_vec;

  // delete resources that will not be used in pool, global table and frame
  delete_pool(gp);
  delete_global_table(gt);
  delete_frame(gf);

  return program;
}

int compile_func(Pool* pool, GlobalTable* gt, Frame* frame, ScopeFn* stmt, int is_method) {

  // resolve func name
  int name_index = get_pool_str(pool, stmt->name);

  // create func value
  MethodValue* func_val = make_method_value(name_index, stmt->nargs);

  // global
  if (frame->parent == NULL) {
    add_global_method(gt, stmt->name, name_index);
  }

  // create func frame, for class method the real nargs should be nargs + 1
  Frame* fn_frame = make_frame(frame);
  if (is_method) {
    func_val->nargs++;
    add_frame_var(fn_frame, this_str);
  }
  for (int i = 0; i < stmt->nargs; ++i) {
    add_frame_var(fn_frame, stmt->args[i]);
  }

  // compile its code
  compile_func_stmt(pool, gt, fn_frame, func_val->code, stmt->body, 1);
  vector_add(func_val->code, byte_return_ins);

  // resolve number of locals
  func_val->nlocals = fn_frame->index - func_val->nargs;

  // add to pool
  int func_index = add_pool_method(pool, func_val);

  // add to global slots
  if (frame->parent == NULL) {
    add_global_val(gt, func_index);
  }

  return func_index;
}

void compile_func_stmt(Pool* pool, GlobalTable* gt, Frame* frame, Vector* byte_code, ScopeStmt* stmt, int is_last) {
  switch (stmt->tag) {
    case VAR_STMT: {
      ScopeVar* stmt1 = (ScopeVar*)stmt;
      
      if (frame->parent == NULL) {
        // global variable
        // resolve slot name
        int name_index = get_pool_str(pool, stmt1->name);

        // add to pool and global table
        int var_index = get_pool_var(pool, stmt1->name, 1);
        add_global_val(gt, var_index);
        add_global_var(gt, stmt1->name, name_index);

        compile_exp(pool, gt, frame, byte_code, stmt1->exp, is_last);

        SetGlobalIns* set_global_ins = make_set_global_ins(name_index);

        // set global variable to top value of stack and pop the value
        vector_add(byte_code, set_global_ins);
        // vector_add(byte_code, drop_ins);

      } else {
        // local variable, allocate memory on stack of frame
        int loc = add_frame_var(frame, stmt1->name);
        compile_exp(pool, gt, frame, byte_code, stmt1->exp, is_last);

        // emit set local ins and drop ins
        SetLocalIns* set_local_ins = make_set_local_ins(loc);
        vector_add(byte_code, set_local_ins);
        // vector_add(byte_code, drop_ins);
      }
      if (!is_last) {
        vector_add(byte_code, byte_drop_ins);
      }

      break;
    }

    case FN_STMT: {
      ScopeFn* stmt1 = (ScopeFn*)stmt;
      compile_func(pool, gt, frame, stmt1, 0);
      break;
    }

    case EXP_STMT: {
      ScopeExp* stmt1 = (ScopeExp*)stmt;
      compile_exp(pool, gt, frame, byte_code, stmt1->exp, is_last);
      if (!is_last) {
        vector_add(byte_code, byte_drop_ins);
      }
      break;
    }

    case SEQ_STMT: {
      ScopeSeq* stmt1 = (ScopeSeq*)stmt;
      compile_func_stmt(pool, gt, frame, byte_code, stmt1->a, 0);
      compile_func_stmt(pool, gt, frame, byte_code, stmt1->b, is_last);
      break;
    }
  }
}

void compile_exp(Pool* pool, GlobalTable* gt, Frame* frame, Vector* byte_code, Exp* exp, int is_last) {
  switch (exp->tag) {
    case NULL_EXP: {
      int null_index = get_pool_null(pool);
      // emit lit ins
      LitIns* lit_ins = make_lit_ins(null_index);
      vector_add(byte_code, lit_ins);
      break;
    }

    case INT_EXP: {
      IntExp* exp1 = (IntExp*)exp;
      int int_index = get_pool_int(pool, exp1->value);
      // emit lit ins
      LitIns* lit_ins = make_lit_ins(int_index);
      vector_add(byte_code, lit_ins);
      break;
    }

    case PRINTF_EXP: {
      PrintfExp* exp1 = (PrintfExp*)exp;
      
      // resolve printf args
      for (int i = 0; i < exp1->nexps; ++i) {
        compile_exp(pool, gt, frame, byte_code, exp1->exps[i], is_last);
      }

      // emit printf ins and drop ins
      int fmt_index = get_pool_str(pool, exp1->format);
      int nargs = exp1->nexps;
      PrintfIns* printf_ins = make_printf_ins(fmt_index, nargs);
      vector_add(byte_code, printf_ins);
      // vector_add(byte_code, drop_ins);
      break;
    }

    case ARRAY_EXP: {
      ArrayExp* exp1 = (ArrayExp*)exp;

      // resolve array's length and init
      compile_exp(pool, gt, frame, byte_code, exp1->length, is_last);
      compile_exp(pool, gt, frame, byte_code, exp1->init, is_last);
      
      // emit array ins
      vector_add(byte_code, byte_array_ins);
      break;
    }

    case OBJECT_EXP: {
      ObjectExp* exp1 = (ObjectExp*)exp;

      // resolve its parent
      compile_exp(pool, gt, frame, byte_code, exp1->parent, is_last);

      ClassValue* class_val = make_class_value();

      // resolve its slots
      for (int i = 0; i < exp1->nslots; ++i) {
        switch (exp1->slots[i]->tag) {
          case VAR_STMT: {
            SlotVar* stmt = (SlotVar*)exp1->slots[i];
            int slot_index = get_pool_var(pool, stmt->name, 1);
            add_class_value_slot(class_val, slot_index);
            compile_exp(pool, gt, frame, byte_code, stmt->exp, 0);
            break;
          }

          case FN_STMT: {
            SlotMethod* stmt = (SlotMethod*)exp1->slots[i];
            int method_index = compile_func(pool, gt, frame, stmt, 1);
            add_class_value_slot(class_val, method_index);
            break;
          }
        }
      }
      int class_index = add_pool_class(pool, class_val);

      // emit object ins
      ObjectIns* object_ins = make_object_ins(class_index);
      vector_add(byte_code, object_ins);
      break;
    }

    case SLOT_EXP: {
      SlotExp* exp1 = (SlotExp*)exp;

      // resolve object
      compile_exp(pool, gt, frame, byte_code, exp1->exp, is_last);

      // get name index
      int name_index = get_pool_str(pool, exp1->name);

      // emit slot ins
      SlotIns* slot_ins = make_slot_ins(name_index);
      vector_add(byte_code, slot_ins);
      break;
    }

    case SET_SLOT_EXP: {
      SetSlotExp* exp1 = (SetSlotExp*)exp;

      // resolve object
      compile_exp(pool, gt, frame, byte_code, exp1->exp, is_last);

      // resolve value
      compile_exp(pool, gt, frame, byte_code, exp1->value, is_last);

      // get name index
      int name_index = get_pool_str(pool, exp1->name);

      // emit set slot ins
      SetSlotIns* set_slot_ins = make_set_slot_ins(name_index);
      vector_add(byte_code, set_slot_ins); 
      //vector_add(byte_code, drop_ins);
      break;
    }

    case CALL_SLOT_EXP: {
      CallSlotExp* exp1 = (CallSlotExp*)exp;

      // resolve object
      compile_exp(pool, gt, frame, byte_code, exp1->exp, 0);

      // resolve args
      for (int i = 0; i < exp1->nargs; ++i) {
        compile_exp(pool, gt, frame, byte_code, exp1->args[i], 0);
      }

      // get name index
      int name_index = get_pool_str(pool, exp1->name);

      // emit call slot ins
      CallSlotIns* call_slot_ins = make_call_slot_ins(name_index, exp1->nargs + 1);
      vector_add(byte_code, call_slot_ins);
      break;
    }

    case CALL_EXP: {
      CallExp* exp1 = (CallExp*)exp;

      // resolve args
      for (int i = 0; i < exp1->nargs; ++i) {
        compile_exp(pool, gt, frame, byte_code, exp1->args[i], 0);
      }

      // get name index
      int name_index = get_global_method(gt, exp1->name);

      // emit call ins
      CallIns* call_ins = make_call_ins(name_index, exp1->nargs);
      vector_add(byte_code, call_ins);
      break;
    }

    case SET_EXP: {
      SetExp* exp1 = (SetExp*)exp;

      // resolve value
      compile_exp(pool, gt, frame, byte_code, exp1->exp, is_last);

      // get var
      int var_index = get_frame_var(frame, exp1->name);
      if (var_index >= 0) {
        SetLocalIns* set_local_ins = make_set_local_ins(var_index);
        vector_add(byte_code, set_local_ins);
      } else {
        int name_index = get_global_var(gt, exp1->name);
        DEBUG_INFO(name_index < 0, "resolving reference fail");
        SetGlobalIns* set_global_ins = make_set_global_ins(name_index);
        vector_add(byte_code, set_global_ins);
      }

      // emit drop ins
      // vector_add(byte_code, drop_ins);
      break;
    }

    case IF_EXP: {
      IfExp* exp1 = (IfExp*)exp;

      // set conseq, end
      char* conseq_name = alloc_label(CONSEQ, alloc_label_index());
      char* end_name = alloc_label(END, alloc_label_index());
      int conseq_name_index = get_pool_str(pool, conseq_name);
      int end_name_index = get_pool_str(pool, end_name);

      // compile pred exp and set branch ins
      compile_exp(pool, gt, frame, byte_code, exp1->pred, 0);
      BranchIns* branch_ins = make_branch_ins(conseq_name_index);
      vector_add(byte_code, branch_ins);

      // compile alt exp and set goto ins pointing to end
      compile_func_stmt(pool, gt, frame, byte_code, exp1->alt, 1);
      GotoIns* goto_ins = make_goto_ins(end_name_index);
      vector_add(byte_code, goto_ins);

      // set conseq label
      LabelIns* label_ins = make_label_ins(conseq_name_index);
      vector_add(byte_code, label_ins);

      // compile conseq exp and set end label
      compile_func_stmt(pool, gt, frame, byte_code, exp1->conseq, 1);
      label_ins = make_label_ins(end_name_index);
      vector_add(byte_code, label_ins);
      break;
    }

    case WHILE_EXP: {
      WhileExp* exp1 = (WhileExp*)exp;

      // set test, loop, end
      char* test_name = alloc_label(TEST, alloc_label_index());
      char* loop_name = alloc_label(LOOP, alloc_label_index());
      char* end_name = alloc_label(END, alloc_label_index());
      int test_name_index = get_pool_str(pool, test_name);
      int loop_name_index = get_pool_str(pool, loop_name);
      int end_name_index = get_pool_str(pool, end_name);

      // set goto label. put test at end
      GotoIns* goto_ins = make_goto_ins(test_name_index);
      vector_add(byte_code, goto_ins);

      // set loop label
      LabelIns* label_ins = make_label_ins(loop_name_index);
      vector_add(byte_code, label_ins);
      compile_func_stmt(pool, gt, frame, byte_code, exp1->body, 0);

      // set test label
      label_ins = make_label_ins(test_name_index);
      vector_add(byte_code, label_ins);
      compile_exp(pool, gt, frame, byte_code, exp1->pred, 0);

      // set branch ins
      BranchIns* branch_ins = make_branch_ins(loop_name_index);
      vector_add(byte_code, branch_ins);

      // at the end of a while exp, push null object to stack
      LitIns* lit_null_is = make_lit_ins(get_pool_null(pool));
      vector_add(byte_code, lit_null_is);
      break;
    }

    case REF_EXP: {
      RefExp* exp1 = (RefExp*)exp;

      // get ref
      int var_index = get_frame_var(frame, exp1->name);
      if (var_index >= 0) {
        GetLocalIns* get_local_ins = make_get_local_ins(var_index);
        vector_add(byte_code, get_local_ins);
      } else {
        int name_index = get_global_var(gt, exp1->name);
        DEBUG_INFO(name_index < 0, "resolving reference fail");
        GetGlobalIns* get_global_ins = make_get_global_ins(name_index);
        vector_add(byte_code, get_global_ins);
      }
      break;
    }
  }
}

