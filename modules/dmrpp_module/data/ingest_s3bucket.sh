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
dataset_regex_match="^.*\\.(h5|he5|nc4)(\\.bz2|\\.gz|\\.Z)?$";
s3_service_endpoint="https://s3.amazonaws.com/"
s3_bucket_name="cloudydap/"

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
find_files=

while getopts "h?vVrs:b:d:t:r:f" opt; do
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
    r)
        just_dmr="yes"
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
        
        
    esac
done

shift $((OPTIND-1))

[ "$1" = "--" ] && shift
#################################################################################




#################################################################################
#
# mk_file_list() 
#
# Creates a list of all the files in or below data_root using find. The list is 
# written to ALL_FILES. Once created the regex (either default or supplied with 
# the -r option) is applied to the list and the matching files collected as 
# DATA_FILES 
# . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
function mk_file_list_from_s3() {

    echo "Retrieving files from S3. ALL_FILES: ${ALL_FILES}";
    time -p aws s3 ls --recursive ${s3_bucket_name} > ${ALL_FILES};
    
    echo "Locating DATA_FILES: ${DATA_FILES}";
    time -p grep -E -e "${dataset_regex_match}" ${ALL_FILES} > ${DATA_FILES};
    
    dataset_count=`cat ${DATA_FILES} | wc -l`;
    echo "Found ${dataset_count} suitable data files in ${data_root}"
}
#################################################################################




#################################################################################
#
# mk_file_list_s3() 
#
# Creates a list of all the files in or below data_root using find. The list is 
# written to ALL_FILES. Once created the regex (either default or supplied with 
# the -r option) is applied to the list and the matching files collected as 
# DATA_FILES 
# . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
function mk_file_list_from_filesystem() {

    echo "Retrieving files from local file system. ALL_FILES: ${ALL_FILES}";
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
          
        if test -n "$verbose"
        then
            echo "data_root:    ${data_root}";
            echo "dataset_file: ${dataset_file}";
            echo "relative_filename: ${relative_filename}";
        fi

        s3_url="${s3_service_endpoint}${s3_bucket_name}${relative_filename}";
        if test -n "$very_verbose"
        then
            echo "s3_url:       ${s3_url}";
        fi
        
        target_file="${target_dir}/${relative_filename}.dmrpp";
        if test -n "$very_verbose"
        then
            echo "target_file:  ${target_file}";
        fi
        mkdir -p `dirname ${target_file}`
        
        ./get_dmrpp.sh -V -u "${s3_url}" -d "${data_root}" -o "${target_file}" "${relative_filename}";
        
    done

}
#################################################################################




#################################################################################
#
# main() 
# 
# . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
echo "${0} START: "`date`;

if test -n ${find_local_files}
then
    mk_file_list_from_filesystem;
elif test -n ${find_s3_files}
then
    mk_file_list_from_s3;
fi 

mk_dmrpp;

echo "${0} END:   "`date`;
#################################################################################

