nccopy -k "classic" -m 500000000 http://alpaca:8080/opendap/data/hdf5/hdf5-files/NASA-files/3A-MO.GPM.GMI.GRID2014R1.20140601-S000000-E235959.06.V03A.h5 3A-MO.nc
nccopy -k "netCDF-4 classic model" -m 500000000 -d 2 -s http://alpaca:8080/opendap/data/hdf5/hdf5-files/NASA-files/3A-MO.GPM.GMI.GRID2014R1.20140601-S000000-E235959.06.V03A.h5 3A-MO.nc4
nccopy -k "classic" -m 500000000 http://alpaca:8080/opendap/data/hdf5/hdf5-files/NASA-files/OMPS-NPP_NMTO3-L3-DAILY_v2.1_2018m0102_2018m0104t012837.h5 OMPS.nc
nccopy -k "netCDF-4 classic model" -m 500000000 -d 2 -s http://alpaca:8080/opendap/data/hdf5/hdf5-files/NASA-files/OMPS-NPP_NMTO3-L3-DAILY_v2.1_2018m0102_2018m0104t012837.h5 OMPS.nc4
nccopy -k "netCDF-4 classic model" -m 500000000 -d 2 -s http://alpaca:8080/opendap/data/hdf5/hdf5-files/NASA-files/SMAP_L3_SM_P_20150406_R14010_001.h5 SMAP_L3.nc4
nccopy -k "classic" -m 500000000  http://alpaca:8080/opendap/data/hdf5/hdf5-files/NASA-files/SMAP_L3_SM_P_20150406_R14010_001.h5 SMAP_L3.nc
nccopy -k "classic" -m 500000000  http://alpaca:8080/opendap/data/hdf5/hdf5-files/NASA-files/VNP09A1.A2015257.h29v11.001.2016221164845.h5 VNP.nc
nccopy -k "netCDF-4 classic model" -m 500000000 -d 2 -s http://alpaca:8080/opendap/data/hdf5/hdf5-files/NASA-files/VNP09A1.A2015257.h29v11.001.2016221164845.h5 VNP.nc4


#!/bin/sh

#The path BES is installed must be provided explicitly.
if [ -z "$BESPATH" ]; then
    echo Environment variable BESPATH should be set. It should be the BES path the handler is linked with.
    exit
fi

#FNC_NOCLEANUP="yes"

# Use this path to figure out the location of besstandalone, bes.conf and the data.
BESCONF="$BESPATH/etc/bes/bes.conf"
BESH5SHARE="$BESPATH/share/hyrax/data/hdf5"
NCCOPY="$BESPATH/deps/bin/nccopy"
NCDUMP="$BESPATH/deps/bin/ncdump"
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

NC_CP_TEST="nccp_nasa_test"
mkdir $NC_CP_TEST
cd $NC_CP_TEST
HDF5_FILE_DIR=`pwd`

#NASA files
$GET https://gamma.hdfgroup.org/ftp/pub/outgoing/opendap/data/HDF5/NASA1/GESDISC/3A-MO.GPM.GMI.GRID2014R1.20140601-S000000-E235959.06.V03A.h5
$GET https://gamma.hdfgroup.org/ftp/pub/outgoing/opendap/data/HDF5/NASA1/GESDISC/OMPS-NPP_NMTO3-L3-DAILY_v2.1_2018m0102_2018m0104t012837.h5
$GET https://gamma.hdfgroup.org/ftp/pub/outgoing/opendap/data/HDF5/NASA1/LPDAAC/VNP09A1.A2015257.h29v11.001.2016221164845.h5
$GET https://gamma.hdfgroup.org/ftp/pub/outgoing/opendap/data/HDF5/NASA1/NSIDC/SMAP_L3_SM_P_20150406_R14010_001.h5


#3. Use Unidata's nccopy to generate the netCDF-3 and the netCDF-4
cd $CURDIR

#nccopy -k 'classic' http://alpaca:8080/opendap/data/hdf5/hdf5-files/fake-files-for-handler/t_2d_2dll.nc4.h5 t_2d_2dll.nc
#nccopy -k 'netCDF-4 classic model' http://alpaca:8080/opendap/data/hdf5/hdf5-files/fake-files-for-handler/t_2d_2dll.nc4.h5 t_2d_2dll.nc4
#/opt/kent/opendap/bes-dev/opendapbin/bin/besstandalone -c /opt/kent/opendap/bes-dev/opendapbin/etc/bes/bes.conf -i grid_1_2d.nc.bescmd >grid_1_2d.h5.nc
echo "Generate netcdf-3 and netcdf-4 files by using nccopy"

nccopy -k "classic" -m 500000000 http://localhost:8080/opendap/data/hdf5/$NC_CP_TEST/3A-MO.GPM.GMI.GRID2014R1.20140601-S000000-E235959.06.V03A.h5 3A-MO.nc
nccopy -k "netCDF-4 classic model" -m 500000000 -d 2 -s http://localhost:8080/opendap/data/hdf5/$NC_CP_TEST/3A-MO.GPM.GMI.GRID2014R1.20140601-S000000-E235959.06.V03A.h5 3A-MO.nc4
nccopy -k "classic" -m 500000000 http://localhost:8080/opendap/data/hdf5/$NC_CP_TEST/OMPS-NPP_NMTO3-L3-DAILY_v2.1_2018m0102_2018m0104t012837.h5 OMPS.nc
nccopy -k "netCDF-4 classic model" -m 500000000 -d 2 -s http://localhost:8080/opendap/data/hdf5/$NC_CP_TEST/OMPS-NPP_NMTO3-L3-DAILY_v2.1_2018m0102_2018m0104t012837.h5 OMPS.nc4
nccopy -k "netCDF-4 classic model" -m 500000000 -d 2 -s http://localhost:8080/opendap/data/hdf5/$NC_CP_TEST/SMAP_L3_SM_P_20150406_R14010_001.h5 SMAP_L3.nc4
nccopy -k "classic" -m 500000000  http://localhost:8080/opendap/data/hdf5/$NC_CP_TEST/SMAP_L3_SM_P_20150406_R14010_001.h5 SMAP_L3.nc
nccopy -k "classic" -m 500000000  http://localhost:8080/opendap/data/hdf5/$NC_CP_TEST/VNP09A1.A2015257.h29v11.001.2016221164845.h5 VNP.nc
nccopy -k "netCDF-4 classic model" -m 500000000 -d 2 -s http://localhost:8080/opendap/data/hdf5/$NC_CP_TEST/VNP09A1.A2015257.h29v11.001.2016221164845.h5 VNP.nc4


echo "Compare ncdump output (ncdump -h for big files) for nc and nc-4 files"
command -v $NCDUMP >/dev/null
if [ "$?" -ne "0" ]
then 
	echo "ncdump is not provided by the Hyrax dependencies under BESPATH. Won't check ncdump output."
        echo "Abort the program"
        cd ..
       	if [ -z "$FNC_NOCLEANUP" ]; then 
       		rm -rf $HDF5_FILE_DIR
                rm -rf *.nc
                rm -rf *.nc4
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
        	echo "Abort the program"
	       	if [ -z "$FNC_NOCLEANUP" ]; then 
       			rm -rf $HDF5_FILE_DIR
                        rm -rf *.nc
                        rm -rf *.nc4
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
        	echo "Abort the program"
        	cd ..
       		if [ -z "$FNC_NOCLEANUP" ]; then 
       			rm -rf $HDF5_FILE_DIR
                        rm -rf *.nc
                        rm -rf *.nc4
			rm -rf *.ndr3
		fi
        	exit 1
	fi
done

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
       			rm -rf $HDF5_FILE_DIR
                        rm -rf *.nc
                        rm -rf *.nc4
			rm -rf *.ndr3
			rm -rf *.ndr4
		fi
        	exit 1
	fi

done

echo "All NASA tests get PASSED"
#Remove all the HDF5 files under BES hyrax share directory and the generated NC files
if [ -z "$FNC_NOCLEANUP" ]; then 
   rm -rf $HDF5_FILE_DIR
   rm -rf *.nc
   rm -rf *.nc4
   rm -rf *.ndr3
   rm -rf *.ndr4
	
fi
<<'COMMENT'
COMMENT
