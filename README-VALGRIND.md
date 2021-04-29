#To get valgrind output with suppressed errors

(for instance with command `besstandalone -c tests/bes.conf -i tests/contiguous/coads_climatology.dmr` in dmrpp-module):

## 1. Generate valgrind output
####(vg-dmrpp.txt)

`cd ../hyrax/bes/modules/dmrpp-module`

`valgrind --leak-check=full --show-reachable=yes --error-limit=no --gen-suppressions=all --log-file=vg-dmrpp.txt besstandalone -c tests/bes.conf -i tests/contiguous/coads_climatology.dmr
`

If in the log file you get "Warning: client switching stacks?..." then use `--max-stackframe` option to get clean output.

## 2. Get suppression file 
####(vg-dmrpp.supp)

`cat vg-dmrpp.txt | /home/opendap/hyrax/bes/parse_valgrind_suppressions.sh > vg-dmrpp.supp
`
## 3. Get valgrind output with suppression file

`valgrind -s --leak-check=full --show-reachable=yes --error-limit=no --suppressions=./vg-dmrpp.supp --gen-suppressions=all --log-file=test_bes_make.log besstandalone -c tests/bes.conf -i tests/contiguous/coads_climatology.dmr`

Suppression file could be edited to remove repeated lines that catches multiple similar errors.


https://wiki.wxwidgets.org/Valgrind_Suppression_File_Howto