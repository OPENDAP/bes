/*
  A simple test program that generates one Swath with 3d swath, 2d lat/lon, 
  1d level, 1d time, and 1d date.
  
  Compilation instruction:

  %h5cc -I/path/to/hdfeos5/include  -L/path/to/hdfeos5/lib -o \
  swath_1_3d_2x2yztd swath_1_3d_2x2yztd.c -lhe5_hdfeos -lGctp

  To generate the test file, run
  %./swath_1_3d_2x2yztd

  To view the test file, run
  %/path/to/hdf5/bin/h5dump swath_1_3d_2x2yztd.h5

  Copyright (C) 2012 The HDF Group
  
 */
#include <HE5_HdfEosDef.h>

herr_t write_attr(hid_t swid, char* field_name, char* attr_name, char* value);
herr_t write_dimension(hid_t swid);
herr_t write_field_1d(hid_t swid, char* field_name, char* dim_name, int geo);
herr_t write_field_2d(hid_t swid, char* field_name, int geo);
herr_t write_field_3d(hid_t swid, char* field_name, int geo);
herr_t write_swath(hid_t swfid, char* sname);

/* Write a local attribute. See HDF-EOS5 testdrivers/grid/TestGrid.c.  */
herr_t write_attr(hid_t swid, char* field_name, char* attr_name, char* value)
{
    hsize_t count[1];
    count[0] = strlen(value);
    return HE5_SWwritelocattr(swid, field_name, attr_name, H5T_NATIVE_CHAR,
                              count, value);
}

herr_t write_field_1d(hid_t swid, char* field_name, char* dim_name, int geo)
{
    int dim = 0;

    float val = 0.0;
    float var[2];
    
    hssize_t start[1];
    hsize_t count[1];

    start[0] = 0;
    count[0] = 2;


    while(dim < 2) {
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

    return HE5_SWwritefield(swid, field_name, start, NULL, count, var);

}

herr_t write_field_2d(hid_t swid, char* field_name, int geo)
{
    int ydim = 0;
    int xdim = 0;

    float val = 0.0;
    float var[4][8];
    
    hssize_t start[2];
    hsize_t count[2];

    start[0] = 0;
    start[1] = 0;
    count[0] = 4;
    count[1] = 8;

    while(ydim < 4) {
        while(xdim < 8) {
          var[ydim][xdim] = val;
          val = val + 1.0;
          xdim++;
        }
        ydim++;
        xdim = 0;
    }

    if(1 == geo)
        HE5_SWdefgeofield(swid, field_name, "YDim,XDim", NULL, 
                                 H5T_NATIVE_FLOAT, 0);
    if(0 == geo)
        HE5_SWdefdatafield(swid, field_name, "YDim,XDim", NULL, 
                                  H5T_NATIVE_FLOAT, 0);
    return HE5_SWwritefield(swid, field_name, start, NULL, count, var);
}

herr_t write_field_3d(hid_t swid, char* field_name, int geo)
{
    int ydim = 0;
    int xdim = 0;
    int zdim = 0;

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

    while(zdim < 2) {
        while(ydim < 4) {
            while(xdim < 8) {
                var[zdim][ydim][xdim] = val;
                val = val + 1.0;
                xdim++;
            }
            ydim++;
            xdim = 0;
        }
        zdim++;
        ydim = 0;
        xdim = 0;
    }

    if(1 == geo)
        HE5_SWdefgeofield(swid, field_name, "TDim,YDim,XDim", NULL, 
                                 H5T_NATIVE_FLOAT, 0);
    if(0 == geo)
        HE5_SWdefdatafield(swid, field_name, "TDim,YDim,XDim", NULL, 
                                  H5T_NATIVE_FLOAT, 0);
    return HE5_SWwritefield(swid, field_name, start, NULL, count, var);
}

herr_t write_dimension(hid_t swid)
{
    HE5_SWdefdim(swid, "YDim", 4);
    HE5_SWdefdim(swid, "XDim", 8);
    HE5_SWdefdim(swid, "ZDim", 2);
    HE5_SWdefdim(swid, "TDim", 2);
    return SUCCEED;
    
}

herr_t write_swath(hid_t swfid, char* sname)
{
 
    hid_t swid = FAIL;
  
     /* Create a Swath. */
    swid = HE5_SWcreate(swfid, sname);

    /* Define dimension. */
    write_dimension(swid);

    /* Define geolocation fields. */
    write_field_1d(swid, "Pressure", "ZDim",1);
    write_field_1d(swid, "Time", "TDim", 1);
    write_field_1d(swid, "Date", "TDim", 1);
    write_field_2d(swid, "Latitude", 1);
    write_field_2d(swid, "Longitude", 1);


    /* Define data fields. */
    write_field_3d(swid, "Temperature", 0);

    /* Write attributes. */
    write_attr(swid, "Pressure", "units", "hPa");
    write_attr(swid, "Time", "units", "seconds since 2012-1-1 0:0:0");
    write_attr(swid, "Date", "units", "days since 2012-1-1 0:0:0");
    write_attr(swid, "Latitude", "units", "degrees_north");
    write_attr(swid, "Longitude", "units", "degrees_east");
    write_attr(swid, "Temperature", "units", "K");

    /* Close the Swath. */
    return HE5_SWdetach(swid);

}
int main()
{
    herr_t status = FAIL;

    hid_t  swfid = FAIL;


    /* Create a file. */
    swfid = HE5_SWopen("swath_1_3d_2x2yztd.h5", H5F_ACC_TRUNC);

    /* Create a swath. */
    write_swath(swfid, "Swath");

    /* Close the file. */
    status = HE5_SWclose(swfid);
    
}
/* References */
/* [1] hdfeos5/samples/he5_sw_setup.c */
/* [2] hdfeos5/samples/he5_sw_definefields.c */
/* [3] hdfeos5/samples/he5_sw_writedata.c */
