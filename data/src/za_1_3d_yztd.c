/*
  A simple test program that generates one ZA with 3d ZA, 1d lat, 
  1d level, 1d time, and 1d date.
  
  Compilation instruction:

  %h5cc -I/path/to/hdfeos5/include  -L/path/to/hdfeos5/lib -o \
  za_1_3d_yztd za_1_3d_yztd.c -lhe5_hdfeos -lGctp

  To generate the test file, run
  %./za_1_3d_yztd

  To view the test file, run
  %/path/to/hdf5/bin/h5dump za_1_3d_yztd.h5

  Copyright (C) 2012 The HDF Group
  
 */
#include <HE5_HdfEosDef.h>

herr_t write_attr(hid_t zaid, char* field_name, char* attr_name, char* value);
herr_t write_dimension(hid_t zaid);
herr_t write_field_1d(hid_t zaid, char* field_name, char* dim_name, int size);
herr_t write_field_3d(hid_t zaid, char* field_name);
herr_t write_za(hid_t zafid, char* sname);

/* Write a local attribute. See HDF-EOS5 testdrivers/grid/TestGrid.c.  */
herr_t write_attr(hid_t zaid, char* field_name, char* attr_name, char* value)
{
    hsize_t count[1];
    count[0] = strlen(value);
    return HE5_ZAwritelocattr(zaid, field_name, attr_name, H5T_NATIVE_CHAR,
                              count, value);
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

herr_t write_field_3d(hid_t zaid, char* field_name)
{
    int dim0 = 0;
    int dim1 = 0;
    int dim2 = 0;

    float val = 0.0;
    float var[2][4][8];
    
    hssize_t start[3];
    hsize_t count[3];

    start[0] = 0;
    start[1] = 0;
    start[2] = 0;
    count[0] = 2;
    count[1] = 4;
    count[2] = 8;

    while(dim2 < 2) {
        while(dim1 < 4) {
            while(dim0 < 8) {
                var[dim2][dim1][dim0] = val;
                val = val + 1.0;
                dim0++;
            }
            dim1++;
            dim0 = 0;
        }
        dim2++;
        dim1 = 0;
        dim0 = 0;
    }


    HE5_ZAdefine(zaid, field_name, "TDim,ZDim,YDim", NULL, 
                 H5T_NATIVE_FLOAT);
    return HE5_ZAwrite(zaid, field_name, start, NULL, count, var);
}

herr_t write_dimension(hid_t zaid)
{
    HE5_ZAdefdim(zaid, "YDim", 8);
    HE5_ZAdefdim(zaid, "ZDim", 4);
    HE5_ZAdefdim(zaid, "TDim", 2);
    return SUCCEED;
    
}

herr_t write_za(hid_t zafid, char* sname)
{
 
    hid_t zaid = FAIL;
  
     /* Create a Swath. */
    zaid = HE5_ZAcreate(zafid, sname);

    /* Define dimension. */
    write_dimension(zaid);

    /* Define geolocation fields. */
    write_field_1d(zaid, "Pressure", "ZDim",  4);
    write_field_1d(zaid, "Time", "TDim", 2);
    write_field_1d(zaid, "Date", "TDim", 2);
    write_field_1d(zaid, "Latitude", "YDim", 8);

    /* Define data fields. */
    write_field_3d(zaid, "Temperature");

    /* Write attributes. */
    write_attr(zaid, "Pressure", "units", "hPa");
    write_attr(zaid, "Time", "units", "seconds since 2012-1-1 0:0:0");
    write_attr(zaid, "Date", "units", "days since 2012-1-1 0:0:0");
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
    zafid = HE5_ZAopen("za_1_3d_yztd.h5", H5F_ACC_TRUNC);

    /* Create a ZA. */
    write_za(zafid, "ZA");

    /* Close the file. */
    status = HE5_ZAclose(zafid);
    
}
/* References */
/* [1] hdfeos5/samples/he5_za_setup.c */
/* [2] hdfeos5/samples/he5_za_definefields.c */
/* [3] hdfeos5/samples/he5_za_writedata.c */
