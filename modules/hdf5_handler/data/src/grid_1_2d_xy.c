/*
  A simple test program that generates one Geographic projection Grid with
  2d variable and lat/lon.
  
  Compilation instruction:

  %h5cc -I/path/to/hdfeos5/include  -L/path/to/hdfeos5/lib -o grid_1_2d_xy \
        -lhe5_hdfeos -lgctp grid_1_2d_xy.c

  To generate the test file, run

  %./grid_1_2d_xy

  Copyright (C) 2012 The HDF Group
  
 */
#include <HE5_HdfEosDef.h>

herr_t write_attr(hid_t gdid, char* field_name, char* attr_name, char* value);
herr_t write_field_1d(hid_t gdid, char* field_name, char* dim_name, int size,
                      char* unit);
herr_t write_field_2d(hid_t gdid, char* field_name, char* unit);
herr_t write_grid(hid_t gdfid, char* gname);

/* Write a local attribute. See HDF-EOS5 testdrivers/grid/TestGrid.c.  */
herr_t write_attr(hid_t gdid, char* field_name, char* attr_name, char* value)
{
    hsize_t count[1];
    count[0] = strlen(value);
    return HE5_GDwritelocattr(gdid, field_name, attr_name, H5T_NATIVE_CHAR,
                              count, value);
    
    
}

herr_t write_field_1d(hid_t gdid, char* field_name, char* dim_name, int size,
                      char* unit)
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

    HE5_GDdeffield(gdid, field_name, dim_name, NULL, H5T_NATIVE_FLOAT, 0);
    HE5_GDwritefield(gdid, field_name, start, NULL, count, var);

    if(var != NULL)
        free(var);
    return write_attr(gdid, field_name, "units", unit);
}

herr_t write_field_2d(hid_t gdid, char* field_name, char* unit)
{
    int i = 0;
    int j = 0;

    float temp[4][8];          /* longitude-xdim first. */

    hsize_t edge[2];
    hssize_t start[2];

    /* Fill data. */
    for (i=0; i < 4; i++)
        for(j=0; j < 8; j++)
            temp[i][j] = (float)(10 + i);

    start[0] = 0; 
    start[1] = 0;
    edge[0] = 4; /* latitude-ydim first */ 
    edge[1] = 8;

    /* Create a field. */
    HE5_GDdeffield(gdid, field_name, "YDim,XDim", NULL, 
                   H5T_NATIVE_FLOAT, 0);
    

    HE5_GDwritefield(gdid, field_name, start, NULL, edge, temp);
    /* Write attribute. */
    return write_attr(gdid, field_name, "units", unit);
}

herr_t write_grid(hid_t gdfid, char* gname)
{
    double upleft[2];
    double lowright[2];

    herr_t status = FAIL;    

    hid_t  gdid = FAIL;
    
    int dummy = 0;

    long xdim = 8;
    long ydim = 4;


    /* Set corner points. */
    upleft[0] = HE5_EHconvAng(0.,  HE5_HDFE_DEG_DMS); /* left */
    upleft[1]  = HE5_EHconvAng((float)ydim, HE5_HDFE_DEG_DMS); /* up */
    lowright[0] = HE5_EHconvAng((float)xdim, HE5_HDFE_DEG_DMS); /* right */
    lowright[1] = HE5_EHconvAng(0., HE5_HDFE_DEG_DMS);          /* low */


    /* Create Grids. */
    gdid  = HE5_GDcreate(gdfid, gname, xdim, ydim, upleft, lowright);

    /* Set projection. */
    status = HE5_GDdefproj(gdid, HE5_GCTP_GEO, dummy, dummy, NULL);
	
    /* Write field. */
    write_field_2d(gdid, "temperature", "K");

    /* Write  lat/lon. */
    write_field_1d(gdid, "Longitude", "XDim", 8, "degrees_east");
    write_field_1d(gdid, "Latitude", "YDim", 4, "degrees_north");

    /* Close the Grid. */
    status = HE5_GDdetach(gdid);
    return status;
}

int main()
{
    herr_t status = FAIL;    
    hid_t  gdfid = FAIL;

    /* Create a file. */
    gdfid = HE5_GDopen("grid_1_2d_xy.h5", H5F_ACC_TRUNC);

    /* Write two identical grids except names. */
    write_grid(gdfid, "GeoGrid");

    /* Close the file. */
    status = HE5_GDclose(gdfid);
    return 0;
}

/* 
References 
 [1] hdfeos5/samples/he5_gd_setup.c 
 [2] hdfeos5/samples/he5_gd_definefields.c 
 [3] hdfeos5/samples/he5_gd_writedata.c
*/
