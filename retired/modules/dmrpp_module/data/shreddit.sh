#!/bin/sh
#
# Takes a dmrpp file (name from the command line) and the asscoiated data file name
# The script  reads the dmrpp file, extracts the h4:byteStream objects and uses
# tehm to shred the data file into byteStream binary blobs using the 
# offset and nbytes attributes of the h4:byteStream to define what goes
# into the binary file. The md5 attribute is used as the binary blob's name.
#
#
#

#file="../../../../../build/share/hyrax/arch3/merra2/MERRA2_100.instM_2d_asm_Nx.198001.nc4.dmrpp"

dmrpp_file=$1
if [[ -z "${dmrpp_file// }" ]] || [ ! -n $dmrpp_file ] 
then 
    echo "You must supply a relevant dmr++ file name.";
    exit 2;
fi
echo "dmrpp_file: $dmrpp_file"; 

data_file=$2
if [[ -z "${data_file// }" ]] || [ ! -n $data_file ] 
then 
    echo "You must supply a relevant data file name.";
    exit 2;
fi
echo "data_file: $data_file"; 


md5_list=`grep md5 $dmrpp_file | awk '
    {
        for(i=1; i<=NF ;i++){
            if(match($i,"md5=\"")){
                split($i,md5,"\"");
                print md5[2];
            }
        }
    }' - `

echo "Located "`echo $md5_list | wc -w`" md5 tags in $dmrpp_file"
md5_list=`echo $md5_list | sort | uniq`
echo "There are "`echo $md5_list | wc -w`" unique md5 tags in $dmrpp_file"
for md5 in $md5_list 
do
    echo "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -"
    echo "byteStreams: "
    bs_list=`grep $md5 $dmrpp_file`
    for bs in $bs_list
    do
        echo "    "$bs
    done
    echo "md5: "$md5
    offset=`grep $md5 $dmrpp_file | awk '{
        for(i=1; i<=NF ;i++){
            if(match($i,"offset=\"")){
                split($i,offset,"\"");
                print offset[2];
            }
        }   
    }' -  | head -1 `
    echo "offset: "$offset;
    
    nBytes=`grep $md5 $dmrpp_file | awk '{
        for(i=1; i<=NF ;i++){
            if(match($i,"nBytes=\"")){
                split($i,nBytes,"\"");
                print nBytes[2];
            }
        }   
    }' -  | head -1 `
    echo "nBytes: "$nBytes
    
    dd skip=$offset count=$nBytes if=$data_file of=$md5 bs=1

    
done
