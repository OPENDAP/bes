import shutil
import h5py
import numpy

shutil.copyfile("../grid_1_2d.h5","grid_1_2d_convention.h5");
file = h5py.File ("grid_1_2d_convention.h5", 'a')
grp1=file["/HDFEOS/ADDITIONAL/FILE_ATTRIBUTES"]
grp1.attrs["Conventions"]="CF-1.7"
file.close
