nccopy -k 'classic' http://alpaca:8080/opendap/data/hdf5/hdf5-files/fake-files-for-handler/t_2d_2dll.nc4.h5 t_2d_2dll.nc
nccopy -k 'netCDF-4 classic model' http://alpaca:8080/opendap/data/hdf5/hdf5-files/fake-files-for-handler/t_2d_2dll.nc4.h5 t_2d_2dll.nc4
nccopy -k 'classic' http://alpaca:8080/opendap/data/hdf5/hdf5-files/fake-files-for-handler/grid_1_2d.h5 grid_1_2d.nc
nccopy -k 'netCDF-4 classic model' http://alpaca:8080/opendap/data/hdf5/hdf5-files/fake-files-for-handler/grid_1_2d.h5 grid_1_2d.nc4
nccopy -k 'classic' http://alpaca:8080/opendap/data/hdf5/hdf5-files/fake-files-for-handler/dim_scale.h5  dim_scale.nc
nccopy -k 'netCDF-4 classic model' http://alpaca:8080/opendap/data/hdf5/hdf5-files/fake-files-for-handler/dim_scale.h5 dim_scale.nc4
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

FNC_CP_TEST="fnc_test"
mkdir $FNC_CP_TEST
cd $FNC_CP_TEST
$GET https://gamma.hdfgroup.org/ftp/pub/outgoing/opendap/data/HDF5/hdf5_handler/grid_1_2d.h5 
$GET https://gamma.hdfgroup.org/ftp/pub/outgoing/opendap/data/HDF5/hdf5_handler/dim_scale.h5 
$GET https://gamma.hdfgroup.org/ftp/pub/outgoing/opendap/data/HDF5/hdf5_handler/t_2d_2dll.nc4.h5

#3. Use fileout netcdf modules to generate the netCDF-3 and the netCDF-4
cd $CURDIR
BCMD_DIR="default_test" 
cd $BCMD_DIR
mkdir $FNC_CP_TEST

#/opt/kent/opendap/bes-dev/opendapbin/bin/besstandalone -c /opt/kent/opendap/bes-dev/opendapbin/etc/bes/bes.conf -i grid_1_2d.nc.bescmd >grid_1_2d.h5.nc
echo "Generate netcdf-3 and netcdf-4 files by using the filenetCDF modules"
for BCMD in *.bescmd
do 
	NCF=${BCMD%.bescmd}
        echo "$NCF"
	$BESSTANDALONE -c $BESCONF -i $BCMD > $FNC_CP_TEST/$NCF
done

echo "Compare ncdump output (ncdump -h for big files) for nc and nc-4 files"
cd $FNC_CP_TEST
NCDUMP="$BESPATH/deps/bin/ncdump"
command -v $NCDUMP >/dev/null
if [ "$?" -ne "0" ]
then 
	echo "ncdump is not provided by the Hyrax dependencies under BESPATH. Won't check ncdump output."
        echo "Abort the program"
        cd ..
        rm -rf $FNC_CP_TEST
       	if [ -z "$FNC_NOCLEANUP" ]; then 
       		rm -rf $FNC_CP_TEST
	fi
	exit 1
fi


for NCF in *.nc
do 
        NCF_NO_SUFFIX=${NCF%.nc}
	$NCDUMP $NCF > $NCF_NO_SUFFIX.ndp3
	if [ "$?" -ne "0" ]
	then 
		echo "ncdump FAIL to dump $NCF"
        	echo "Abort the program"
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
	$NCDUMP $NCF > $NCF_NO_SUFFIX.ndp4
	if [ "$?" -ne "0" ]
	then 
		echo "ncdump FAIL to dump $NCF"
        	echo "Abort the program"
        	cd ..
       		if [ -z "$FNC_NOCLEANUP" ]; then 
       			rm -rf $FNC_CP_TEST
		fi
        	exit 1
	fi
done

for NCF in *.nc
do 
        NCF_NO_SUFFIX=${NCF%.nc}
	diff -I  ":history = " $NCF_NO_SUFFIX.ndp3 $NCF_NO_SUFFIX.ndp4
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

echo "All default tests get PASSED"
cd ..
#Remove all the HDF5 files under BES hyrax share directory and the generated NC files
if [ -z "$FNC_NOCLEANUP" ]; then 
  	rm -rf $FNC_CP_TEST
	cd $SHAREDIR
        rm -rf $FNC_CP_TEST 
fi

