/*
  A simple test program that generates two Grids that have different sizes.
  
  To compile this program, run

  %h5cc -I/path/to/hdfeos5/include -L/path/to/hdfeos5/lib -o grid_2_2d_size.c \
        -lhe5_hdfeos -lgctp

  To generate the test file, run

  %./grid_2_2d_size

  Copyright (C) 2012 The HDF Group
  
*/
#include <HE5_HdfEosDef.h>

herr_t write_attr(hid_t gdid, char* field_name, char* attr_name, char* value);
herr_t write_field_2d(hid_t gdid, char* field_name, long xdim, long ydim);
herr_t write_field_2d_dynamic(hid_t gdid, char* field_name, long xdim, long ydim);
herr_t write_grid(hid_t gdfid, char* gname, long xdim, long ydim);

/* Write a local attribute. See HDF-EOS5 testdrivers/grid/TestGrid.c.  */
herr_t write_attr(hid_t gdid, char* field_name, char* attr_name, char* value)
{
    hsize_t count[1];
    count[0] = strlen(value);
    return HE5_GDwritelocattr(gdid, field_name, attr_name, H5T_NATIVE_CHAR,
                              count, value);
    
    
}

herr_t write_field_2d(hid_t gdid, char* field_name, long xdim, long ydim)
{
    int i = 0;
    int j = 0;

    int x = 0;
    int y = 0;

    float temp[4][8];
    float temp2[8][16];
    float val = 0.0;

    hsize_t edge[2];
    hssize_t start[2];

    x = (int) xdim;
    y = (int) ydim;

    /* Fill data. */
    for (i=0; i < y; i++){
        for(j=0; j < x; j++){
            if(xdim == 8)
                temp[i][j] = val;
            if(xdim == 16)
                temp2[i][j] = val;
            ++val; 
        }
    }

    start[0] = 0; 
    start[1] = 0;
    edge[0] = (hsize_t) ydim;
    edge[1] = (hsize_t) xdim;

    /* Create a field. */
    HE5_GDdeffield(gdid, field_name, "YDim,XDim", NULL, 
                   H5T_NATIVE_FLOAT, 0);
    
    if(xdim == 8)
        HE5_GDwritefield(gdid, field_name, start, NULL, edge, temp);
    if(xdim == 16)
        HE5_GDwritefield(gdid, field_name, start, NULL, edge, temp2);

    /* Write attribute. */
    return write_attr(gdid, field_name, "units", "K");
}

/* This one doesn't work with HDF-EOS5 library. We need to check it later. */
herr_t write_field_2d_dynamic(hid_t gdid, char* field_name, long xdim, long ydim)
{
    int i = 0;
    int j = 0;

    int x = 0;
    int y = 0;

    float **temp = NULL;

    hsize_t edge[2];
    hssize_t start[2];

    x = (int) xdim;
    y = (int) ydim;

    /* Allocate memory */
    temp = malloc(y * sizeof(float *));
    if(temp == NULL)
        fprintf(stderr, "Out of Memory\n");
    for (i=0; i < y; i++){
        temp[i] = malloc(x * sizeof(float));
        if(temp[i] == NULL){
            fprintf(stderr, "Out of Memory\n");
        }
    }

    /* Fill data. */
    for (i=0; i < y; i++)
        for(j=0; j < x; j++)
            temp[i][j] = (float)(10 + i);

    start[0] = 0; 
    start[1] = 0;
    edge[0] = (hsize_t) ydim;
    edge[1] = (hsize_t) xdim;

    /* Create a field. */
    HE5_GDdeffield(gdid, field_name, "YDim,XDim", NULL, 
                   H5T_NATIVE_FLOAT, 0);
    

    HE5_GDwritefield(gdid, field_name, start, NULL, edge, temp);

    for(i=0; i < y; i++){
        free(temp[i]);
    }
    free(temp);

    /* Write attribute. */
    return write_attr(gdid, field_name, "units", "K");
}

herr_t write_grid(hid_t gdfid, char* gname, long xdim, long ydim)
{
    double upleft[2];
    double lowright[2];

    herr_t status = FAIL;    

    hid_t  gdid = FAIL;
    
    int dummy = 0;


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
    write_field_2d(gdid, "Temperature", xdim, ydim);


    /* Close the Grid. */
    status = HE5_GDdetach(gdid);
    return status;
}

int main()
{
    herr_t status = FAIL;    
    hid_t  gdfid = FAIL;

    /* Create a file. */
    gdfid = HE5_GDopen("grid_2_2d_size.h5", H5F_ACC_TRUNC);

    /* Write two identical grids except names. */
    write_grid(gdfid, "GeoGrid1", 16, 8);
    write_grid(gdfid, "GeoGrid2", 8, 4);

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
