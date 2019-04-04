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
s3_bucket_base_url="https://s3.amazonaws.com/cloudydap/"

#target_dir=".";

ALL_FILES=$(mktemp -t s3ingest_all_files_XXXX);
DATA_FILES=$(mktemp -t s3ingest_data_files_XXXX);  

ALL_FILES="./all_files.txt";
DATA_FILES="./data_files.txt";

show_usage() {
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
 -u: Base URL for the HTTP datastore.
 -d: The filesystem base for the data.
 -r: The dataset match regex used to screen the base filesystem 
     for datasets.

 Limitations: 
 * The pathanme to the hdf5 file must be relative from the
   directory where this command was run; absolute paths won't work. 
 * The build_dmrpp command must be in the CWD. 
 * The bes conf template has to build by hand. jhrg 5/11/18
EOF
}


OPTIND=1        # Reset in case getopts has been used previously in this shell

verbose=
very_verbose=
just_dmr=
dmrpp_url=

while getopts "h?vVru:d:t:r:" opt; do
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
    u)
        s3_bucket_base_url="$OPTARG"
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
        
        
        
    esac
done

shift $((OPTIND-1))

[ "$1" = "--" ] && shift







function mk_file_list() {
	echo "Retrieving ALL_FILES: ${ALL_FILES}";
    time find ${data_root} -type f > ${ALL_FILES};
 	echo "Locating DATA_FILES: ${DATA_FILES}";
    time grep -E -e "${dataset_regex_match}" ${ALL_FILES} > ${DATA_FILES};
    dataset_count=`cat ${DATA_FILES} | wc -l`;
    echo "Found ${dataset_count} suitable data files in ${data_root}"
}

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

        s3_url="${s3_bucket_base_url}${relative_filename}";
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
        
        ./get_dmrpp.sh -V -u ${s3_url} -d ${data_root} ${relative_filename} > "${target_file}";
        
    done

}

# mk_file_list;
mk_dmrpp;

