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

function curl_multi_process_get(){
    target_url=$1
    target_output_file=$2
    children=$3;
    remote_size=$4;
    shard_size=`echo "v=$remote_size/$children; v" | bc`;
    echo "shard_size: $shard_size";
    for ((i = 0; i < children; i++)); 
    do
        rbegin=`echo "v=$i; v*=$shard_size; v" | bc`;
        rend=`echo "v=$rbegin; v+=$shard_size; v--; v" | dbcc`;
        range="$rbegin-$rend";
        echo "range: $range";
        curl -s "$target_url" -r $range -o $target_output_file"_$i_shard" & 
        pid=$!
        pids="$pids $pid";
    done   
    echo "Waiting for $pids"; 
    wait $(jobs -p);
}



##################################################
#
# Here are some simple tests to see...
#


# time -p ./multiball -u "https://s3.amazonaws.com/opendap.test/data/nc/MB2006001_2006001_chla.nc" -s 140904652 -o MB2006001_2006001_chla.nc -c 20

#url="https://s3.amazonaws.com/opendap.test/MVI_1803.MOV"; resource_size=1647477620;
url="https://s3.amazonaws.com/opendap.test/data/nc/MB2006001_2006001_chla.nc"; resource_size=140904652;
#url="https://s3.amazonaws.com/opendap.test/data/nc/MB2006001_2006001_chla.nc"; resource_size=140023;
name="mvi_1803"
#############################
#MULTIBALL
for shards in 20 10 5 2 1
do
    file_base=$name"_"$shards;
    rm -f $file_base*
    for rep in {1..10}
    do
        echo "url: $url size: $resource_size shards: $shards  rep: $rep  seconds: \c"
        (time -p ./multiball -u $url -s $resource_size -o $file_base -c $shards 2>> $file_base.log) 2>>$file_base.time
        seconds=`tail -3 $file_base.time | grep real | awk '{print $2;}' -`
        echo $seconds;
        echo "$file_base: shards: $shards rep: $rep seconds: $seconds" >> $file_base.log;
    done
    time_vals=`grep real $file_base.time | awk '{printf("%s + ",$2);}' -`"0.0";
    #echo "time_vals: $time_vals";
    avg=`echo "scale=3; v=$time_vals; v=v/10.0; v" | bc`;
    echo "$file_base:  shards: $shards average_time: $avg" | tee -a $file_base.log
    #exit;
done
#
# CuRL Command Line
#
file_base=$name"_curl_cmdln";
for rep in {1..10}
do
    echo "url: $url CuRL_command_line rep: $rep  seconds: \c"
    (time -p curl -s "$url" -o $file_base) 2>> $file_base.time
    seconds=`tail -3 $file_base.time | grep real | awk '{print $2;}' -`
    echo $seconds;
     echo "$file_base: rep: $rep seconds: $seconds" >> $file_base.log;
done
time_vals=`grep real $file_base.time | awk '{printf("%s + ",$2);}' -`"0.0";
avg=`echo "scale=3; v=$time_vals; v=v/10.0; v" | bc`;
echo "$file_base:  CuRL_command_line average_time: $avg" | tee -a $file_base.log

#
# CuRL Command Line, use HTTP Range GET to shard and background processes
#
for shards in 20 10 5 2 1
do
    file_base=$name"_curl_mproc";
    for rep in {1..10}
    do
        echo "url: $url CuRL_command_line_multi_proc rep: $rep  seconds: \c"
        (time -p curl_multi_process_get $url $file_base $shards $resource_size) 2>> $file_base.time
        seconds=`tail -3 $file_base.time | grep real | awk '{print $2;}' -`
        echo $seconds;
        echo "$file_base: shards: $shards rep: $rep seconds: $seconds" >> $file_base.log;
    done
    time_vals=`grep real $file_base.time | awk '{printf("%s + ",$2);}' -`"0.0";
    avg=`echo "scale=3; v=$time_vals; v=v/10.0; v" | bc`;
    echo "$file_base:  CuRL_command_line_multi_proc shards: $shards avg_time: $avg" | tee -a $file_base.log
done





# time -p ./multiball -u "https://s3.amazonaws.com/cloudydap/airs/AIRS.2015.01.02.L3.RetStd_IR001.v6.0.11.0.G15005190621.nc.h5"  -s 318843092 -o airs -c 20

# time -p ./multiball -u "https://s3.amazonaws.com/cloudydap/merra2/MERRA2_100.instM_2d_gas_Nx.198802.nc4" -s 2651421 -o merra2 -c 20