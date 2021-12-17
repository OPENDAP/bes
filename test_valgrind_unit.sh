#!/bin/bash
grep "definitely lost: " tt.log >tt.out
while read line; do

   #echo $line
   if [[ $line != *"definitely lost: 0 bytes in 0 blocks"* ]]; then
        echo "memory leaks"
        echo $line
        exit 1
   fi

done<tt.out
/bin/rm -rf tt.out
echo "no memory leak is found"
