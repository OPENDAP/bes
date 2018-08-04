import shutil
import h5py
import numpy

def add_swath_coordinates(file,v1path,v1name,v2path,v2name):
    grp1 = file[v1path]
    v1dset = grp1[v1name]
    v1dset.attrs["coordinates"]="Latitude"+" "+"Longitude"+" "+"Pressure";
    grp2 = file[v2path];
    v2dset = grp2[v2name];
    v2dset.attrs["coordinates"]="Latitude"+" "+"Longitude"+" "+"Pressure";
    return

shutil.copyfile("../swath_2_3d_2x2yz_ef.h5","swath_2_3d_2x2yz_ef_co.h5");
file = h5py.File ("swath_2_3d_2x2yz_ef_co.h5", 'a')
add_swath_coordinates(file,"/HDFEOS/SWATHS/Swath1/Data Fields","Temperature","/HDFEOS/SWATHS/Swath2/Data Fields","Temperature");
file.close
