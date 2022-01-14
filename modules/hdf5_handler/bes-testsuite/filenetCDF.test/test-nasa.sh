#!/bin/sh

#The path BES is installed must be provided explicitly.
if [ -z "$BESPATH" ]; then
    echo Environment variable BESPATH should be set. It should be the BES path the handler is linked with.
    exit
fi

#FNC_NOCLEANUP="yes"

# Use this path to figure out the location of besstandalone, bes.conf and the data.
BESSTANDALONE="$BESPATH/bin/besstandalone"
BESCONF="$BESPATH/etc/bes/bes.conf"
BESH5SHARE="$BESPATH/share/hyrax/data/hdf5"

#1. Remember the current directory
CURDIR=`pwd`
echo $CURDIR

#2. Obtain HDF5 files
cd $BESH5SHARE
SHAREDIR=`pwd`
echo $SHAREDIR
GET=""
command -v  wget > /dev/null && GET="wget -N --retr-symlinks"
if [ -z "$GET" ]; then
  command -v  curl > /dev/null && GET="curl -O"
fi

if [ -z "$GET" ]; then
  echo "Neither wget nor curl found in your system."
  exit
fi

FNC_CP_TEST="fnc_nasa_test"
mkdir $FNC_CP_TEST
cd $FNC_CP_TEST

#NASA files
$GET https://gamma.hdfgroup.org/ftp/pub/outgoing/opendap/data/HDF5/NASA1/GESDISC/3A-MO.GPM.GMI.GRID2014R1.20140601-S000000-E235959.06.V03A.h5
$GET https://gamma.hdfgroup.org/ftp/pub/outgoing/opendap/data/HDF5/NASA1/GESDISC/OMPS-NPP_NMTO3-L3-DAILY_v2.1_2018m0102_2018m0104t012837.h5
$GET https://gamma.hdfgroup.org/ftp/pub/outgoing/opendap/data/HDF5/NASA1/LPDAAC/VNP09A1.A2015257.h29v11.001.2016221164845.h5
$GET https://gamma.hdfgroup.org/ftp/pub/outgoing/opendap/data/HDF5/NASA1/NSIDC/SMAP_L3_SM_P_20150406_R14010_001.h5

#3. Use fileout netcdf modules to generate the netCDF-3 and the netCDF-4
cd $CURDIR
BCMD_DIR="nasa_test" 
cd $BCMD_DIR
echo "BESCMD files to generate netCDF-3 files(Also have the corresponding BESCMD files for netCDF-4 files). "
ls *.nc.bescmd
mkdir $FNC_CP_TEST
#/opt/kent/opendap/bes-dev/opendapbin/bin/besstandalone -c /opt/kent/opendap/bes-dev/opendapbin/etc/bes/bes.conf -i grid_1_2d.nc.bescmd >grid_1_2d.h5.nc
echo "Generate netcdf-3 and netcdf-4 files by using the filenetCDF modules"
for BCMD in *.bescmd
do 
	NCF=${BCMD%.bescmd}
        echo "$NCF"
	$BESSTANDALONE -c $BESCONF -i $BCMD > $FNC_CP_TEST/$NCF
done

echo "Compare ncdump -h output for nc and nc-4 files"
cd $FNC_CP_TEST
NCDUMP="$BESPATH/deps/bin/ncdump"
command -v $NCDUMP >/dev/null
if [ "$?" -ne "0" ]
then 
	echo "ncdump is not provided by the Hyrax dependencies under BESPATH. Won't check ncdump output."
        echo "Abort the program"
        cd ..
       	if [ -z "$FNC_NOCLEANUP" ]; then 
       		rm -rf $FNC_CP_TEST
	fi
	exit 1
fi
for NCF in *.nc
do 
        NCF_NO_SUFFIX=${NCF%.nc}
	$NCDUMP -h $NCF > $NCF_NO_SUFFIX.ndr3
	if [ "$?" -ne "0" ]
	then 
		echo "ncdump FAIL to dump $NCF"
        	echo "Abort the program!"
        	cd ..
		if [ -z "$FNC_NOCLEANUP" ]; then 
        		rm -rf $FNC_CP_TEST
		fi
        	exit 1
	fi
done

for NCF in *.nc4
do 
        NCF_NO_SUFFIX=${NCF%.nc4}
	$NCDUMP -h $NCF > $NCF_NO_SUFFIX.ndr4
	if [ "$?" -ne "0" ]
	then 
		echo "ncdump FAIL to dump $NCF"
        	echo "Abort the program!"
        	cd ..
		if [ -z "$FNC_NOCLEANUP" ]; then 
        		rm -rf $FNC_CP_TEST
		fi
        	exit 1
	fi
done

#ncdump's history attribute records the time the file is generated. We need to remove it for nc file comparison. 
for NCF in *.nc
do 
        NCF_NO_SUFFIX=${NCF%.nc}
	#OMPS-NPP's history attribute contains two lines. We need to remove them for comparsion
	if [[ $NCF_NO_SUFFIX == *"OMPS-NPP"* ]]; then
		tt=$(echo "$tt" | grep -n history $NCF_NO_SUFFIX.ndr3)
        	#echo $tt
        	tt=$(echo "$tt" | cut -d ":" -f1)
        	#echo $tt
                md=d
                sed -i $tt$md $NCF_NO_SUFFIX.ndr3
                sed -i $tt$md $NCF_NO_SUFFIX.ndr3
                sed -i $tt$md $NCF_NO_SUFFIX.ndr4
                sed -i $tt$md $NCF_NO_SUFFIX.ndr4
        fi

	diff -I  ":history = " $NCF_NO_SUFFIX.ndr3 $NCF_NO_SUFFIX.ndr4
	if [ "$?" -ne "0" ]
	then 
		echo "ncdump output comparsion FAIL for  $NCF_NO_SUFFIX"
        	echo "Abort the program"
        	cd ..
        	if [ -z "$FNC_NOCLEANUP" ]; then 
        		rm -rf $FNC_CP_TEST
		fi
        	exit 1
	fi

done
echo "All NASA tests get PASSED"

cd ..
#Remove all the HDF5 files under BES hyrax share directory and the generated NC files
if [ -z "$FNC_NOCLEANUP" ]; then 
  	rm -rf $FNC_CP_TEST
	cd $SHAREDIR
        rm -rf $FNC_CP_TEST 
fi
