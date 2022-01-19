/*
  A simple test program that generates one Swath with 3d swath, 2d lat/lon, 
  and 1d level.
  
  Compilation instruction:

  %h5cc -I/path/to/hdfeos5/include  -L/path/to/hdfeos5/lib -o \
  swath_2_3d_2x2yz swath_2_3d_2x2yz.c -lhe5_hdfeos -lGctp

  To generate the test file, run
  %./swath_2_3d_2x2yz

  To view the test file, run
  %/path/to/hdf5/bin/h5dump swath_2_3d_2x2yz.h5

  Copyright (C) 2012 The HDF Group
  
 */
#include <HE5_HdfEosDef.h>

herr_t write_attr(hid_t swid, char* field_name, char* attr_name, char* value);
herr_t write_dimension(hid_t swid, int zdim, int ydim, int xdim);
herr_t write_field_1d(hid_t swid, char* field_name, int geo, int size, 
                      char* dim_name, char* unit);
herr_t write_field_2d(hid_t swid, char* field_name, int geo, 
                      int ydim, int xdim, char* dim_name,  char* unit);
herr_t write_field_3d_dynamic(hid_t swid, char* field_name, int geo, int zdim, 
                              int ydim, int xdim, char* unit);
herr_t write_field_3d(hid_t swid, char* field_name, int geo, int zdim, 
                      int ydim, int xdim, char* unit);
herr_t write_swath(hid_t swfid, char* sname, int zdim, int ydim, int xdim);


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

herr_t write_field_2d(hid_t swid, char* field_name, int geo,
                      int ydim, int xdim,  char* dim_name,  char* unit)
{
    int i = 0;
    int dim0 = 0;
    int dim1 = 0;

    float var2[4][8];
    float var3[8][16];

    float val = -90.0;

    hssize_t start[2];
    hsize_t count[2];

    start[0] = 0;
    start[1] = 0;
    count[0] = (hsize_t)ydim;
    count[1] = (hsize_t)xdim;

    while(dim1 < ydim) {
        while(dim0 < xdim) {
            if(ydim == 4){
                var2[dim1][dim0] = val;
            }
            if(ydim == 8){
                var3[dim1][dim0] = val;
            }
            ++val;
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
    if(4 == ydim)
        HE5_SWwritefield(swid, field_name, start, NULL, count, var2);
    if(8 == ydim)
        HE5_SWwritefield(swid, field_name, start, NULL, count, var3);
    return write_attr(swid, field_name, "units", unit);

}

herr_t write_field_2d_dynamic(hid_t swid, char* field_name, int geo,
                              int ydim, int xdim,  char* dim_name,  char* unit)
{
    int i = 0;
    int dim0 = 0;
    int dim1 = 0;

    float val = 0.0;
    float **var = (float**) malloc(ydim * sizeof(float *));

    hssize_t start[2];
    hsize_t count[2];

    start[0] = 0;
    start[1] = 0;
    count[0] = (hsize_t)ydim;
    count[1] = (hsize_t)xdim;

    var[0] = (float*) malloc(ydim * xdim * sizeof(float));
    for(i=1; i < ydim; i++){
        var[i] = var[i-1] + xdim;
    }

    while(dim1 < ydim) {
        while(dim0 < xdim) {
            var[dim1][dim0] = val;
            printf("var[%d][%d]=%f, %p\n", dim1, dim0, 
                   var[dim1][dim0], &var[dim1][dim0]);
            ++val;
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
    free(var);

    return write_attr(swid, field_name, "units", unit);

}


herr_t write_field_3d(hid_t swid, char* field_name, int geo, int zdim, 
                      int ydim, int xdim, char* unit)
{
    int i = 0;
    int j = 0;
    int y = 0;
    int x = 0;
    int z = 0;

    float val = 0.0;

    float var[2][4][8];
    float var2[4][8][16];

    hssize_t start[3];
    hsize_t count[3];

    start[0] = 0;
    start[1] = 0;
    start[2] = 0;
    count[0] = zdim;
    count[1] = ydim;
    count[2] = xdim;


    while(z < zdim) {
        while(y < ydim) {
            while(x < xdim) {
                if(zdim == 2)
                    var[z][y][x] = val;
                if(zdim == 4)
                    var2[z][y][x] = val;                
                val = val + 1.0;
                x++;
            }
            y++;
            x = 0;
        }
        z++;
        y = 0;
        x = 0;
    }

    if(1 == geo)
        HE5_SWdefgeofield(swid, field_name, "ZDim,YDim,XDim", NULL, 
                                 H5T_NATIVE_FLOAT, 0);
    if(0 == geo)
        HE5_SWdefdatafield(swid, field_name, "ZDim,YDim,XDim", NULL, 
                                  H5T_NATIVE_FLOAT, 0);
    if(2 == zdim)
        HE5_SWwritefield(swid, field_name, start, NULL, count, var);    
    if(4 == zdim)
        HE5_SWwritefield(swid, field_name, start, NULL, count, var2);
    return write_attr(swid, field_name, "units", unit);
}

herr_t write_field_3d_dynamic(hid_t swid, char* field_name, int geo, int zdim, 
                              int ydim, int xdim, char* unit)
{
    int i = 0;
    int j = 0;
    int y = 0;
    int x = 0;
    int z = 0;

    float val = 0.0;

    float*** var = (float***)malloc(zdim*sizeof(float***)+
                                    zdim*ydim*sizeof(float**)+
                                    zdim*ydim*xdim*sizeof(float));
    float** var2 = (float**)(var + zdim);
    float* var3 = (float*)(var2 + zdim * ydim);
    for(i=0; i < zdim ; i++)
        var[i] = &var2[i*ydim];
    for(i=0; i < zdim; i++)
        for(j=0; j < ydim ; j++)
            var2[i*ydim + j] = &var3[i*ydim*xdim*sizeof(float) + 
                                     j*ydim*sizeof(float)];

      
   
    hssize_t start[3];
    hsize_t count[3];

    start[0] = 0;
    start[1] = 0;
    start[2] = 0;
    count[0] = zdim;
    count[1] = ydim;
    count[2] = xdim;


    while(z < zdim) {
        while(y < ydim) {
            while(x < xdim) {
                var[z][y][x] = val;
                val = val + 1.0;
                x++;
            }
            y++;
            x = 0;
        }
        z++;
        y = 0;
        x = 0;
    }

    if(1 == geo)
        HE5_SWdefgeofield(swid, field_name, "ZDim,YDim,XDim", NULL, 
                                 H5T_NATIVE_FLOAT, 0);
    if(0 == geo)
        HE5_SWdefdatafield(swid, field_name, "ZDim,YDim,XDim", NULL, 
                                  H5T_NATIVE_FLOAT, 0);
    HE5_SWwritefield(swid, field_name, start, NULL, count, var);
    free(var);
    return write_attr(swid, field_name, "units", unit);
}

herr_t write_dimension(hid_t swid, int zdim, int ydim, int xdim)
{
    HE5_SWdefdim(swid, "XDim", xdim);
    HE5_SWdefdim(swid, "YDim", ydim);
    HE5_SWdefdim(swid, "ZDim", zdim);
    return SUCCEED;
    
}

herr_t write_swath(hid_t swfid, char* sname, int zdim, int ydim, int xdim)
{
 
    hid_t swid = FAIL;
  
     /* Create a Swath. */
    swid = HE5_SWcreate(swfid, sname);

    /* Define dimension. */
    write_dimension(swid, zdim, ydim, xdim);

    /* Define geolocation fields. */
    write_field_1d(swid, "Pressure", 1, zdim, "ZDim", "hPa");
    write_field_2d(swid, "Latitude", 1, ydim, xdim, "YDim,XDim", 
                   "degrees_north");
    write_field_2d(swid, "Longitude", 1, ydim, xdim, "YDim,XDim", 
                   "degrees_east");


    /* Define data fields. */
    write_field_3d(swid, "Temperature", 0, zdim, ydim, xdim, "K");

    /* Close the Swath. */
    return HE5_SWdetach(swid);

}
int main()
{
    herr_t status = FAIL;

    hid_t  swfid = FAIL;


    /* Create a file. */
    swfid = HE5_SWopen("swath_2_3d_2x2yz.h5", H5F_ACC_TRUNC);

    /* Create a swath. */
    write_swath(swfid, "Swath1", 2,4,8);
    write_swath(swfid, "Swath2", 4,8,16);

    /* Close the file. */
    status = HE5_SWclose(swfid);
    
}
/* References */
/* [1] hdfeos5/samples/he5_sw_setup.c */
/* [2] hdfeos5/samples/he5_sw_definefields.c */
/* [3] hdfeos5/samples/he5_sw_writedata.c */
