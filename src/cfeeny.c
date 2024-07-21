#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include "./utils/utils.h"
#include "./compiler/ast.h"
#include "./compiler/compiler.h"
#include "./vm/vm.h"

int main (int argc, char** argv) {

  int option;
  int compile_flag = 0, interpret_flag = 0, jit_flag = 0;
  while ((option = getopt(argc, argv, "cij")) != -1) {
    switch (option) {
      case 'c':
        compile_flag = 1;
        break;
      case 'i':
        interpret_flag = 1;
        break;
      case 'j':
        jit_flag = 1;
        break;
    }
  }

  if (optind == argc) {
    printf("missing feeny ast file");
    exit(0);
  }
  
  //Read in AST
  char* filename = argv[optind];
  ScopeStmt* stmt = read_ast(filename);

  //Compile to bytecode
  Program* program = compile(stmt);
  if (compile_flag == 1) {
    print_prog(program);
  }

  //Interpret bytecode
  if (interpret_flag == 1) {
    interpret_bc(program, jit_flag);
  }
}


