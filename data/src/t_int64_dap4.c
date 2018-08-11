/*
  generate 64-bit integer  variables and attributes for DAP4 CF DMR support.
  Have 1d, 2d (16) /  3d, 5d (32) of size 2 in each dimension.
  Fill in min/max numbers for the first and last item.

  Compilation instruction:

  %/path/to/hdf5/bin/h5cc -o d_int d_int.c 

  To generate the test file, run
  %./d_int

  To view the test file, run
  %/path/to/hdf5/bin/h5dump d_int.h5

  Copyright (C) 2012 The HDF Group
 */

#include "hdf5.h"
#include "hdf5_hl.h"

#define H5FILE_NAME "t_int64_dap4.h5"

int
main (void)
{
    hid_t       file,  dset_ud, dset_us, dset_d, dset_s;  /* file and dataset handles */
    hid_t       dtype_vs, dtype_fs; 	     	 	/* handles */
    hid_t 	    space_s, space_d; /* handles */
    hid_t       aspace_vstr,aspace_fstr;
    hsize_t     dimd[2],dims[2];     /* dataset dimensions */
    hid_t       aid;    						/* dataspace identifiers */
    hid_t       attr_u64, attr_u64s,attr_vs,attr_fs; /* attribute identifiers */
    hid_t       grp_id;
    herr_t      status;
    uint64_t    data_uint64[2],data_uint64_sca; 		/* data to write*/
    int64_t	    data_int64[2],data_int64_sca; /* data to write */
    int8_t      data_int8_sca;
    int         data_int[2];
    char        wdata[4][8] = {"Parting", "is so\0", "swe\0et", ""};
    char*       wdata2 = "Parting is such sweet sorrow.";
    int i = 0;

    /*
     * Data and output buffer initialization.
     */
    /*
     * Assigns minimal and maximal values of int16 to datat1 and 
     * they will used to check boudary values.
     */
    data_int8_sca = (int8_t)(~(1<<8-1));
    data_int[0] =  (int32_t) (-1-~(1<<32-1)); 
    data_int[1] =  (int32_t)(~(1<<32-1));

    data_uint64_sca = (uint64_t) ~0Lu;
    data_int64_sca =(int64_t) ~(1L<<64-1);
    data_uint64[0] = data_uint64_sca;
    data_uint64[1] = data_uint64_sca-1;
    data_int64[0] = data_int64_sca;
    data_int64[1] = (int64_t) (-1-~(1L<<64-1));;

    /*
     * Create a new file using H5F_ACC_TRUNC access,
     * default file creation properties, and default file
     * access properties.
     */
    file = H5Fcreate(H5FILE_NAME, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Describe the size of the array and create the data space for fixed
     * size dataset.
     */
    dimd[0] = 2;
    space_d = H5Screate_simple(1, dimd, NULL);
    space_s  = H5Screate(H5S_SCALAR);
    dims[0] = 4;
    aspace_fstr = H5Screate_simple(1,dims,NULL);

    /*
     * Create a new dataset within the file using defined dataspace and
     * datatype and default dataset creation properties.
     */
    dset_d = H5Dcreate2(file, "d64", H5T_NATIVE_LLONG, space_d, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    dset_ud = H5Dcreate2(file, "du64", H5T_NATIVE_ULLONG, space_d, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    dset_s = H5Dcreate2(file, "d64s", H5T_NATIVE_LLONG, space_s, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    dset_us = H5Dcreate2(file, "du64s", H5T_NATIVE_ULLONG, space_s, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Write the data to the dataset using default transfer properties.
     */
    status = H5Dwrite(dset_d, H5T_NATIVE_LLONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, data_int64);
    status = H5Dwrite(dset_ud, H5T_NATIVE_ULLONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, data_uint64);
    status = H5Dwrite(dset_s, H5T_NATIVE_LLONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, &data_int64_sca);
    status = H5Dwrite(dset_us, H5T_NATIVE_ULLONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, &data_uint64_sca);

    /*
     * Create attributes.
     */

    dtype_fs = H5Tcopy(H5T_C_S1);
    H5Tset_size(dtype_fs,8);

    dtype_vs = H5Tcopy(H5T_C_S1);
    H5Tset_size(dtype_vs,H5T_VARIABLE);
    
    status = H5LTset_attribute_int(file,"/d64","attr_int",data_int,2);

    status = H5LTset_attribute_long_long(file,"/du64","attr_int64",(const long long *)data_int64,2);
    status = H5LTset_attribute_long_long(file,"/du64s","attr_int64s",(const long long *)data_int64,2);
    status = H5LTset_attribute_char(file,"/du64","attr_int8",(const char*)&data_int8_sca,1);

    attr_u64s   = H5Acreate2(dset_s, "attr_uint64s", H5T_NATIVE_ULLONG, space_s, H5P_DEFAULT, H5P_DEFAULT);
    status  = H5Awrite(attr_u64s, H5T_NATIVE_ULLONG, &data_uint64_sca);
    H5Aclose(attr_u64s);

    attr_u64   = H5Acreate2(dset_d, "attr_uint64", H5T_NATIVE_ULLONG, space_s, H5P_DEFAULT, H5P_DEFAULT);
    status  = H5Awrite(attr_u64, H5T_NATIVE_ULLONG, &data_uint64_sca);
    H5Aclose(attr_u64);

    attr_fs = H5Acreate2(dset_ud, "attr_fstr", dtype_fs, aspace_fstr, H5P_DEFAULT, H5P_DEFAULT);
    status  = H5Awrite(attr_fs, dtype_fs, wdata);
    H5Aclose(attr_fs);

    attr_vs = H5Acreate2(dset_d, "attr_vstr", dtype_vs, space_s, H5P_DEFAULT, H5P_DEFAULT);
    status  = H5Awrite(attr_fs, dtype_vs, &wdata2);
    H5Aclose(attr_vs);

    /* 64-bit intger group and root attributes */

    status = H5LTset_attribute_long_long(file,"/","root_attr_int64",(const long long *)data_int64,2);
    status = H5LTset_attribute_long_long(file,"/","root_attr_int64_scalar",(const long long *)&data_int64_sca,1);
    grp_id = H5Gcreate2(file,"grp_int64",H5P_DEFAULT,H5P_DEFAULT,H5P_DEFAULT);
    status = H5LTset_attribute_long_long(file,"/grp_int64","grp_attr_int64",(const long long *)data_int64,2);
    status = H5LTset_attribute_long_long(file,"/grp_int64","grp_attr_int64_scalar",(const long long *)&data_int64_sca,1);
    /*
     * Close/release resources.
     */
    H5Sclose(space_d);
    H5Sclose(space_s);
    H5Sclose(aspace_fstr);

    H5Tclose(dtype_fs);
    H5Tclose(dtype_vs);

    H5Dclose(dset_d);
    H5Dclose(dset_ud);
    H5Dclose(dset_s);
    H5Dclose(dset_us);

    H5Fclose(file);

    return 0;
}

