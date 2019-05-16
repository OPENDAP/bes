import shutil
import h5py
import numpy

def add_extra_field(fname,varname,varsize):
    
    file = h5py.File (fname, 'a')
    # initialize temperature array
    var_array = numpy.arange(varsize)
    file.create_dataset (varname, data=var_array,
                                     dtype=numpy.int32)
    
    file.close
    return
def add_grid_dim_info(fname,xdim,ydim,var1name,var2name):
    
    file = h5py.File (fname, 'a')
    
    #obtain temperature array
    v1dset = file[var1name]
    
    file[xdim].dims.create_scale(file[xdim])
    file[ydim].dims.create_scale(file[ydim])
    file[var1name].dims[0].attach_scale(file[ydim])
    file[var1name].dims[1].attach_scale(file[xdim])
    file[var2name].dims[0].attach_scale(file[ydim])
    file[var2name].dims[1].attach_scale(file[xdim])
    
    file.close
    return

def add_grid_mapping_info(fname,v1path,v1name,projv1name,v2path,v2name,projv2name):
    file = h5py.File (fname, 'a')
    grp1 = file[v1path];
    grp1.create_dataset(projv1name,(),dtype=numpy.int8);
    v1dset = grp1[v1name];
    v1dset.attrs["grid_mapping"]=projv1name;
    grp2 = file[v2path];
    grp2.create_dataset(projv2name,(),dtype=numpy.int8);
    v2dset = grp2[v2name];
    v2dset.attrs["grid_mapping"]=v2path+"/"+projv2name;
    file.close
    return

def add_swath_dim_info(file,fname,xdimname,ydimname,zdimname,v1name):
    
    # obtain temperature array
    v1dset = file[v1name]
    ydimsize = v1dset.shape[1]
    xdimsize = v1dset.shape[2]
    file[zdimname].dims.create_scale(file[zdimname])
    e = ydimname in file
    if (e==False):
        add_extra_field(fname,ydimname,ydimsize)
        file[ydimname].dims.create_scale(file[ydimname])
    e = xdimname in file
    if (e==False):
        add_extra_field(fname,xdimname,xdimsize)
        file[xdimname].dims.create_scale(file[xdimname])
    
    file[v1name].dims[0].attach_scale(file[zdimname])
    file[v1name].dims[1].attach_scale(file[ydimname])
    file[v1name].dims[2].attach_scale(file[xdimname])
    
    return


def add_swath_coordinates(file,v1path,v1name,v2path,v2name,geo1path,geo2path):
    grp1 = file[v1path]
    grp2 = file[v2path];
    return

def attach_2d_scales(file,swath_geov_2d_list,dim0name,dim1name):
    for varname in swath_geov_2d_list:
        file[varname].dims[0].attach_scale(file[dim0name])
        file[varname].dims[1].attach_scale(file[dim1name])
    return

shutil.copyfile("../grid_2_2d.h5","grid_2_2d_ef.h5");
shutil.copyfile("../swath_2_3d_2x2yz.h5","swath_2_3d_2x2yz_ef.h5");
shutil.copyfile("../za_1_2d_yz.h5","za_1_2d_yz_ef.h5");
add_extra_field("grid_2_2d_ef.h5","/HDFEOS/GRIDS/XDim",8);
add_extra_field("grid_2_2d_ef.h5","/HDFEOS/GRIDS/YDim",4);
add_extra_field("swath_2_3d_2x2yz_ef.h5","/HDFEOS/SWATHS/dummy_extra",2);
add_grid_dim_info("grid_2_2d_ef.h5","/HDFEOS/GRIDS/XDim","/HDFEOS/GRIDS/YDim","/HDFEOS/GRIDS/GeoGrid1/Data Fields/temperature","/HDFEOS/GRIDS/GeoGrid2/Data Fields/temperature" );
add_grid_mapping_info("grid_2_2d_ef.h5","/HDFEOS/GRIDS/GeoGrid1/Data Fields","temperature","dummy1_mapping_info","/HDFEOS/GRIDS/GeoGrid2/Data Fields", "temperature","dummy2_mapping_info");
#handling swath, many fields, we'd better not open and close many times
file = h5py.File ("swath_2_3d_2x2yz_ef.h5", 'a')
add_swath_dim_info(file,"swath_2_3d_2x2yz_ef.h5","/HDFEOS/SWATHS/Swath1/XDim","/HDFEOS/SWATHS/Swath1/YDim","/HDFEOS/SWATHS/Swath1/Geolocation Fields/Pressure","/HDFEOS/SWATHS/Swath1/Data Fields/Temperature");
add_swath_dim_info(file,"swath_2_3d_2x2yz_ef.h5","/HDFEOS/SWATHS/Swath2/XDim","/HDFEOS/SWATHS/Swath2/YDim","/HDFEOS/SWATHS/Swath2/Geolocation Fields/Pressure","/HDFEOS/SWATHS/Swath2/Data Fields/Temperature");
grp1_path="/HDFEOS/SWATHS/Swath1/Geolocation Fields/";
grp2_path="/HDFEOS/SWATHS/Swath2/Geolocation Fields/";
swath_geov_2d_list =[grp1_path+"Latitude",grp1_path+"Longitude"]
attach_2d_scales(file,swath_geov_2d_list,"/HDFEOS/SWATHS/Swath1/YDim","/HDFEOS/SWATHS/Swath1/XDim")
swath_geov_2d_list=[grp2_path+"Latitude",grp2_path+"Longitude"]
attach_2d_scales(file,swath_geov_2d_list,"/HDFEOS/SWATHS/Swath2/YDim","/HDFEOS/SWATHS/Swath2/XDim")
add_swath_coordinates(file,"/HDFEOS/SWATHS/Swath1/Data Fields","Temperature","/HDFEOS/SWATHS/Swath2/Data Fields","Temperature","/HDFEOS/SWATHS/Swath1/Geolocation Fields","/HDFEOS/SWATHS/Swath2/Geolocation Fields");
file.close
add_extra_field("za_1_2d_yz_ef.h5","/HDFEOS/ZAS/1dummy_extra",2);
