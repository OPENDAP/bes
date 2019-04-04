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



ALL_FILES=$(mktemp -t s3ingest_all_files_XXXX);
DATA_FILES=$(mktemp -t s3ingest_data_files_XXXX);  

ALL_FILES="./all_files.txt";
DATA_FILES="./data_files.txt";

function mk_file_list() {
	echo "Retrieving ALL_FILES: ${ALL_FILES}";
    time find $data_root -type f > ${ALL_FILES};
 	echo "Locating DATA_FILES: ${DATA_FILES}";
    time grep -E -e "${dataset_regex_match}" ${ALL_FILES} > ${DATA_FILES};
    dataset_count=`cat ${DATA_FILES} | wc -l`;
    echo "Found ${dataset_count} suitable data files in ${data_root}"
}

function mk_dmrpp() {

	mkdir -p ${target_dir};

    for dataset_file in `cat ${DATA_FILES}`
    do
        echo "dataset_file: ${dataset_file}";
        
        s3_url=${s3_bucket_base};
        echo "s3_url: ${s3_url}";
        
        # get_dmrpp -u ${s3_url} ${dataset_file};
    
    
    done

}

# mk_file_list;
mk_dmrpp;

