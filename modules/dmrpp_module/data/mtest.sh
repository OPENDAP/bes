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


# time -p ./multiball -u "https://s3.amazonaws.com/opendap.test/data/nc/MB2006001_2006001_chla.nc" -s 140904652 -o MB2006001_2006001_chla.nc -c 20




#url="https://s3.amazonaws.com/opendap.test/MVI_1803.MOV"; resource_size=1647477620;
url="https://s3.amazonaws.com/opendap.test/data/nc/MB2006001_2006001_chla.nc"; resource_size=140904652;
url="https://s3.amazonaws.com/opendap.test/data/nc/MB2006001_2006001_chla.nc"; resource_size=1403;
name="mvi_1803"




#
# CuRL Command Line, use HTTP Range GET to shard and background processes
#
for shards in 3 #20 10 5 2 1
do
    file_base=$name"_curl_mproc";
    for rep in {1..2}
    do
       time -p (
            echo `date `" CuRL_command_line_multi_proc proc: $shards rep: $rep url: $url "
            shard_size=`echo "v=$resource_size/$shards; v" | bc`
            if [[ $(($shards % $resource_size)) ]] 
            then
                let shards=$shards-1
                echo "shards: $shards"
            fi
            #echo "shard_size: $shard_size";
            for ((i = 0; i < $shards; i++)); 
            do
                # echo "shard_size; $shard_size i: $i";
                rbegin=`echo "v=$i; v*=$shard_size; v" | bc`;
                rend=`echo "v=$rbegin; v+=$shard_size-1; v" | bc`;
                range="$rbegin-$rend";
                curl -s "$url" -r $range -o $file_base"_"$i"_shard" & 
                pid=$!
                pids="$pids $pid";
                echo "$file_base Launched cmdln CuRL for range: $range  pid: $pid";
            done   
            if [[ $rend -lt $resource_size ]] 
            then
                # echo "shard_size; $shard_size i: $i";
                rbegin=`echo "v=$i; v*=$shard_size; v" | bc`;
                rend=` echo "v=$resource_size-1; v" | bc `;
                range="$rbegin-$rend";
                curl -s "$url" -r $range -o $file_base"_"$i"_shard" & 
                pid=$!
                pids="$pids $pid";
                echo "$file_base Launched cmdln CuRL. pid: $pid range: $range";
            fi
            
            echo "Waiting for $pids ("`jobs -p`")"; 
            wait `jobs -p`
            
        )  >> $file_base.log  2>&1
        seconds=`tail -3 $file_base.log | grep real | awk '{print $2;}' -`
        echo "$file_base: shards: $shards rep: $rep seconds: $seconds" >>  $file_base.log;
   done
    time_vals=`grep real $file_base.log | awk '{printf("%s + ",$2);}' -`"0.0";
    avg=`echo "scale=3; v=$time_vals; v=v/10.0; v" | bc`;
    echo "CuRL_command_line_multi_proc shards: $shards avg_time: $avg resource_size: $resource_size url: $url" | tee -a $file_base.log
done



exit;








#############################
#MULTIBALL
for shards in 20 10 5 2 1
do
    file_base=$name"_"$shards;
    rm -f $file_base*
    for rep in {1..10}
    do
        echo -n "url: $url size: $resource_size shards: $shards  rep: $rep  seconds: \c"
        (time -p ./multiball -u $url -s $resource_size -o $file_base -c $shards) 2>> $file_base.log 
        seconds=`tail -3 $file_base.log | grep real | awk '{print $2;}' -`
        echo $seconds;
        echo "$file_base: shards: $shards rep: $rep seconds: $seconds" >> $file_base.log;
    done
    time_vals=`grep real $file_base.log | awk '{printf("%s + ",$2);}' -`"0.0";
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
    echo -n "url: $url CuRL_command_line rep: $rep  seconds: \c"
    (time -p curl -s "$url" -o $file_base) 2>> $file_base.log
    seconds=`tail -3 $file_base.log | grep real | awk '{print $2;}' -`
    echo $seconds;
    echo "$file_base: rep: $rep seconds: $seconds" >> $file_base.log;
done
time_vals=`grep real $file_base.log | awk '{printf("%s + ",$2);}' -`"0.0";
avg=`echo "scale=3; v=$time_vals; v=v/10.0; v" | bc`;
echo "$file_base:  CuRL_command_line average_time: $avg" | tee -a $file_base.log




# time -p ./multiball -u "https://s3.amazonaws.com/cloudydap/airs/AIRS.2015.01.02.L3.RetStd_IR001.v6.0.11.0.G15005190621.nc.h5"  -s 318843092 -o airs -c 20

# time -p ./multiball -u "https://s3.amazonaws.com/cloudydap/merra2/MERRA2_100.instM_2d_gas_Nx.198802.nc4" -s 2651421 -o merra2 -c 20