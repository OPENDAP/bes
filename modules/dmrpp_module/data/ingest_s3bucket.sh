#!/bin/bash
# -*- mode: bash; c-basic-offset:4 -*-
#
# This file is part of the Hyrax data server.
#
# Copyright (c) 2019 OPeNDAP, Inc.
# Author: Nathan Potter <ndp@opendap.org>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#
# You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
#
data_root="/home/centos/hyrax/build/share/hyrax/s3/cloudydap";
target_dir="/home/centos/hyrax/build/share/hyrax/dmrpp";

# Should match the value of BES.Catalog.catalog.TypeMatch in the bes.hdf5.cf.conf(.in) files.
# TODO - can we make this read the conf file to get the regex info?
dataset_regex_match="^.*\\.(h5|he5|nc4)(\\.bz2|\\.gz|\\.Z)?$";

s3_service_endpoint="https://s3.amazonaws.com/"
s3_bucket_name="cloudydap"

#target_dir=".";

ALL_FILES=$(mktemp -t s3ingest_all_files_XXXX);
DATA_FILES=$(mktemp -t s3ingest_data_files_XXXX);  

ALL_FILES="./all_files.txt";
DATA_FILES="./data_files.txt";

#################################################################################
#
# show_usage()
#    Print the usage statement to stdout.
#
# . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
function show_usage() {
    cat <<EOF

 Usage: $0 [options] 

 Crawl filesystem and make a DMR++ for every dataset matching the
 default or supplied regex.

 The CWD is set as the BES Data Root directory by default. This utility will
 add an entry to the bes.log specified in the configuration file.
 The DMR++ is built using the DMR as returned by the HDF5 handler,
 using options as set in the bes configuration file found here.
 
 -h: Show help
 -v: Verbose: Print the DMR too
 -V: Very Verbose: print the DMR, the command and the configuration
     file used to build the DMR
 -r: Just print the DMR that will be used to build the DMR++
 -s: The endpoint URL for the S3 datastore. 
     (default: ${s3_service_endpoint})
 -b: The S3 bucket name. 
     (default: ${s3_bucket_name})
 -d: The "local" filesystem base for the data. 
     (default: ${data_root})
 -f: Find all matching data files (if missing then dmrpp generation
     will happen using an exisiting file list, if found.
 -r: The dataset match regex used to screen the base filesystem 
     for datasets. 
     (default: ${dataset_regex_match})
 -l: Make the file list from local files. 
     (default: disabled)
 -a: Make the file list from AWS S3 bucket listing. 
     (default: disabled)


 Limitations: 
 * The pathanme to the hdf5 file must be relative from the
   directory where this command was run; absolute paths won't work. 
 * The build_dmrpp command must be in the CWD. 
 * The bes conf template has to build by hand. jhrg 5/11/18
EOF
}
#################################################################################


#################################################################################
#
# Process the commandline options.
#
# . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
OPTIND=1        # Reset in case getopts has been used previously in this shell

verbose=
very_verbose=
just_dmr=
dmrpp_url=
find_s3_files=
find_local_files=
keep_data_files=

while getopts "h?vVjrs:b:d:t:r:lak" opt; do
    case "$opt" in
    h|\?)
        show_usage
        exit 0
        ;;
    v)
        verbose="-v"
        echo "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -";
        echo "${0} - BEGIN (verbose)";
        ;;
    V)
        very_verbose="yes"
        verbose="-v"
         echo "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -";
        echo "${0} - BEGIN (very_verbose)";
       ;;
    j)
        just_dmr="-r";
        ;;
    s)
        s3_service_endpoint="$OPTARG"
        ;;
    b)
        s3_bucket_name="$OPTARG"
        ;;
    d)
        data_root="$OPTARG"
        ;;
    t)
        target_dir="$OPTARG"
        ;;
    r)
        dataset_regex_match="$OPTARG"
        ;;
    l)
        find_local_files="yes"
        ;;
    a)
        find_s3_files="yes"
        ;;
    k)
        keep_data_files="yes";
        ;;
        
        
    esac
done

shift $((OPTIND-1))

[ "$1" = "--" ] && shift

S3_ALL_FILES="./s3_${s3_bucket_name}_all_files.txt"
S3_DATA_FILES="./s3_${s3_bucket_name}_data_files.txt"

#################################################################################




#################################################################################
#
# mk_file_list() 
#
# Creates a list of all the files in or below data_root the AWS CLI. The output
# of "aws s3 ls --recursive bucket_name" is written to S3_ALL_FILES. Once the 
# list ahs been created the regex (either default or supplied with 
# the -r option) is applied to the list and the matching files collected as 
# S3_DATA_FILES 
# . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
function mk_file_list_from_s3() {

    echo "Retrieving file list from S3."
    echo "s3_bucket_name: ${s3_bucket_name}"
    echo "S3_ALL_FILES: ${S3_ALL_FILES}";
    
    time -p aws s3 ls --recursive "${s3_bucket_name}" > "${S3_ALL_FILES}";
    
    echo "Locating S3_DATA_FILES: ${S3_DATA_FILES}";
    time -p grep -E -e "${dataset_regex_match}" "${S3_ALL_FILES}" > "${S3_DATA_FILES}";
    
    dataset_count=`cat ${S3_DATA_FILES} | wc -l`;
    echo "Found ${dataset_count} suitable data files in s3 bucket: ${s3_bucket_name}"
}
#################################################################################




#################################################################################
#
# mk_dmrpp_from_s3_list() 
#
# Looks at each file name in DATA_FILES and computes the dmr++ for that file.
# 
# . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
function mk_dmrpp_from_s3_list() {

	mkdir -p ${target_dir};

    for relative_filename  in `cat ${S3_DATA_FILES} | awk '{print $4;}' -`
    do        
        s3_url="${s3_service_endpoint}${s3_bucket_name}/${relative_filename}";
        data_root=`pwd`"/data/${s3_bucket_name}";
        data_file="${data_root}/${relative_filename}";
        target_file="${target_dir}/${relative_filename}.dmrpp";       
          
        if test -n "$verbose"
        then
            echo "dataset:           ${dataset}";
            echo "relative_filename: ${relative_filename}";
            echo "s3_url:            ${s3_url}";
            echo "data_file:         ${data_file}";
            echo "target_file:       ${target_file}";
        fi
        
        
        mkdir -p `dirname ${data_file}`;
        time -p aws s3 cp --quiet "s3://${s3_bucket_name}/${relative_filename}" "${data_file}";
        
        mkdir -p `dirname ${target_file}`;
		set -x;
        ./get_dmrpp.sh -V ${just_dmr} -u "${s3_url}" -d "${data_root}" -o "${target_file}" "${relative_filename}";
     
        if test -z "${keep_data_files}"
        then
        	echo "Deleting data file: ${data_file}";
            rm -vf "${data_file}";
        fi
        
 done



}
#################################################################################




#################################################################################
#
# mk_file_list_from_filesystem() 
#
# Creates a list of all the files in or below data_root using find. The list is 
# written to ALL_FILES. Once created the regex (either default or supplied with 
# the -r option) is applied to the list and the matching files collected as 
# DATA_FILES 
# . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
function mk_file_list_from_filesystem() {

    echo "Retrieving file list from ${data_root}. ALL_FILES: ${ALL_FILES}";    
    time -p find ${data_root} -type f > ${ALL_FILES};
    
    echo "Locating DATA_FILES: ${DATA_FILES}";
    time -p grep -E -e "${dataset_regex_match}" ${ALL_FILES} > ${DATA_FILES};
    
    dataset_count=`cat ${DATA_FILES} | wc -l`;
    echo "Found ${dataset_count} suitable data files in ${data_root}"
}
#################################################################################




#################################################################################
#
# mk_dmrpp() 
#
# Looks at each file name in DATA_FILES and computes the dmr++ for that file.
# 
# . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
function mk_dmrpp() {

	mkdir -p ${target_dir};

    for dataset_file in `cat ${DATA_FILES}`
    do
        relative_filename=${dataset_file#"$data_root/"};
        target_file="${target_dir}/${relative_filename}.dmrpp";
          
        if test -n "$verbose"
        then
            echo "data_root:    ${data_root}";
            echo "dataset_file: ${dataset_file}";
            echo "relative_filename: ${relative_filename}";
            echo "target_file: ${target_file}";
        fi

        mkdir -p `dirname ${target_file}`
		set -x;
        ./get_dmrpp.sh -V ${just_dmr}  -u "${s3_url}" -d "${data_root}" -o "${target_file}" "${relative_filename}";
        
    done

}
#################################################################################




#################################################################################
#
# main() 
# 
# . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
echo "${0} START: "`date`;

if test -n "${find_local_files}"
then
    mk_file_list_from_filesystem;
elif test -n "${find_s3_files}"
then
    mk_file_list_from_s3;
fi 

# mk_dmrpp;
mk_dmrpp_from_s3_list

echo "${0} END:   "`date`;
#################################################################################

