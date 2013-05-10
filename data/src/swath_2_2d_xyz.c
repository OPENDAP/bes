/*
  A simple test program that generates one Swath with 2d variable.
  
  Compilation instruction:

  %h4cc -I/path/to/hdfeos/include  -L/path/to/hdfeos/lib -o swath_2_2d_xyz \
        -lhdfeos -lgctp swath_2_2d_xyz.c

  To generate the test file, run

  %./swath_2_2d_xyz

  Copyright (C) 2012 The HDF Group
  
 */

#include <stdio.h>
#include <mfhdf.h>
#include <HdfEosDef.h>

intn write_dimension(int32 swid, int xdim, int ydim);
intn 
write_field_1d(int32 swid, char* field_name, int geo, int size, char* dim_name);

intn 
write_field_2d(int32 swid, char* field_name, int geo, int xdim, int ydim, char* dim_name);

intn 
write_swath(int32 swfid, char* sname, int xdim, int ydim);

intn write_dimension(int32 swid, int xdim, int ydim)
{
    char dim1[] = "ZDim";
    char dim2[] = "NDim";

    SWdefdim(swid, dim1, xdim);
    SWdefdim(swid, dim2, ydim);
    return SUCCEED;
}

intn 
write_field_1d(int32 swid, char* field_name, int geo, int size, char* dim_name)
{
    int dim = 0;

    intn status = 0;

    float val = 0.0;
    float* var = NULL;
    
    int32 start[1];
    int32 count[1];

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
        SWdefgeofield(swid, field_name, dim_name,
                                 DFNT_FLOAT32, HDFE_NOMERGE);
    if(0 == geo)
        SWdefdatafield(swid, field_name, dim_name, 
                                  DFNT_FLOAT32, HDFE_NOMERGE);
    status = SWwritefield(swid, field_name, start, NULL, count, var);
    if(status == -1){
        fprintf(stderr, "SWwritefield() failed.\n");
	return -1;        
    }

    if(var != NULL)
        free(var);

    return status;

}

intn write_field_2d(int32 swid, char* field_name, int geo, int xdim, int ydim, char* dim_name)
{
    int i = 0;
    intn status = 0;

    float32 *temp = (float32*)malloc(xdim*ydim*sizeof(float32)); 
    
    int32 edge[2];
    int32 start[2];

    /* Fill data. */
    for (i=0; i<xdim*ydim; i++)
	temp[i] = i+1;
        
    start[0] = 0; 
    start[1] = 0;
    edge[0] = xdim; /* latitude-ydim first */ 
    edge[1] = ydim;

    /* Create a field. */
    if(geo == 1)
        SWdefgeofield(swid, field_name, dim_name, DFNT_FLOAT32, 0); 
    else
        SWdefdatafield(swid, field_name, dim_name, DFNT_FLOAT32, 0); 

    /* status = SWwritefield(swid, field_name, start, NULL, edge, temp); */
    status = SWwritefield(swid, field_name, NULL, NULL, NULL, temp); 
    if(status == -1){
        fprintf(stderr, "SWwritefield() failed.\n");
	return -1;        
    }
    /* In HDFEOS2, you can't write attribute on the individual field. */

    if(temp != NULL)
	free(temp);

    return status;
}

intn write_swath(int32 swfid, char* sname, int xdim, int ydim)
{
    char field_name[]= "temperature";
    char field_dim_name[]= "ZDim,NDim";
    char geo_name[]= "pressure";
    char geo_name_lon[]= "Longitude";
    char geo_name_lat[]= "Latitude";
    char geo_dim_name[]= "ZDim";
    char geo_dim_name2[]= "NDim";

    int swid = 0;

    swid = SWcreate(swfid, sname);
    if(swid == -1)  {
	fprintf(stderr, "SWcreate() failed.\n");
	return -1;        
    }
    /* Define dimension. */
    write_dimension(swid, xdim, ydim);

    /* Define geolocaiton fields. */
    write_field_1d(swid, geo_name, 1, xdim, geo_dim_name);
    write_field_1d(swid, geo_name_lat, 1, ydim, geo_dim_name2);
    write_field_1d(swid, geo_name_lon, 1, ydim, geo_dim_name2);

    /* Write field. */
    write_field_2d(swid, field_name, 0, xdim, ydim, field_dim_name);

    /* Close the Grid. */
    SWdetach(swid);
    return 0;
}

int main(int argc, char *argv[])
{
    intn status = 0;
    char filename[]= "swath_2_2d_xyz.hdf";
    char swathname[]= "Swath";

    /* Create an HDF-EOS2 file. */
    int32 swfid = SWopen(filename, DFACC_RDWR);
    if(swfid == -1){
        fprintf(stderr, "SWopen() failed.\n");
        return 1;
    }

    /* Create a swath. */
    write_swath(swfid, "Swath1", 4, 8);
    write_swath(swfid, "Swath2", 8, 16);

    /* Close the HDF-EOS2 file. */
    status= SWclose(swfid);
    if (status == -1) {
        fprintf(stderr, "SWclose() failed.\n");
        return 1;
    }

    return 0;
}
