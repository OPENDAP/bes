/*
  A simple test program that generates two ZAs with 2d ZA, 1d lat and 1d level.
  Each ZA has different set of dimension sizes.
  Compilation instruction:

  %h5cc -I/path/to/hdfeos5/include  -L/path/to/hdfeos5/lib -o \
         za_2_2d_yz za_2_2d_yz.c \
         -lhe5_hdfeos -lGctp

  To generate the test file, run
  %./za_2_2d_yz

  To view the test file, run
  %/path/to/hdf5/bin/h5dump za_2_2d_yz.h5

  Copyright (C) 2012 The HDF Group
  
 */
#include <HE5_HdfEosDef.h>
herr_t write_attr(hid_t zaid, char* field_name, char* attr_name, char* value);
herr_t write_dimension(hid_t zaid, int zdim, int ydim);
herr_t write_field_1d(hid_t zaid, char* field_name, char* dim_name, int size);
herr_t write_field_2d(hid_t zaid, char* field_name, int zdim, int ydim);
herr_t write_za(hid_t zafid, char* sname, int zdim, int ydim);

/* Write a local attribute. See HDF-EOS5 testdrivers/grid/TestGrid.c.  */
herr_t write_attr(hid_t zaid, char* field_name, char* attr_name, char* value)
{
    hsize_t count[1];
    count[0] = strlen(value);
    return HE5_ZAwritelocattr(zaid, field_name, attr_name, H5T_NATIVE_CHAR,
                              count, value);
}

herr_t write_dimension(hid_t zaid, int zdim, int ydim)
{
    HE5_ZAdefdim(zaid, "YDim", ydim);
    HE5_ZAdefdim(zaid, "ZDim", zdim);
    return SUCCEED;
    
}

herr_t write_field_1d(hid_t zaid, char* field_name, char* dim_name, int size)
{
    int dim = 0;

    float val = 0.0;
    float* var = NULL;

    
    hssize_t start[1];
    hsize_t count[1];

    start[0] = 0;
    count[0] = size;
    
    var = (float*) malloc(size * sizeof(float));
    while(dim < size) {
        var[dim] = val;
        val = val + 1.0;
        dim++;
    }

    HE5_ZAdefine(zaid, field_name, dim_name, NULL, H5T_NATIVE_FLOAT);
    HE5_ZAwrite(zaid, field_name, start, NULL, count, var);
    if(var != NULL)
        free(var);
    return SUCCEED;
}

herr_t write_field_2d(hid_t zaid, char* field_name, int zdim, int ydim)
{
    int dim0 = 0;
    int dim1 = 0;

    float val = 0.0;
    float var[2][4];
    float var2[4][8];
    
    hssize_t start[2];
    hsize_t count[2];

    start[0] = 0;
    start[1] = 0;
    count[0] = (hsize_t)zdim;
    count[1] = (hsize_t)ydim;


    while(dim1 < zdim) {
        while(dim0 < ydim) {
            if(zdim == 2)
                var[dim1][dim0] = val;
            if(zdim == 4)
                var2[dim1][dim0] = val;
            val = val + 1.0;
            dim0++;
        }
        dim1++;
        dim0 = 0;
    }
    HE5_ZAdefine(zaid, field_name, "ZDim,YDim", NULL, 
                 H5T_NATIVE_FLOAT);
    return HE5_ZAwrite(zaid, field_name, start, NULL, count, var);
}

herr_t write_za(hid_t zafid, char* sname, int zdim, int ydim)
{
 
    hid_t zaid = FAIL;
  
     /* Create a Swath. */
    zaid = HE5_ZAcreate(zafid, sname);

    /* Define dimension. */
    write_dimension(zaid, zdim, ydim);

    /* Define geolocation fields. */
    write_field_1d(zaid, "Pressure", "ZDim", zdim);
    write_field_1d(zaid, "Latitude", "YDim", ydim);

    /* Define data fields. */
    write_field_2d(zaid, "Temperature", zdim, ydim);

    /* Write attributes. */
    write_attr(zaid, "Pressure", "units", "hPa");
    write_attr(zaid, "Latitude", "units", "degrees_north");
    write_attr(zaid, "Temperature", "units", "K");

    /* Close the Swath. */
    return HE5_ZAdetach(zaid);

}
int main()
{
    herr_t status = FAIL;

    hid_t  zafid = FAIL;

    /* Create a file. */
    zafid = HE5_ZAopen("za_2_2d_yz.h5", H5F_ACC_TRUNC);

    /* Create a ZA. */
    write_za(zafid, "ZA1", 2, 4);
    write_za(zafid, "ZA2", 4, 8);

    /* Close the file. */
    status = HE5_ZAclose(zafid);
    return 0;
}
/* References */
/* [1] hdfeos5/samples/he5_za_setup.c */
/* [2] hdfeos5/samples/he5_za_definefields.c */
/* [3] hdfeos5/samples/he5_za_writedata.c */
