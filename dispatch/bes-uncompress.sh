#!/bin/sh

infile=${1}
cacheDir=${2}

# Was a file passed to compress?
if [ "${infile}" = "" ]
then
    echo -n "No file is specified to compress"
    exit 1
fi

# Does the file to compress exist?
if [ ! -f ${infile} ]
then
    echo -n "The compressed file ${infile} does not exist"
    exit 1
fi

# If the cacheDir is not specified, then default to /tmp
if [ "${cacheDir}" = "" ]
then
    cacheDir="/tmp"
fi

# does the cache directory exists and is it writable
if [ ! -w ${cacheDir} ]
then
    echo -n "The cache directory ${cacheDir} does not exist"
    exit 1
fi

# create the uncompressed file name by removing any leading slashes,
# converting the rest of the slashes to pound signs, and removing the file
# exension (.gz, .bz2, Z, etc...)
file=`echo ${infile} | sed 's/^\///' | sed 's/\//#/g' | sed 's/\(.*\)\..*$/\1/g'`

# What is the extensioon of the compressed file? (gz, bz2, Z)
ext=`echo ${infile} | sed 's/.*\.\(.*\)$/\1/'`

# Build the full path to the uncompressed file using the cache directory and
# the mangled file name
cache_file="${cacheDir}/bes_cache#${file}"

# If the cached file already exists, then just echo the name of the cached
# file and exit
if [ -f ${cache_file} ]
then
    echo -n "${cache_file}"
    exit 0
fi

# determine the uncompression script to use
case ${ext} in
    gz)
	script='gzip'
	;;
    bz2)
	script='bzip2'
	;;
    Z)
	script='gzip'
	;;
esac

# Uncompress the file
${script} -d -c ${infile} > ${cache_file}
stat=$?
if [ $stat != 0 ]
then
    echo -n "Failed to uncomress the file ${infile} to ${cache_file}"
    exit 1
fi

# Success
echo -n "${cache_file}"
exit 0

