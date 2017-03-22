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
    keep_alive=$6
    
if [ 1 ] 
then
    time -p { ./keepalive2 \
        -o scratch/foo \
        -m $max_handles \
        -c $chunk_count \
        -s $total_size \
        $reuse_handles $keep_alive >> $log_file 2>&1 ;
    }
fi
}


t_size=500000000;


for c_count in  1 2 4 8 16 32 64 128 256 512 1024 2048 4096
do
    for m_handles in  1 2 4 8 16 32 64 
    do
        if [ $m_handles -gt $c_count ]
        then
            echo "More curl handles than chunks, skipping edge case"
        else 
            
            log_tag="_s"$t_size"_c"$c_count"_m"$m_handles
            no_reuse_log_file="scratch/keepalive2"$log_tag".log"
            reuse_log_file="scratch/keepalive2"$log_tag"_rk.log"
    
            rm -f $no_reuse_log_file $reuse_log_file
        
            for i in {1..10}
            do
                echo "no_handle_reuse_no_keepalive $log_tag lap $i"
                run_keep_alive $no_reuse_log_file $t_size $c_count $m_handles " " " " >> $no_reuse_log_file 2>&1
                echo "reuse_handles_and_keepalive $log_tag lap $i"
                run_keep_alive $reuse_log_file $t_size $c_count $m_handles "-r" "-k" >> $reuse_log_file 2>&1
            done
        fi
    done
    
    
done



