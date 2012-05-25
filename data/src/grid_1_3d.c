/*
  A simple test program that generates one Geographic porjection Grid with 
  3d variable.
  
  Compilation instruction:

  %h5cc -I/path/to/hdfeos5/include  -L/path/to/hdfeos5/lib -o grid_1_3d \
  -lhe5_hdfeos -lgctp grid_1_3d.c

  To generate the test file, run
  %./grid_1_3d

  Copyright (C) 2012 The HDF Group
  
 */
#include <HE5_HdfEosDef.h>

int main()
{
    double upleft[2];
    double lowright[2];

    float temp[2][4][8];   
    float val = 0.0;

    herr_t status = FAIL;

    hid_t  gdfid = FAIL;
    hid_t  gdid = FAIL;

    hsize_t count[1];
    hsize_t edge[3];

    hssize_t start[3];
    
    int dummy = 0;
    int i = 0;
    int j = 0;
    int k = 0;

    long xdim = 8;
    long ydim = 4;
    long zdim = 2;

    /* Set corner points. */
    upleft[0] = HE5_EHconvAng(0.,  HE5_HDFE_DEG_DMS); /* left */
    upleft[1]  = HE5_EHconvAng((float)ydim, HE5_HDFE_DEG_DMS); /* up */
    lowright[0] = HE5_EHconvAng((float)xdim, HE5_HDFE_DEG_DMS); /* right */
    lowright[1] = HE5_EHconvAng(0., HE5_HDFE_DEG_DMS);          /* low */

    /* Fill data. */
    for(i=0; i < zdim; i++)
        for (j=0; j < ydim; j++)
            for(k=0; k < xdim; k++){
                temp[i][j][k] = val;
                ++val;
            }

    /* Create a file. */
    gdfid = HE5_GDopen("grid_1_3d.h5", H5F_ACC_TRUNC);

    /* Create a Grid. */
    gdid  = HE5_GDcreate(gdfid, "GEOGrid", xdim, ydim, upleft, lowright);

    /* Set projection. */
    status = HE5_GDdefproj(gdid, HE5_GCTP_GEO, dummy, dummy, NULL);

    /* Define "Level" Dimension */
    status = HE5_GDdefdim(gdid, "ZDim", zdim);

    /* Create a field. */
    status = HE5_GDdeffield(gdid, "temperature", "ZDim,YDim,XDim", NULL, 
                            H5T_NATIVE_FLOAT, 0);
    /* Write field. */
    start[0] = 0; 
    start[1] = 0;
    start[2] = 0;
    edge[0] = 2; 
    edge[1] = 4;
    edge[2] = 8;
    status = HE5_GDwritefield(gdid, "temperature", start, NULL, edge, temp);
    /* Write a local attribute. See HDF-EOS5 testdrivers/grid/TestGrid.c.  */
    count[0] = 1;
    status = HE5_GDwritelocattr(gdid, "temperature", "units", H5T_NATIVE_CHAR, 
                                count, "K");


    /* Close the Grid. */
    status = HE5_GDdetach(gdid);

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
