#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "./utils/utils.h"
#include "./compiler/ast.h"
#include "./compiler/compiler.h"
#include "./vm/vm.h"

int main (int argc, char** argvs) {
  //Check number of arguments
  if(argc != 2){
    printf("Expected 1 argument to commandline.\n");
    exit(-1);
  }

  //Read in AST
  char* filename = argvs[1];
  ScopeStmt* stmt = read_ast(filename);

  //Compile to bytecode
  Program* program = compile(stmt);

  // print_prog(program);
  interpret_bc(program);

  //Interpret bytecode
}


