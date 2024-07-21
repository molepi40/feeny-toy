# Compile
make

# Clean output folder
rm output/*.out
rm output/*.ast

# Run output
function test {
   ./parser -i tests/$1.feeny -oast output/$1.ast
   ./cfeeny output/$1.ast > output/$1.out
}
test hello
test hello2
test hello3
test hello4
test hello5
test hello6
test hello7
test hello8
test hello9
test cplx
test bsearch
test fibonacci
test inheritance
test lists
test vector
test sudoku
test sudoku2
test memory