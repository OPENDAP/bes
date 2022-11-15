#!/bin/bash
#

./testsuite -v 1 &
p1=$!

./testsuite -v 2 &
p2=$!

# collect the exit status of each process
wait $p1
s1=$?
wait $p2
s2=$?

echo "Process $p1: $s1"
echo "Process $p2: $s2"
