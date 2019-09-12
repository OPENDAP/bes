3nccopy -k "classic" -m 500000000 http://alpaca:8080/opendap/data/hdf5/hdf5-files/NASA-files/3A-MO.GPM.GMI.GRID2014R1.20140601-S000000-E235959.06.V03A.h5 3A-MO.nc
nccopy -k "netCDF-4 classic model" -m 500000000 -d 2 -s http://alpaca:8080/opendap/data/hdf5/hdf5-files/NASA-files/3A-MO.GPM.GMI.GRID2014R1.20140601-S000000-E235959.06.V03A.h5 3A-MO.nc4
nccopy -k "classic" -m 500000000 http://alpaca:8080/opendap/data/hdf5/hdf5-files/NASA-files/OMPS-NPP_NMTO3-L3-DAILY_v2.1_2018m0102_2018m0104t012837.h5 OMPS.nc
nccopy -k "netCDF-4 classic model" -m 500000000 -d 2 -s http://alpaca:8080/opendap/data/hdf5/hdf5-files/NASA-files/OMPS-NPP_NMTO3-L3-DAILY_v2.1_2018m0102_2018m0104t012837.h5 OMPS.nc4
nccopy -k "netCDF-4 classic model" -m 500000000 -d 2 -s http://alpaca:8080/opendap/data/hdf5/hdf5-files/NASA-files/SMAP_L3_SM_P_20150406_R14010_001.h5 SMAP_L3.nc4
nccopy -k "classic" -m 500000000  http://alpaca:8080/opendap/data/hdf5/hdf5-files/NASA-files/SMAP_L3_SM_P_20150406_R14010_001.h5 SMAP_L3.nc
nccopy -k "classic" -m 500000000  http://alpaca:8080/opendap/data/hdf5/hdf5-files/NASA-files/VNP09A1.A2015257.h29v11.001.2016221164845.h5 VNP.nc
nccopy -k "netCDF-4 classic model" -m 500000000 -d 2 -s http://alpaca:8080/opendap/data/hdf5/hdf5-files/NASA-files/VNP09A1.A2015257.h29v11.001.2016221164845.h5 VNP.nc4

