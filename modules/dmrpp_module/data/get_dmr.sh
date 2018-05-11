#!/bin/sh

# Usage: get_dmr <hdf5 file>
# Write the DMR for hdf5_file to stdout
# The hdf5 file pathname must be relative to the BES.Catalog.catalog.RootDirectory
# defined in the bes configuration file used by this utility. This utility will
# add an entry to the bes.log specified in the configuration file.

hdf5_file=$1
temp=tmp.bes.cmd

# build the xml file
sed -e "s%[@]hdf5_pathname[@]%$hdf5_file%" < get_dmr_template.bescmd > $temp

# use besstandalone to get the DMR
besstandalone -c bes.hdf5.cf.conf -i $temp

rm $temp
 