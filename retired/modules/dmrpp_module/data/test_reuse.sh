#!/bin/bash


# ./keepalive2 [-?][-D][-h][-d][-r][-k] 
#  -?|h  Print this message.
# [-o <output_file_base>] 
# [-u <url>] 
# [-m <max_easy_handles>] 
# [-s <total_size>] 
# [-c <chunk_count>] 
#   -D   Dryrun.
#   -d   Produce debugging output.
#   -r   Reuse CuRL Easy handles.
#   -k   Utilize Keep-Alive





function run_keep_alive() {
    log_file=$1
    total_size=$2;
    chunk_count=$3;
    max_handles=$4;
    reuse_handles=$5
    pthreads=$6;
    
    if [[ -z "${pthreads// }" ]] || [ ! -n $pthreads ] 
    then 
        pthreads=""
    else 
        pthreads="-t $pthreads"
    fi

    shard_base="scratch/foo";
    rm -f $shard_base*;
    
    params="-o $shard_base -m $max_handles -c $chunk_count  -s $total_size $pthreads $reuse_handles"
    echo "keepalive2 params: $params"
    echo "keepalive2 log_file: $log_file"
            
    time -p { ./keepalive2 $params >> $log_file 2>&1 ; }

}


function runz_it() {
    t_size=$1
    c_count=$2
    for m_handles in  2 4 8 16 # 1 # (I started with 1 but omg was it slow)
    do
        if [ $m_handles -gt $c_count ]
        then
            echo "More curl handles than chunks, skipping edge case"
        else 
            for i in {1..10}
            do
                log_tag="_s"$t_size"_c"$c_count"_m"$m_handles
                reuse_log_file="scratch/keepalive2"$log_tag"_rk.log"
                #rm -f $reuse_log_file
                echo "reuse_handles with_keepalive lap: $i log: $reuse_log_file"
                run_keep_alive $reuse_log_file $t_size $c_count $m_handles "-r" "" >> $reuse_log_file 2>&1

                for p_threads in  2 4 8 
                do
                    pthreads_reuse_log_file="scratch/keepalive2"$log_tag"_p"$p_threads"_rk.log"
            
                    #rm -f $pthreads_reuse_log_file 
                    echo "pthreads_and_reuse_handles_with_keepalive lap: $i log: $pthreads_reuse_log_file "
                    run_keep_alive "$pthreads_reuse_log_file" "$t_size" "$c_count" "$m_handles" "-r" "$p_threads" >> $pthreads_reuse_log_file 2>&1

                done
            done
        fi
    done
    
    
}


# MERRA2
total_size=112966523 # 107.7MB

for chunk_size in  1444 2304 52416 831744 # 4
do
   let chunk_count=total_size/chunk_size;
   echo "Running MERRA2 test. total_size: $total_size  chunk_size: $chunk_size  chunk_count: $chunk_count"
   runz_it "$total_size" "$chunk_count"
done



# AIRS
total_size=317869561 # 302.0MB

for chunk_size in 12 16 48 720 1440 259200 777600 1036800 3110400 6220800
do
   let chunk_count=total_size/chunk_size;
   echo "Running AIRS test. total_size: $total_size  chunk_size: $chunk_size  chunk_count: $chunk_count"
   runz_it "$total_size" "$chunk_count"
done




