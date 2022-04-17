#!/bin/bash

Get_Data() {

   case "$1" in
   -d)
        echo "Using pydap client to access data with the default URL: $4"
        ./write_data_pydap.py >& /dev/null
        ;;
   -o)
        echo "Using pydap client to access data with user-defined URL: $5"
        ./write_data_pydap.py $2  >& /dev/null
        ;;
    esac
    if [ "$?" -ne "0" ]
    then
        case "$1" in
        -d)
            rm -rf $2
            ;;
        -o)    
            rm -rf $3
            ;;
        esac
        echo "FAILED"
        echo "Pydap client failed to obtain data "
    fi

}

Get_DASDDS() {
   case "$1" in
   -d)
        echo "Using pydap client to access DAS and DDS with the default URL: $4"  
        ./write_cf_das_dds.py >& /dev/null
        ;;
   -o)
        echo "Using pydap client to access DAS and DDS with user-defined URL: $5"
        ./write_cf_das_dds.py $2  >& /dev/null
        ;;
    esac
    if [ "$?" -ne "0" ]
    then
        case "$1" in
        -d)
            rm -rf $2
            ;;
        -o)    
            rm -rf $3
            ;;
        esac
        echo "FAILED"
        echo "Pydap client failed to obtain data "
    fi

}

Compare() {
    diff $1 $2
    if [ "$?" -ne "0" ]
    then
        rm -rf $1
        echo "FAILED"
        echo "Pydap client failed" 
        exit 1
    fi
    echo "OK"
    rm -rf $1
}

Test_Data() {
   case "$1" in
   -d)
        Get_Data $1 $2 $3 $4
        Compare $2 $3
        ;;
   -o)
        Get_Data $1 $2 $3 $4 $5
        Compare $3 $4
        ;;
   esac

}

Test_DASDDS() {
   case "$1" in
   -d)
        Get_DASDDS $1 $2 $3 $4
        Compare $2 $3
        ;;
   -o)
        Get_DASDDS $1 $2 $3 $4 $5
        Compare $3 $4
        ;;
   esac

}

#default option 
Test_DASDDS -d grid_1_2d_ddsdas.txt grid_1_2d_ddsdas.txt.exp grid_1_2d.h5
Test_Data -d grid_1_2d_data.txt grid_1_2d_data.txt.exp grid_1_2d.h5

#own-URL, replace the URL with your own URL at the hyrax server
Test_DASDDS -o https://eosdap.hdfgroup.org:8080/opendap/data/hdf5/grid_1_2d.h5 grid_1_2d_ddsdas.txt grid_1_2d_ddsdas.txt.exp grid_1_2d.h5 
Test_DASDDS -o https://eosdap.hdfgroup.org:8080/opendap/data/NASAFILES/hdf5/MLS-Aura_L2GP-BrO_v04-23-c03_2016d302.he5 MLS-Aura_L2GP-BrO_v04-23-c03_2016d302.he_ddsdas.txt MLS-Aura_L2GP-BrO_v04-23-c03_2016d302.he_ddsdas.txt.exp MLS-EOS5
Test_DASDDS -o https://eosdap.hdfgroup.org:8989/opendap/data/test/kent/hdf5_handler_fake/d_int.h5 d_int_default_ddsdas.txt d_int_default_ddsdas.txt.exp default-d_int.h5
echo "All pydap client tests get passed."

# example to obtain data with URL
#./write_data_pydap.py http://guanaco:8080/opendap/data/hdf5/grid_1_2d.h5.dods?temperature[0:1:1][0:1:1]

