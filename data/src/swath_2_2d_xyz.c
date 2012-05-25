/*
  A simple HDF-EOS5 Swath file.
  
  Compilation instruction:

  %h5cc -I/path/to/hdfeos5/include  -L/path/to/hdfeos5/lib \
        -o swath_2_2d_xyz swath_2_2d_xyz.c \
        -lhe5_hdfeos -lGctp

  To generate the test file, run
  %./swath_2_2d_xyz

  To view the test file, run
  %/path/to/hdf5/bin/h5dump swath_2_2d_xyz.h5

  Copyright (C) 2012 The HDF Group
  
 */
#include <HE5_HdfEosDef.h>
herr_t write_attr(hid_t swid, char* field_name, char* attr_name, char* value);
herr_t write_dimension(hid_t swid, int zdim, int ndim);
herr_t write_field_1d(hid_t swid, char* field_name, int geo, int size, 
                      char* dim_name, char* unit);
herr_t write_field_2d(hid_t swid, char* field_name, int geo, char* dim_name, 
                      int zdim, int ndim, char* unit);
herr_t write_swath(hid_t swfid, char* sname, int zdim, int ndim);

herr_t write_attr(hid_t swid, char* field_name, char* attr_name, char* value)
{
    hsize_t count[1];
    count[0] = strlen(value);
    return HE5_SWwritelocattr(swid, field_name, attr_name, H5T_NATIVE_CHAR,
                              count, value);
}

herr_t write_field_1d(hid_t swid, char* field_name, int geo, int size, 
                      char* dim_name, char* unit)
{
    int dim = 0;

    float val = 0.0;
    float* var = NULL;
    
    hssize_t start[1];
    hsize_t count[1];

    start[0] = 0;
    count[0] = size;

    var = (float*) malloc(size * sizeof(float));
    if(var == NULL){
        fprintf(stderr, "Out of Memory\n");
        return FAIL;
    }

    while(dim < size) {
        var[dim] = val;
        val = val + 1.0;
        dim++;
    }

    if(1 == geo)
        HE5_SWdefgeofield(swid, field_name, dim_name, NULL, 
                                 H5T_NATIVE_FLOAT, 0);
    if(0 == geo)
        HE5_SWdefdatafield(swid, field_name, dim_name, NULL, 
                                  H5T_NATIVE_FLOAT, 0);
    HE5_SWwritefield(swid, field_name, start, NULL, count, var);

    if(var != NULL)
        free(var);

    return write_attr(swid, field_name, "units", unit);

}

herr_t write_field_2d(hid_t swid, char* field_name, int geo, char* dim_name, 
                      int zdim, int ndim, char* unit)
{
    int i = 0;
    int dim0 = 0;
    int dim1 = 0;

    float val = 0.0;
    /* Dynamic memory allocation doesn't work with HE5 writefield() API. */
    /* You'll get wrong values being stored into dataset. */
    /* Is it related to C storage order? Investigate it later further. */
    /* float **var = NULL;*/
    float var[2][4];
    float var2[4][8];

    hssize_t start[2];
    hsize_t count[2];

    start[0] = 0;
    start[1] = 0;
    count[0] = (hsize_t)zdim;
    count[1] = (hsize_t)ndim;

    while(dim1 < zdim) {
        while(dim0 < ndim) {
            if(zdim == 2){
               var[dim1][dim0] = val;
            }
            if(zdim == 4){
               var2[dim1][dim0] = val;
            }
            val = val + 1.0;
            dim0++;
        }
        dim1++;
        dim0 = 0;
    }


    if(1 == geo)
        HE5_SWdefgeofield(swid, field_name, dim_name, NULL, 
                                 H5T_NATIVE_FLOAT, 0);
    if(0 == geo)
        HE5_SWdefdatafield(swid, field_name, dim_name, NULL, 
                                  H5T_NATIVE_FLOAT, 0);
    if(zdim == 2){
        HE5_SWwritefield(swid, field_name, start, NULL, count, var);
    }

    if(zdim == 4){
        HE5_SWwritefield(swid, field_name, start, NULL, count, var2);
    }

    return write_attr(swid, field_name, "units", unit);

}


herr_t write_dimension(hid_t swid, int zdim, int ndim)
{
    HE5_SWdefdim(swid, "ZDim", zdim);
    HE5_SWdefdim(swid, "NDim", ndim);
    return SUCCEED;
}

herr_t write_swath(hid_t swfid, char* sname, int zdim, int ndim)
{
 
    hid_t swid = FAIL;
  
     /* Create a Swath. */
    swid = HE5_SWcreate(swfid, sname);

    /* Define dimension. */
    write_dimension(swid, zdim, ndim);

    /* Define geolocation fields. */
    write_field_1d(swid, "Pressure",  1, zdim, "ZDim", "hPa");
    write_field_1d(swid, "Latitude",  1, ndim, "NDim", "degrees_north");
    write_field_1d(swid, "Longitude", 1, ndim, "NDim", "degrees_east" );

    /* Define data fields. */
    write_field_2d(swid, "Temperature", 0, "ZDim,NDim", zdim, ndim, "K");

    /* Close the Swath. */
    return HE5_SWdetach(swid);

}

int main()
{
    herr_t status = FAIL;

    hid_t  swfid = FAIL;

    /* Create a file. */
    swfid = HE5_SWopen("swath_2_2d_xyz.h5", H5F_ACC_TRUNC);

    /* Create a Swath. */
    write_swath(swfid, "Swath1", 4, 8);
    write_swath(swfid, "Swath2", 2, 4);

    /* Close the file. */
    status = HE5_SWclose(swfid);
    
}
/* References */
/* [1] hdfeos5/samples/he5_sw_setup.c */
/* [2] hdfeos5/samples/he5_sw_definefields.c */
/* [3] hdfeos5/samples/he5_sw_writedata.c */
