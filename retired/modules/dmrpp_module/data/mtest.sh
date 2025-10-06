#!/bin/bash

##################################################
#
# Here are some simple tests to see...

total_reps=10;

# 1.6GB resource held in S3 with access via S3 http
# url="https://s3.amazonaws.com/opendap.test/MVI_1803.MOV"; resource_size=1647477620;

# Here are some smaller files. Note that we can provide different values for
# 'resource_size' and thus work with an artificially smaller file (passing
# a smaller resource size to 'multiball' and using sharded access will cause
# that program to to request fewer bytes). The first line below is the real
# size of the MB...chla.nc file (134MB). Use a smaller number to test the scrip.
#
# If the resource size is smaller than the total file size, the values returned for 
# different tests here may not be comparable (e.g., the 'get the whole file in one
# shot' test will still get all the bytes of the file.
#
url="https://s3.amazonaws.com/opendap.test/data/nc/MB2006001_2006001_chla.nc"; resource_size=140904652;
# url="https://s3.amazonaws.com/opendap.test/data/nc/MB2006001_2006001_chla.nc"; resource_size=100000;

#
# Read the response from an HTTP HEAD request and return the value of the
# Content-Length header.
#
# This is not used, but if you source the script ('source mtest.sh') then
# it can be called from the command line.
#
function get_resource_size() {
    resource_id=$1
    
    http_header_file=$(mktemp -t http_header_XXXXXX)
    if [ $verbose = 1 ] ; then echo "http_header_file: " $http_header_file;  fi

    curl -s -I $resource_id > $http_header_file
    header_size=`cat $http_header_file | wc -c`
    content_length=`cat $http_header_file | grep -i Content-Length | awk '{printf("%s",$2);}' -`
    if [ $verbose = 1 ] ; then echo "content_length: " $content_length;  fi
    
    export resource_size=`tr -dc '[[:print:]]' <<< "$content_length"`
    if [ $verbose = 1 ] ; then 
            echo "resource_size: " $resource_size
    else 
            echo $resource_size
    fi    
}

#
# Call the 'multiball' command line program to read $resource_size bytes from $url.
# This function will put the results in ${name}_multi_perform_${shards}. For each 
# shard value (50, 20, ..., 1) the function will run $total_reps times.
#
# The entire resource will be accessed 8 * $total_reps times
#
# All parameters are accessed via shell variables:
#
# $total_reps - Run each shard-set this many times
# $url - Use this remote resource
# $resource_size - Assume it's this many bytes
# $name - Prefix the log files with this name (use the shard count as a suffix)
# 
function multiball() {
    
    test_base="${name}_multi_perform_";
    rm -f "$test_base*";

    header=`echo "-------------------------------------------------------------------------";
    echo "- Testing CuRL multi_perform"; 
    echo "-" ;
    echo "- Target URL: $url"; 
    echo "- Target Size: $resource_size";  
    echo "-" ;`
    echo "$header"

    for shards in 50 20 10 5 2 1
    do
        file_base=${test_base}${shards}
        echo "$header" >> $file_base.log
        echo "file_base: $file_base" >> $file_base.log
        for ((rep = 0; rep < $total_reps; rep++))
        do
            cmd="./multiball -u $url -s $resource_size -o $file_base -c $shards"
	    echo -n "."
            echo "COMMAND: $cmd" >> $file_base.log
            (time -p $cmd) 2>> $file_base.log
            seconds=`tail -3 $file_base.log | grep real | awk '{print $2;}' -`
            echo "CuRL_multi_perform file_base: $file_base size: $resource_size shards: $shards  rep: $rep  seconds: $seconds" >> $file_base.log
        done
	echo ""
        time_vals=`grep real $file_base.log | awk '{printf("%s ",$2);}' -`
        metrics=`echo $time_vals | awk '{for(i=1;i<=NF;i++){sum += $i; sumsq += ($i)^2;}}END {printf("%f %f \n", sum/NF, sqrt((sumsq-sum^2/NF)/NF))}' -`
        avg=`echo $metrics | awk '{print $1}' -`
        stdev=`echo $metrics | awk '{print $2}' -`
        echo "CuRL_multi_perform shards: $shards reps: $total_reps average_time: $avg stddev: $stdev" | tee -a $file_base.log
    done
}

#
# CuRL Command Line
#
function cmdln_curl() {

    file_base=$name"_curl_cmdln";
    rm -f "$file_base*"

    echo "-------------------------------------------------------------------------" | tee -a $file_base.log
    echo "- Testing CuRL Command Line" | tee -a $file_base.log
    echo "-" | tee -a $file_base.log
    echo "- Target URL: $url" | tee -a  $file_base.log
    echo "- Target Size: $resource_size"  | tee -a $file_base.log
    echo "-" | tee -a $file_base.log


    for ((rep = 0; rep < $total_reps; rep++))
    do
        echo -n "."
        cmd="curl -s $url -o $file_base"
        echo "COMMAND: $cmd" >> $file_base.log
        (time -p $cmd) 2>> $file_base.log
        seconds=`tail -3 $file_base.log | grep real | awk '{print $2;}' -`
        echo "CuRL_command_line file_base: $file_base: rep: $rep seconds: $seconds" >> $file_base.log;
    done
    echo "";
    time_vals=`grep real $file_base.log | awk '{printf("%s ",$2);}' -`;
    metrics=`echo $time_vals | awk '{for(i=1;i<=NF;i++){sum += $i; sumsq += ($i)^2;}}END {printf("%f %f \n", sum/NF, sqrt((sumsq-sum^2/NF)/NF))}' -`;        
    avg=`echo $metrics | awk '{print $1}' -`;
    stdev=`echo $metrics | awk '{print $2}' -`;

    echo "CuRL_command_line shards: 1 reps: $total_reps average_time: $avg stddev: $stdev" | tee -a $file_base.log
}

#
# CuRL Command Line, use HTTP Range GET to shard and background processes
#
function multi_process_curl_cmdln() {
    file_base=$name"_curl_mproc";
    rm -f "$file_base*";

    echo "-------------------------------------------------------------------------" | tee -a $file_base.log
    echo "- Testing CuRL command line using a multi process RANGE GET approach." | tee -a $file_base.log
    echo "-"  | tee -a $file_base.log
    echo "- Target URL: $url"  | tee -a $file_base.log
    echo "- Target Size: $resource_size"  | tee -a $file_base.log
    echo "-"  | tee -a $file_base.log
    # Old loop values:  50 20 10 5 
    for shards in 5 2 1
    do
        echo "SHARDS: $shards ##########################" >> $file_base.log;
        for ((rep = 0; rep < $total_reps; rep++))
        do
	    echo -n "."
            time -p (
                echo "CuRL_command_line_multi_proc proc: $shards rep: $rep url: $url ";
                if [ $shards -gt 1 ]
                then
                    shard_size=`echo "v=$resource_size/($shards-1); v" | bc`;
                else
                    shard_size=$resource_size;
                fi
                    
                echo "shard_size: $shard_size";
                for ((i = 0; i < $shards; i++)); 
                do
                    # echo "shard_size; $shard_size i: $i";
                    rbegin=`echo "v=$i*$shard_size; v" | bc`;
                    let end_diff=$resource_size-$rbegin;
                    # echo "shard_size: $shard_size";
                    # echo "end_diff:   $end_diff";
                    let rend=$rbegin+$shard_size-1;
                    if [ $rend -ge $resource_size ]; then let rend=$resource_size-1; fi
                    range="$rbegin-$rend";
                    # echo "rbegin: $rbegin rend: $rend range: $range";
		    output_file="/dev/null" # ${file_base}_${i}_${shards}_shard
		    cmd="curl -s $url -r $range -o $output_file"
                    echo "COMMAND: $cmd" >> $file_base.log
                    $cmd &
                    pid=$!
                    pids="$pids $pid";
                    echo "Launched cmdln CuRL. file_base: $file_base pid: $pid range: $range";
                done   
                echo "CuRL_command_line_multi_proc Waiting for "`jobs -p`; 
                echo "Wait Started At: "`date`;
                wait `jobs -p`
                
            )  >> $file_base.log  2>&1
            seconds=`tail -3 $file_base.log | grep real | awk '{print $2;}' -`
        done
	echo ""
        time_vals=`grep real $file_base.log | awk '{printf("%s ",$2);}' -`;
        # echo "time_vals: $time_vals";        
        metrics=`echo $time_vals | awk '{for(i=1;i<=NF;i++){sum += $i; sumsq += ($i)^2;}}END {printf("%f %f \n", sum/NF, sqrt((sumsq-sum^2/NF)/NF))}' -`;        
        avg=`echo $metrics | awk '{print $1}' -`;
        stdev=`echo $metrics | awk '{print $2}' -`;
        echo "CuRL_command_line_multi_proc  shards: $shards reps: $total_reps avg_time: $avg stddev: $stdev" | tee -a $file_base.log
    done
}


echo ""
echo "-------------------------------------------------------------------------"
echo "- This is $0"
echo "-"
echo "- Target URL: $url"
echo "- Target size: $resource_size"
name="scratch/"`basename $url`
echo "- LocalNameBase: $name"
echo "- Total Reps: $total_reps"
echo "-"

rm -f $name*

multiball
# cmdln_curl
# multi_process_curl_cmdln

#pthread
# time -p ./multiball -u "https://s3.amazonaws.com/opendap.test/data/nc/MB2006001_2006001_chla.nc" -s 140904652 -o MB2006001_2006001_chla.nc -c 20


# time -p ./multiball -u "https://s3.amazonaws.com/cloudydap/airs/AIRS.2015.01.02.L3.RetStd_IR001.v6.0.11.0.G15005190621.nc.h5"  -s 318843092 -o airs -c 20

# time -p ./multiball -u "https://s3.amazonaws.com/cloudydap/merra2/MERRA2_100.instM_2d_gas_Nx.198802.nc4" -s 2651421 -o merra2 -c 20
