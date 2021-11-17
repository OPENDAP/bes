#!/bin/bash

cp valgrind_nasa_cfdmr_test.sh ..
cp valgrind_nasa_dap4_test.sh ..
cp valgrind_test.sh ..
cd ..
./valgrind_test.sh >& val_test.out 
./valgrind_nasa_cfdmr_test.sh >& val_nasa_cfdmr.out 
./valgrind_nasa_dap4_test.sh >& val_nasa_dap4.out 
grep "definitely lost: " val_test.out >val_test_mem_chk.out
grep "definitely lost: " val_nasa_cfdmr.out >val_nasa_cfdmr_mem_chk.out
grep "definitely lost: " val_nasa_dap4.out >val_nasa_dap4_mem_chk.out

while read line; do
   #echo $line
   if [[ $line != *"definitely lost: 0 bytes in 0 blocks"* ]]; then
        echo "general test memory leaks"
        echo $line
        exit
   fi
done<val_test_mem_chk.out

while read line; do

   #echo $line
   if [[ $line != *"definitely lost: 0 bytes in 0 blocks"* ]]; then
        echo "NASA CF DMR  memory leaks, no leak for the general test"
        echo $line
        exit
   fi

done<val_nasa_cfdmr_mem_chk.out

while read line; do

   #echo $line
   if [[ $line != *"definitely lost: 0 bytes in 0 blocks"* ]]; then
        echo "NASA default DAP4 memory leaks, no leak for the general test and NASA CF DMR"
        echo $line
        exit
   fi

done<val_nasa_dap4_mem_chk.out


/usr/bin/rm -rf val_test.out 
/usr/bin/rm -rf val_nasa_cfdmr.out 
/usr/bin/rm -rf val_nasa_dap4.out 
/usr/bin/rm -rf val_test_mem_chk.out 
/usr/bin/rm -rf val_nasa_cfdmr_mem_chk.out 
/usr/bin/rm -rf val_nasa_dap4_mem_chk.out 
/usr/bin/rm -rf test.nc
/usr/bin/rm -rf nasa_cfdmr_test.nc
/usr/bin/rm -rf nasa_dap4_default_test.nc
/usr/bin/rm -rf valgrind_nasa_cfdmr_test.sh
/usr/bin/rm -rf valgrind_nasa_dap4_test.sh
/usr/bin/rm -rf valgrind_test.sh
cd tests

echo "no memory leak is found for all the tests"
