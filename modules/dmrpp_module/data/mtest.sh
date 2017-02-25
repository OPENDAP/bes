#!/bin/sh
shard_count=$1
debug=$2
resource_id="https://s3.amazonaws.com/opendap.test/data/nc/MB2006001_2006001_chla.nc"
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
