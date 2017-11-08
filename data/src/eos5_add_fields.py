import shutil
import h5py
import numpy

def add_extra_field(fname,varname,varsize):
    
    file = h5py.File (fname, 'a')
    
    # initialize temperature array
    var_array = numpy.arange(varsize)
    temp_dset = file.create_dataset (varname, data=var_array,
                                     dtype=numpy.int32)
    
    file.close
    return
def add_grid_dim_info(fname,xdim,ydim,var1name,var2name):
    
    file = h5py.File (fname, 'a')
    
    # initialize temperature array
    v1dset = file[var1name]
    ydimsize = v1dset.shape[0]
    xdimsize = v1dset.shape[1]
    
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
    projv1dset = grp1.create_dataset(projv1name,(),dtype=numpy.int8);
    v1dset = grp1[v1name];
    v1dset.attrs["grid_mapping"]=projv1name;
    grp2 = file[v2path];
    projv2dset = grp2.create_dataset(projv2name,(),dtype=numpy.int8);
    v2dset = grp2[v2name];
    v2dset.attrs["grid_mapping"]=v2path+"/"+projv2name;
    file.close
    return

def add_swath_coordinates(fname,v1path,v1name,v2path,v2name,geo1path):
    file = h5py.File (fname, 'a')
    grp1 = file[v1path];
    v1dset = grp1[v1name];
    v1dset.attrs["coordinates"]=geo1path+"/"+"Latitude"+" "+geo1path+"/"+"Longitude";
    grp2 = file[v2path];
    v2dset = grp2[v2name];
    v2dset.attrs["coordinates"]=v2path+"/"+"Latitude"+" "+v2path+"/"+"Longitude";
    file.close
    return

 
shutil.copyfile("../grid_2_2d.h5","grid_2_2d_ef.h5");
shutil.copyfile("../swath_2_3d_2x2yz.h5","swath_2_3d_2x2yz_ef.h5");
shutil.copyfile("../za_1_2d_yz.h5","za_1_2d_yz_ef.h5");
add_extra_field("grid_2_2d_ef.h5","/HDFEOS/GRIDS/XDim",8);
add_extra_field("grid_2_2d_ef.h5","/HDFEOS/GRIDS/YDim",4);
add_extra_field("swath_2_3d_2x2yz_ef.h5","/HDFEOS/SWATHS/dummy_extra",2);
add_extra_field("za_1_2d_yz_ef.h5","/HDFEOS/ZAS/1dummy_extra",2);
add_grid_dim_info("grid_2_2d_ef.h5","/HDFEOS/GRIDS/XDim","/HDFEOS/GRIDS/YDim","/HDFEOS/GRIDS/GeoGrid1/Data Fields/temperature","/HDFEOS/GRIDS/GeoGrid2/Data Fields/temperature" );
add_grid_mapping_info("grid_2_2d_ef.h5","/HDFEOS/GRIDS/GeoGrid1/Data Fields","temperature","dummy1_mapping_info","/HDFEOS/GRIDS/GeoGrid2/Data Fields", "temperature","dummy2_mapping_info");
add_swath_coordinates("swath_2_3d_2x2yz_ef.h5","/HDFEOS/SWATHS/Swath1/Data Fields","Temperature","/HDFEOS/SWATHS/Swath2/Data Fields","Temperature","/HDFEOS/SWATHS/Swath1/Geolocation Fields");
