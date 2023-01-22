import shutil
import h5py
import numpy

def add_extra_field(fname,varname,varsize):
    
    h5_file = h5py.File (fname, 'a')
    # initialize temperature array
    var_array = numpy.arange(varsize)
    h5_file.create_dataset (varname, data=var_array,
                                     dtype=numpy.int32)
    
    h5_file.close

def add_grid_dim_info(fname,xdim,ydim,var1name,var2name):
    
    h5_file = h5py.File (fname, 'a')
    
    h5_file[xdim].dims.create_scale(h5_file[xdim])
    h5_file[ydim].dims.create_scale(h5_file[ydim])
    h5_file[var1name].dims[0].attach_scale(h5_file[ydim])
    h5_file[var1name].dims[1].attach_scale(h5_file[xdim])
    h5_file[var2name].dims[0].attach_scale(h5_file[ydim])
    h5_file[var2name].dims[1].attach_scale(h5_file[xdim])
    
    h5_file.close

def add_grid_mapping_info(fname,v1path,v1name,projv1name,v2path,v2name,projv2name):
    h5_file = h5py.File (fname, 'a')
    grp1 = h5_file[v1path];
    grp1.create_dataset(projv1name,(),dtype=numpy.int8);
    v1dset = grp1[v1name];
    v1dset.attrs["grid_mapping"]=projv1name;
    grp2 = h5_file[v2path];
    grp2.create_dataset(projv2name,(),dtype=numpy.int8);
    v2dset = grp2[v2name];
    v2dset.attrs["grid_mapping"]=v2path+"/"+projv2name;
    h5_file.close

def add_swath_dim_info(h5_file,fname,xdimname,ydimname,zdimname,v1name):
    
    # obtain temperature array
    v1dset = h5_file[v1name]
    ydimsize = v1dset.shape[1]
    xdimsize = v1dset.shape[2]
    h5_file[zdimname].dims.create_scale(h5_file[zdimname])
    e = ydimname in h5_file
    if (e==False):
        add_extra_field(fname,ydimname,ydimsize)
        h5_file[ydimname].dims.create_scale(h5_file[ydimname])
    e = xdimname in h5_file
    if (e==False):
        add_extra_field(fname,xdimname,xdimsize)
        h5_file[xdimname].dims.create_scale(h5_file[xdimname])
    
    h5_file[v1name].dims[0].attach_scale(h5_file[zdimname])
    h5_file[v1name].dims[1].attach_scale(h5_file[ydimname])
    h5_file[v1name].dims[2].attach_scale(h5_file[xdimname])
    

def attach_2d_scales(h5_file,swath_geov_2d_list,dim0name,dim1name):
    for varname in swath_geov_2d_list:
        h5_file[varname].dims[0].attach_scale(h5_file[dim0name])
        h5_file[varname].dims[1].attach_scale(h5_file[dim1name])

grid_2_2d_ef_name = "grid_2_2d_ef.h5";
shutil.copyfile("../grid_2_2d.h5",grid_2_2d_ef_name);
shutil.copyfile("../swath_2_3d_2x2yz.h5","swath_2_3d_2x2yz_ef.h5");
shutil.copyfile("../za_1_2d_yz.h5","za_1_2d_yz_ef.h5");
add_extra_field(grid_2_2d_ef_name,"/HDFEOS/GRIDS/XDim",8);
add_extra_field(grid_2_2d_ef_name,"/HDFEOS/GRIDS/YDim",4);
add_extra_field("swath_2_3d_2x2yz_ef.h5","/HDFEOS/SWATHS/dummy_extra",2);
add_grid_dim_info(grid_2_2d_ef_name,"/HDFEOS/GRIDS/XDim","/HDFEOS/GRIDS/YDim","/HDFEOS/GRIDS/GeoGrid1/Data Fields/temperature","/HDFEOS/GRIDS/GeoGrid2/Data Fields/temperature" );
add_grid_mapping_info(grid_2_2d_ef_name,"/HDFEOS/GRIDS/GeoGrid1/Data Fields","temperature","dummy1_mapping_info","/HDFEOS/GRIDS/GeoGrid2/Data Fields", "temperature","dummy2_mapping_info");
#handling swath, many fields, we'd better not open and close many times
h5_file = h5py.File ("swath_2_3d_2x2yz_ef.h5", 'a')
add_swath_dim_info(h5_file,"swath_2_3d_2x2yz_ef.h5","/HDFEOS/SWATHS/Swath1/XDim","/HDFEOS/SWATHS/Swath1/YDim","/HDFEOS/SWATHS/Swath1/Geolocation Fields/Pressure","/HDFEOS/SWATHS/Swath1/Data Fields/Temperature");
add_swath_dim_info(h5_file,"swath_2_3d_2x2yz_ef.h5","/HDFEOS/SWATHS/Swath2/XDim","/HDFEOS/SWATHS/Swath2/YDim","/HDFEOS/SWATHS/Swath2/Geolocation Fields/Pressure","/HDFEOS/SWATHS/Swath2/Data Fields/Temperature");
grp1_path="/HDFEOS/SWATHS/Swath1/Geolocation Fields/";
grp2_path="/HDFEOS/SWATHS/Swath2/Geolocation Fields/";
swath_geov_2d_list =[grp1_path+"Latitude",grp1_path+"Longitude"]
attach_2d_scales(h5_file,swath_geov_2d_list,"/HDFEOS/SWATHS/Swath1/YDim","/HDFEOS/SWATHS/Swath1/XDim")
swath_geov_2d_list=[grp2_path+"Latitude",grp2_path+"Longitude"]
attach_2d_scales(h5_file,swath_geov_2d_list,"/HDFEOS/SWATHS/Swath2/YDim","/HDFEOS/SWATHS/Swath2/XDim")
h5_file.close
add_extra_field("za_1_2d_yz_ef.h5","/HDFEOS/ZAS/1dummy_extra",2);
