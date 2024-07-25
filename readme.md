This repository is the archive of my implementation of labs from [UCB CS294-113: Virtual Machines and Managed Runtimes](http://www.wolczko.com/CS294/). The documents and skeleton code of the whole lab series can be found in its site as well as other materials from this course.

## Implemented Parts

- [x] Feeny bytecode compiler
- [x] Copying garbage collector
- [x] Tagged primitives optimization
- [x] Just-in-time compilation for part instructions
- [x] Primitive inlining & callsite caches & slot caches

## Test

Execute `bash run_tests.sh` to build the source and run tests. Tests output should be in `./output` directory so you must make it first. Or you can just directly execute `make` to compile the source without running tests. There is also `test_time.py` to evaluate the test time.

Option arguments can be set for `cfeeny`. `-c` set to print bytecode, `-i` set to interpret, and `-j` set to launch jit compilation.