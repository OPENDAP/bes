#To get valgrind output with suppressed errors

(for instance with command `make`):

## 1. Generate valgrind output
####(test_bes_make.txt)

`valgrind --max-stackframe=2142999 --leak-check=full --show-reachable=yes --error-limit=no --gen-suppressions=all --log-file=test_bes_make.txt make
`

If in the log file you get "Warning: client switching stacks?..." then use `--max-stackframe` option to get clean output.

## 2. Get suppression file 
####(vg-test_bes_make.supp)

`cat ./test_bes_make.txt | ./parse_valgrind_suppressions.sh > vg-test_bes_make.supp
`
## 3. Get valgrind output with suppression file

`valgrind -s --max-stackframe=2142999 --leak-check=full --show-reachable=yes --error-limit=no --suppressions=./vg-test_bes_make.supp --gen-suppressions=all --log-file=test_bes_make.log make`

Suppression file could be edited to remove repeated lines that catches multiple similar errors.


https://wiki.wxwidgets.org/Valgrind_Suppression_File_Howto