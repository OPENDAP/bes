#!/bin/bash
#

# clean
rm -rf cache tmp1 tmp2 d1 d2
bes_conf=bes.conf
input=http://test.opendap.org/uat_tester_eval_files/chunked_oneD.h5

# run jobs
./remote_resource_tester $bes_conf $input "d1" 2>&1 > tmp1 &
p1=$!
echo $p1

./remote_resource_tester $bes_conf $input "d2" 2>&1 > tmp2 &
p2=$!
echo $p2

# collect the exit status of each process
wait $p1
s1=$?
wait $p2
s2=$?

echo "Process $p1: $s1"
echo "Process $p2: $s2"

# if test $s1 -ne 0 -o $s2 -ne 0
# then
#     exit 1
# fi
