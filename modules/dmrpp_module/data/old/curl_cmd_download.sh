#!/bin/bash
#
# Download a large file using curl and record the results in a text file.
# The result is a CSV text file that lists the URL and the real, user and 
# system times used by curl to dereference the URL. 
#
# Dump the output to a file and open in a spreadsheet.
# 
# $1 URL to download
# $2 number of times to replicate the download
# $3 optional; save the output to the named file

url=$1
reps=$2
out=$3

# Print output file header
echo "URL,Real,User,System"

for ((i = 0; i < $reps; ++i))
do
    if test -n "$out"
    then
	(time -p curl -o $out -s $url) 2> tmp
    else
	(time -p curl -s $url > /dev/null) 2> tmp
    fi

    awk -v url="$url" '/^real.*$/ {r = $2} 
                       /^user.*$/ {u = $2} 
                       /^sys.*$/ {s = $2} 
                       END {print url "," r "," u "," s}' tmp
done

rm tmp
