#!/bin/sh

# Usage: get_dmr <hdf5 file>
#
# Write the DMR for hdf5_file to stdout
# 
# The CWD is set as the BES Data Root directory. This utility will
# add an entry to the bes.log specified in the configuration file.
#
# Limitations: 
# * The pathanme to the hdf5 file must be relative from the
# directory where this command was run; absolute paths won't work. 
# * The build_dmrpp command must be in the CWD. 
# * The bes conf template has to build by hand. jhrg 5/11/18

hdf5_file=$1

hdf5_root_directory=`pwd`

temp_cmd=tmp.bescmd
temp_dmr=hdf5_file.dmr
temp_conf=bes.conf.tmp

# build the xml file
sed -e "s%[@]hdf5_pathname[@]%$hdf5_file%" < get_dmr_template.bescmd > $temp_cmd

# tweak the bes.conf file so that the Data Root will find the hdf5 file.
sed -e "s%[@]hdf5_root_directory[@]%$hdf5_root_directory%" < bes.hdf5.cf.template.conf > $temp_conf

# use besstandalone to get the DMR
besstandalone -c $temp_conf -i $temp_cmd > $temp_dmr

echo "DMR: "
cat $temp_dmr

./build_dmrpp -f $hdf5_file -r $temp_dmr

# TODO Use trap to ensure these are really removed
rm $temp_cmd $temp_dmr $temp_conf
 