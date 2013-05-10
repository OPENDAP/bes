/*
  A simple test program that generates one Swath with 2d variable.
  
  Compilation instruction:

  %h4cc -I/path/to/hdfeos/include  -L/path/to/hdfeos/lib -o h_swath \
        -lhdfeos -lgctp h_swath.c

  To generate the test file, run

  %./h_swath

  Copyright (C) 2012 The HDF Group
  
 */

#include <stdio.h>
#include <mfhdf.h>
#include <hdf.h>
#include <HdfEosDef.h>

intn  write_sds(char *fname);

intn write_dimension(int32 swid);
intn 
write_field_1d(int32 swid, char* field_name, int geo, int size, char* dim_name);

intn 
write_field_2d(int32 swid, char* field_name, int geo, char* dim_name);

intn 
write_swath(int32 swfid, char* sname);

intn write_sds(char *fname)
{
    int32 sd_id, sds_id;
    int32 dim_sizes[2] = {4,8};
    int32 start[2], edges[2];
    float32 data[4][8];
    int   i, j;
    intn status;

    /*
     * Open the file and initialize the SD interface.
     */
    sd_id = SDstart (fname, DFACC_WRITE);
    if (sd_id == FAIL) {
        fprintf(stderr, "SDstart() failed.\n");
        return -1;
    }

    sds_id = SDcreate (sd_id, "temperature", DFNT_FLOAT32, 2, dim_sizes);
   
    /*
     * Data set data initialization.
     */
    for (j = 0; j < 4; j++) {
       for (i = 0; i < 8; i++)
            data[j][i] = (i + j) + 1;
    }

    /*
     * Define the location and size of the data to be written to the data set.
     */
    start[0] = 0;
    start[1] = 0;
    edges[0] = 4;
    edges[1] = 8;

    status = SDwritedata (sds_id, start, NULL, edges, (VOIDP)data);

    status = SDendaccess(sds_id);
    status = SDend(sd_id);

    return 0;
}

intn write_dimension(int32 swid)
{
    char dim1[] = "ZDim";
    char dim2[] = "NDim";

    SWdefdim(swid, dim1, 4);
    SWdefdim(swid, dim2, 8);
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

intn write_field_2d(int32 swid, char* field_name, int geo, char* dim_name)
{
    int i = 0;
    int j = 0;
    intn status = 0;

    float32 temp[4][8];          /* longitude-xdim first. */

    int32 edge[2];
    int32 start[2];

    /* Fill data. */
    for (i=0; i < 4; i++)
        for(j=0; j < 8; j++)
            temp[i][j] = (float32)(10 + i);

    start[0] = 0; 
    start[1] = 0;
    edge[0] = 4; /* latitude-ydim first */ 
    edge[1] = 8;

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

    return status;
}

intn write_swath(int32 swfid, char* sname)
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
    write_dimension(swid);

    /* Define geolocaiton fields. */
    write_field_1d(swid, geo_name, 1, 4, geo_dim_name);
    write_field_1d(swid, geo_name_lat, 1, 8, geo_dim_name2);
    write_field_1d(swid, geo_name_lon, 1, 8, geo_dim_name2);

    /* Write field. */
    write_field_2d(swid, field_name, 0, field_dim_name);

    /* Close the Grid. */
    SWdetach(swid);
    return 0;
}

int main(int argc, char *argv[])
{
    intn status = 0;
    char filename[]= "h_swath.hdf";
    char swathname[]= "Swath";

    /* Create an HDF-EOS2 file. */
    int32 swfid = SWopen(filename, DFACC_RDWR);
    if(swfid == -1){
        fprintf(stderr, "SWopen() failed.\n");
        return 1;
    }

    /* Create a swath. */
    write_swath(swfid, swathname);


    /* Close the HDF-EOS2 file. */
    status= SWclose(swfid);
    if (status == -1) {
        fprintf(stderr, "SWclose() failed.\n");
        return 1;
    }

    /* Write SDS using generic HDF4 SDS interface. */
    write_sds(filename);

    return 0;
}
