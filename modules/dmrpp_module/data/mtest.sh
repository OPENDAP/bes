#!/bin/sh



function get_resource_size(){
    resource_id=$1;
    
    http_header_file=$(mktemp -t http_header_XXXXXX)
    if [ $verbose = 1 ] ; then echo "http_header_file: " $http_header_file;  fi

    curl -s -I $resource_id > $http_header_file;
    header_size=`cat $http_header_file | wc -c`
    content_length=`cat $http_header_file | grep -i Content-Length | awk '{printf("%s",$2);}' -`;
    if [ $verbose = 1 ] ; then echo "content_length: " $content_length;  fi
    
    export resource_size=`tr -dc '[[:print:]]' <<< "$content_length"`
    if [ $verbose = 1 ] ; then 
            echo "resource_size: " $resource_size;  
    else 
            echo $resource_size;  
    fi    
}


##################################################
#
# Here are some simple tests to see...
#
time -p ./multiball -u "https://s3.amazonaws.com/opendap.test/data/nc/MB2006001_2006001_chla.nc" -s 140904652 -o foo -c 20

time -p ./multiball -u "https://s3.amazonaws.com/opendap.test/MVI_1803.MOV" -s 1647477620 -o mvi_1803.mov -c 20
time -p ./multiball -u "https://s3.amazonaws.com/opendap.test/MVI_1803.MOV" -s 1647477620 -o mvi_1803.mov -c 10
time -p ./multiball -u "https://s3.amazonaws.com/opendap.test/MVI_1803.MOV" -s 1647477620 -o mvi_1803.mov -c 5
time -p ./multiball -u "https://s3.amazonaws.com/opendap.test/MVI_1803.MOV" -s 1647477620 -o mvi_1803.mov -c 2
time -p ./multiball -u "https://s3.amazonaws.com/opendap.test/MVI_1803.MOV" -s 1647477620 -o mvi_1803.mov -c 1

time -p curl -s "https://s3.amazonaws.com/opendap.test/MVI_1803.MOV" > MVI_1803.MOV


# time -p ./multiball -u "https://s3.amazonaws.com/cloudydap/airs/AIRS.2015.01.02.L3.RetStd_IR001.v6.0.11.0.G15005190621.nc.h5"  -s 318843092 -o airs -c 20

# time -p ./multiball -u "https://s3.amazonaws.com/cloudydap/merra2/MERRA2_100.instM_2d_gas_Nx.198802.nc4" -s 2651421 -o merra2 -c 20