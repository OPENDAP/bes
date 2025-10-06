#!/bin/bash


basic_stats()
{
    echo "$1" |
        awk '
        {
            #print "Value: "$1;
            if(NR==1){
                max=$1;
                min=$1;
                #print "initialized max: "max" min: "min;
            }
            sum+=$1; 
            sumsq+=$1*$1
            if(max<$1){
                max=$1;
                #print "********** Changed max to "max;
            }
            if(min>$1){
                min=$1;
                #print "********** Changed min to "min;
            }
        }
        END{ 
            mean=sum/NR; 
            stdev=sqrt(sumsq/NR - (sum/NR)**2); 
            # printf("n=%3d, min=%8.2f,  mean=%8.2f +/-%6.2f,  max=%8.2f",NR,min,mean,stdev,max);
            printf("%3d,%8.2f,%8.2f,%6.2f,%8.2f",NR,min,mean,stdev,max);
        }' -
}



echo "\"log file\", \"total transfer size\", \
\"shard count\", \"shard size\", \
\" multi handle count\", \"reuse and keepalive\", \
\"n\", \"min\", \"mean\", \"stdev\", \"max\"";


for file in "$@"
do
    echo $file | awk '{ 
        n=split($0,fields,"_"); 
        total_size = fields[2];
        gsub("s","",total_size);
        shard_count = fields[3];
        gsub("c","",shard_count);
        multi_handle_count = fields[4];
        gsub("m","",multi_handle_count);
        shard_size=total_size/shard_count;
        if(n>4 && index(fields[5],"rk")==1){
            reuse_n_keepalive = "true";
        }
        else {
            reuse_n_keepalive = "false";
        }
        printf("%50s,%10d,%6d,%10d,%3d,%5s, ",
            $0, 
            total_size, 
            shard_count, 
            shard_size,
            multi_handle_count,
            reuse_n_keepalive);
    }' - ;         
    vals=`grep real $file | awk '{print $2}' -`;
    #stats "$vals";    
    echo  "$( basic_stats "$vals" )";    

done
