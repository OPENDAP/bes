#!/bin/bash
#
# 
#
# Dump the output to a file and open in a spreadsheet.
# 
# $1 URL to download
# $2 number of times to replicate the download
# $3 save the output to the named file/path. If a pathname is used, 
#    the directories must exist

one_MB=1048576
ten_MB=10485760
hundred_MB=104857600
five_hundred_MB=524288000
one_GB=1073748824

url=$1
reps=$2
out=$3
count=$4

if test -z "$count"; then count=400; fi

# Print output file header
echo "URL,Size,Threads,KeepAlive,Real,User,System"

threads=1

# Keep-Alive false, then true
for ka in 0 1
do

ka_switch=
if test $ka -eq 1; then ka_switch="-r"; fi

for size in $one_MB $ten_MB $hundred_MB $five_hundred_MB $one_GB 
do

for ((i = 0; i < $reps; ++i))
do
    cmd="./keepalive2 -u $url -s $size -o $out -m 1 -c $count -t $threads $ka_switch"
    (time -p $cmd) 2> tmp

    # Print one line of the result table
    echo -n "$url,$size,$threads,$ka,"
    awk '/^real.*$/ {r = $2} 
         /^user.*$/ {u = $2} 
         /^sys.*$/ {s = $2} 
         END {print r "," u "," s}' tmp
done

done

done

rm tmp
