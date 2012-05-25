/*
  A simple test program that generates one Grid, Swath, and ZA with
  2d variable in a single file.
  
  Compilation instruction:

  %h5cc -I/path/to/hdfeos5/include  -L/path/to/hdfeos5/lib -o  \
         grid_swath_za_1_2d                                    \
        -lhe5_hdfeos -lgctp                                    \
         grid_swath_za_1_2d.c

  To generate the test file, run

  %./grid_swath_za_1_2d

  Copyright (C) 2012 The HDF Group
  
 */
#include <HE5_HdfEosDef.h>
herr_t write_za_attr(hid_t zaid, char* field_name, char* attr_name, 
                     char* value);
herr_t write_za_dimension(hid_t zaid);
herr_t write_za_field_1d(hid_t zaid, char* field_name, char* dim_name, 
                         int size);
herr_t write_za_field_2d(hid_t zaid, char* field_name);
herr_t write_za(hid_t zafid, char* sname);
herr_t write_za_file(char* file_name);

herr_t write_swath_attr(hid_t swid, char* field_name, char* attr_name, 
                        char* value);
herr_t write_swath_dimension(hid_t swid);
herr_t write_swath_field_1d(hid_t swid, char* field_name, int geo, int size, 
                            char* dim_name, char* unit);
herr_t write_swath_field_2d(hid_t swid, char* field_name, int geo, 
                            char* dim_name, char* unit);
herr_t write_swath(hid_t swfid, char* sname);
herr_t write_swath_file(char* file_name);

herr_t write_attr(hid_t gdid, char* field_name, char* attr_name, char* value);
herr_t write_field_2d(hid_t gdid, char* field_name);
herr_t write_grid(hid_t gdfid, char* gname);
herr_t write_grid_file(char* file_name);

herr_t write_za_attr(hid_t zaid, char* field_name, char* attr_name, char* value)
{
    hsize_t count[1];
    count[0] = strlen(value);
    return HE5_ZAwritelocattr(zaid, field_name, attr_name, H5T_NATIVE_CHAR,
                              count, value);
}

herr_t write_za_dimension(hid_t zaid)
{
    HE5_ZAdefdim(zaid, "YDim", 8);
    HE5_ZAdefdim(zaid, "ZDim", 4);
    return SUCCEED;
    
}

herr_t write_za_field_1d(hid_t zaid, char* field_name, char* dim_name, int size)
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

herr_t write_za_field_2d(hid_t zaid, char* field_name)
{
    int dim0 = 0;
    int dim1 = 0;

    float val = 0.0;
    float var[4][8];
    
    hssize_t start[2];
    hsize_t count[2];

    start[0] = 0;
    start[1] = 0;
    count[0] = 4;
    count[1] = 8;


    while(dim1 < 4) {
        while(dim0 < 8) {
            var[dim1][dim0] = val;
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

herr_t write_za(hid_t zafid, char* sname)
{
 
    hid_t zaid = FAIL;
  
     /* Create a Swath. */
    zaid = HE5_ZAcreate(zafid, sname);

    /* Define dimension. */
    write_za_dimension(zaid);

    /* Define geolocation fields. */
    write_za_field_1d(zaid, "Pressure", "ZDim",  4);
    write_za_field_1d(zaid, "Latitude", "YDim", 8);

    /* Define data fields. */
    write_za_field_2d(zaid, "Temperature");

    /* Write attributes. */
    write_za_attr(zaid, "Pressure", "units", "hPa");
    write_za_attr(zaid, "Latitude", "units", "degrees_north");
    write_za_attr(zaid, "Temperature", "units", "K");

    /* Close the Swath. */
    return HE5_ZAdetach(zaid);
}

herr_t 
write_swath_attr(hid_t swid, char* field_name, char* attr_name, char* value)
{
    hsize_t count[1];
    count[0] = strlen(value);
    return HE5_SWwritelocattr(swid, field_name, attr_name, H5T_NATIVE_CHAR,
                              count, value);
}

herr_t write_swath_field_1d(hid_t swid, char* field_name, int geo, int size, 
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

    return write_swath_attr(swid, field_name, "units", unit);

}

herr_t write_swath_field_2d(hid_t swid, char* field_name, int geo, 
                            char* dim_name, char* unit)
{
    int dim1 = 0;
    int dim0 = 0;

    float val = 0.0;
    float var[4][8];
    
    hssize_t start[2];
    hsize_t count[2];

    start[0] = 0;
    start[1] = 0;
    count[0] = 4;
    count[1] = 8;

    while(dim1 < 4) {
        while(dim0 < 8) {
          var[dim1][dim0] = val;
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
    HE5_SWwritefield(swid, field_name, start, NULL, count, var);
    return write_swath_attr(swid, field_name, "units", unit);

}


herr_t write_swath_dimension(hid_t swid)
{
    HE5_SWdefdim(swid, "ZDim", 4);
    HE5_SWdefdim(swid, "NDim", 8);
    return SUCCEED;
}

herr_t write_swath(hid_t swfid, char* sname)
{
 
    hid_t swid = FAIL;
  
     /* Create a Swath. */
    swid = HE5_SWcreate(swfid, sname);

    /* Define dimension. */
    write_swath_dimension(swid);

    /* Define geolocation fields. */
    write_swath_field_1d(swid, "Pressure",  1, 4, "ZDim", "hPa");
    write_swath_field_1d(swid, "Latitude",  1, 8, "NDim", "degrees_north");
    write_swath_field_1d(swid, "Longitude", 1, 8, "NDim", "degrees_east" );

    /* Define data fields. */
    write_swath_field_2d(swid, "Temperature", 0, "ZDim,NDim", "K");

    /* Close the Swath. */
    return HE5_SWdetach(swid);

}

/* Write a local attribute. See HDF-EOS5 testdrivers/grid/TestGrid.c.  */
herr_t write_attr(hid_t gdid, char* field_name, char* attr_name, char* value)
{
    hsize_t count[1];
    count[0] = strlen(value);
    return HE5_GDwritelocattr(gdid, field_name, attr_name, H5T_NATIVE_CHAR,
                              count, value);
}

herr_t write_field_2d(hid_t gdid, char* field_name)
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
    return write_attr(gdid, field_name, "units", "K");
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
    write_field_2d(gdid, "Temperature");


    /* Close the Grid. */
    status = HE5_GDdetach(gdid);
    return status;
}

herr_t write_grid_file(char* file_name)
{
    herr_t status = FAIL;    
    hid_t  gdfid = FAIL;

    /* Create a file. */
    gdfid = HE5_GDopen(file_name, H5F_ACC_TRUNC);

    /* Write two identical grids except names. */
    write_grid(gdfid, "GeoGrid");

    /* Close the file. */
    status = HE5_GDclose(gdfid);
    return status;
}

herr_t write_swath_file(char* file_name)
{
    herr_t status = FAIL;    
    hid_t  swfid = FAIL;

    /* Open the existing file. */
    swfid = HE5_SWopen(file_name, H5F_ACC_RDWR);

    /* Create a Swath. */
    write_swath(swfid, "Swath");

    /* Close the file. */
    status = HE5_SWclose(swfid);
    
    return status;

}

herr_t write_za_file(char* file_name)
{
    herr_t status = FAIL;

    hid_t  zafid = FAIL;

    /* Open the existing file. */
    zafid = HE5_ZAopen(file_name, H5F_ACC_RDWR);

    /* Create a ZA. */
    write_za(zafid, "ZA");

    /* Close the file. */
    status = HE5_ZAclose(zafid);
    
    return status;
}

int main()
{
    write_grid_file("grid_swath_za_1_2d.h5");
    write_swath_file("grid_swath_za_1_2d.h5");
    write_za_file("grid_swath_za_1_2d.h5");
    return 0;
}

