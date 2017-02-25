#!/bin/sh
shard_count=$1
debug=$2
resource_id="https://s3.amazonaws.com/opendap.test/data/nc/MB2006001_2006001_chla.nc"
resource_id="https://s3.amazonaws.com/cloudydap/airs/AIRS.2015.01.02.L3.RetStd_IR001.v6.0.11.0.G15005190621.nc.h5.dmrpp"
output_file="foo";

curl -s -I $resource_id > $output_file.header;
header_size=`cat $output_file.header | wc -c`
content_length=`cat $output_file.header | grep Content-Length | awk '{printf("%s",$2);}' -`;
echo $content_length;
resource_size=`tr -dc '[[:print:]]' <<< "$content_length"`
echo "resource_size "$resource_size;

#resource_size=140904652;
if [ -n "$resource_size"  ]
then
    ./multiball $debug \
        -u "$resource_id" \
        -c $shard_count \
        -s $resource_size \
        -o $output_file
else
    echo "Bad Size From Resource: "$resource_size  
fi

exit;

time ./multiball -u "https://s3.amazonaws.com/opendap.test/data/nc/MB2006001_2006001_chla.nc" -c 20 -s 140904652 -o foo

time ./multiball -u "https://s3.amazonaws.com/cloudydap/airs/AIRS.2015.01.02.L3.RetStd_IR001.v6.0.11.0.G15005190621.nc.h5"  -s 318843092 -o airs -c 20

time ./multiball -u "https://s3.amazonaws.com/cloudydap/merra2/MERRA2_100.instM_2d_gas_Nx.198802.nc4" -s 2651421 -o merra2 -c 20