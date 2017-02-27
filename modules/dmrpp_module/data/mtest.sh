#!/bin/bash



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

#############################
#MULTIBALL
function multiball() {
    
    echo "########################## CuRL multi_perform ##########################"
    test_base=$name"_multi_perform_";
    #echo "test_base: $test_base";
    rm -f "$test_base*";

    for shards in 50 20 10 5 2 1
    do
        file_base=$test_base$shards;
        echo "file_base: $file_base" >> $file_base.log
        reps=10;
        for rep in {1..10}
        do
            #echo -n "."
            cmd="./multiball -u $url -s $resource_size -o $file_base -c $shards"
            echo "COMMAND: $cmd" >> $file_base.log
            (time -p $cmd) 2>> $file_base.log 
            seconds=`tail -3 $file_base.log | grep real | awk '{print $2;}' -`
            echo "CuRL_multi_perform file_base: $file_base size: $resource_size shards: $shards  rep: $rep  seconds: $seconds" >> $file_base.log;
        done
        #echo "";
        # time_vals=`grep real $file_base.log | awk '{printf("%s + ",$2);}' -`"0.0";
        time_vals=`grep real $file_base.log | awk '{printf("%s ",$2);}' -`;
        # echo "time_vals: $time_vals";        
        metrics=`echo $time_vals | awk '{for(i=1;i<=NF;i++){sum += $i; sumsq += ($i)^2;}}END {printf("%f %f \n", sum/NF, sqrt((sumsq-sum^2/NF)/NF))}' -`;        
        # avg=`echo "scale=3; v=$time_vals; v=v/10.0; v" | bc`;
        avg=`echo $metrics | awk '{print $1}' -`;
        stdev=`echo $metrics | awk '{print $2}' -`;

        echo "CuRL_multi_perform file_base: $file_base size: $resource_size shards: $shards reps: $reps average_time: $avg stddev: $stdev" | tee -a $file_base.log
        #exit;
    done

}


#
# CuRL Command Line
#
function cmdln_curl() {
    echo "########################## CuRL Command Line ##########################"
    file_base=$name"_curl_cmdln";
    rm -f "$file_base*"
    reps=10;
    for rep in {1..10}
    do
        #echo -n "."
        cmd="curl -s "$url" -o $file_base"
        echo "COMMAND: $cmd" >> $file_base.log
        (time -p $cmd) 2>> $file_base.log
        seconds=`tail -3 $file_base.log | grep real | awk '{print $2;}' -`
        echo "CuRL_command_line file_base: $file_base: rep: $rep seconds: $seconds" >> $file_base.log;
    done
    #echo "";
    time_vals=`grep real $file_base.log | awk '{printf("%s ",$2);}' -`;
    #echo "time_vals: $time_vals";        
    metrics=`echo $time_vals | awk '{for(i=1;i<=NF;i++){sum += $i; sumsq += ($i)^2;}}END {printf("%f %f \n", sum/NF, sqrt((sumsq-sum^2/NF)/NF))}' -`;        
    avg=`echo $metrics | awk '{print $1}' -`;
    stdev=`echo $metrics | awk '{print $2}' -`;

    echo "CuRL_command_line file_base: $file_base average_time: $avg stddev: $stdev" | tee -a $file_base.log
}



#
# CuRL Command Line, use HTTP Range GET to shard and background processes
#
function multi_process_curl_cmdln() {
    file_base=$name"_curl_mproc";
    rm -f "$file_base*";
    echo "########################## CuRL Command Line Multi Process ##########################" | tee $file_base.log
    for shards in  50 20 10 5 2 1
    do
        echo "SHARDS: $shards ##########################" >> $file_base.log;
        reps=10;
        for rep in {1..10}
        do
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
                    #echo "shard_size: $shard_size";
                    #echo "end_diff:   $end_diff";
                    let rend=$rbegin+$shard_size-1;
                    if [ $rend -ge $resource_size ] 
                    then
                        let rend=$resource_size-1;
                        #echo "------------------ rend: $rend"
                    #else 
                        #echo "rend: $rend"
                    fi
                    range="$rbegin-$rend";
                    #echo "rbegin: $rbegin rend: $rend range: $range";
                    cmd="curl -s "$url" -r $range -o $file_base"_"$i"_"$shards"_shard
                    echo "COMMAND: $cmd" >> $file_base.log
                    $cmd &
                    pid=$!
                    pids="$pids $pid";
                    echo " Launched cmdln CuRL. file_base: $file_base pid: $pid range: $range" >> $file_base.log;
                done   
                echo "CuRL_command_line_multi_proc Waiting for $pids ("`jobs -p`")"; 
                wait `jobs -p`
                
            )  >> $file_base.log  2>&1
            seconds=`tail -3 $file_base.log | grep real | awk '{print $2;}' -`
            # echo "CuRL_command_line_multi_proc file_base: $file_base: shards: $shards rep: $rep seconds: $seconds" | tee -a $file_base.log;
        done
        time_vals=`grep real $file_base.log | awk '{printf("%s ",$2);}' -`;
        # echo "time_vals: $time_vals";        
        metrics=`echo $time_vals | awk '{for(i=1;i<=NF;i++){sum += $i; sumsq += ($i)^2;}}END {printf("%f %f \n", sum/NF, sqrt((sumsq-sum^2/NF)/NF))}' -`;        
        avg=`echo $metrics | awk '{print $1}' -`;
        stdev=`echo $metrics | awk '{print $2}' -`;
        echo "CuRL_command_line_multi_proc  file_base: $file_base resource_size: $resource_size shards: $shards reps: $reps avg_time: $avg stddev: $stdev  url: $url" | tee -a $file_base.log
    done
}

##################################################
#
# Here are some simple tests to see...

url="https://s3.amazonaws.com/opendap.test/MVI_1803.MOV"; resource_size=1647477620;
#url="http://54.152.22.48/MVI_1803.MOV"  resource_size=1647477620;

#resource_size=164747;

#url="https://s3.amazonaws.com/opendap.test/data/nc/MB2006001_2006001_chla.nc"; resource_size=140904652;
#url="https://s3.amazonaws.com/opendap.test/data/nc/MB2006001_2006001_chla.nc"; resource_size=1403;
name="scratch/"`basename $url`
echo "NAME: $name"

rm -f $name*

multiball
#cmdln_curl
#multi_process_curl_cmdln

exit;



#pthre
# time -p ./multiball -u "https://s3.amazonaws.com/opendap.test/data/nc/MB2006001_2006001_chla.nc" -s 140904652 -o MB2006001_2006001_chla.nc -c 20


# time -p ./multiball -u "https://s3.amazonaws.com/cloudydap/airs/AIRS.2015.01.02.L3.RetStd_IR001.v6.0.11.0.G15005190621.nc.h5"  -s 318843092 -o airs -c 20

# time -p ./multiball -u "https://s3.amazonaws.com/cloudydap/merra2/MERRA2_100.instM_2d_gas_Nx.198802.nc4" -s 2651421 -o merra2 -c 20