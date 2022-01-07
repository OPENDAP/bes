/*
  A simple test program that generates one Swath with 2d variable.
  
  Compilation instruction:

  %h4cc -I/path/to/hdfeos/include  -L/path/to/hdfeos/lib -o swath_1_2d_xy_dim_mismatch.c -lhdfeos -lgctp swath_1_2d_xy_dim_mismatch

  To generate the test file, run

  %./swath_1_2d_xy_dim_mismatch

  Copyright (C) 2012 The HDF Group
  
 */

#include <stdio.h>
#include <mfhdf.h>
#include <HdfEosDef.h>

intn write_dimension(int32 swid);
intn 
write_field_1d(int32 swid, char* field_name, int geo, int size, char* dim_name);

intn 
write_field_2d(int32 swid, char* field_name, int geo, char* dim_name);

intn 
write_swath(int32 swfid, char* sname);

intn write_dimension(int32 swid)
{
    char dim[] = "NDim";
    char udim[] = "Unlim";

    if(SWdefdim(swid, udim, 0) != SUCCEED) {
        fprintf(stderr, "%d:SWdefdim() failed.\n", __LINE__);
        return FAIL;
    };

    if(SWdefdim(swid, dim, 8) != SUCCEED) {
        fprintf(stderr, "%d:SWdefdim() failed.\n", __LINE__);
        return FAIL;
    };
    return SUCCEED;
}

intn 
write_field_1d(int32 swid, char* field_name, int geo, int size, char* dim_name)
{
    int dim = 0;

    intn status = SUCCEED;

    float val = 0.0;
    float* var = NULL;
    
    int32 start[1];
    int32 count[1];

    start[0] = 0;
    count[0] = size;

    var = (float*) malloc(size * sizeof(float));
    if(var == NULL){
        fprintf(stderr, "%d:Out of Memory\n",__LINE__);
        return FAIL;
    }

    while(dim < size) {
        var[dim] = val;
        val = val + 1.0;
        dim++;
    }

    if(1 == geo){
        status = SWdefgeofield(swid, field_name, dim_name,
                               DFNT_FLOAT32, HDFE_NOMERGE);
        if(status == FAIL){
            fprintf(stderr, "%d:SWdefgeofield() faield.\n", __LINE__);
            return status;
        }
    }
    if(0 == geo){
        status = SWdefdatafield(swid, field_name, dim_name, 
                                  DFNT_FLOAT32, HDFE_NOMERGE);
        if(status == FAIL){
            fprintf(stderr, "%d:SWdefdatafield() faield.\n", __LINE__);
            return status;
        }        
    }
    status = SWwritefield(swid, field_name, start, NULL, count, var);
    if(status == FAIL){
        fprintf(stderr, "%d:SWwritefield() failed.\n", __LINE__);
	return status;
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

    float32 temp[2][8];          /* longitude-xdim first. */

    int32 start[2], edge[2];

    /* Fill data. */
    for (i=0; i < 2; i++)
        for(j=0; j < 8; j++)
            temp[i][j] = (float32)(10 + i);

    start[0] = 0; 
    start[1] = 0;
    edge[0] = 2;
    edge[1] = 8;

    /* Create a field. */
    if(geo == 1) {
        status = SWdefgeofield(swid, field_name, dim_name, DFNT_FLOAT32, 0); 
        if(status == FAIL){
            fprintf(stderr, "%d:SWdefgeofield() failed.\n", __LINE__);
            return FAIL;        
        }
    }
    else {
        status = SWdefdatafield(swid, field_name, dim_name, DFNT_FLOAT32, HDFE_NOMERGE); 
        if (status == FAIL) {
            fprintf(stderr, "%d:SWdefdatafield() failed.\n", __LINE__);
            return FAIL;
        }
    }

    status = SWwritefield(swid, field_name, start, NULL, edge, temp); 
    if(status == FAIL){
        fprintf(stderr, "%d:SWwritefield() failed.\n", __LINE__);
	return FAIL;        
    }

    return status;
}

intn write_swath(int32 swfid, char* sname)
{
    char field_name[] = "temperature";
    char field_dim_name[] = "Unlim,NDim";
    char geo_name_lon[] = "Longitude";
    char geo_name_lat[] = "Latitude";
    char geo_dim_name[] = "NDim";

    int swid = 0;

    intn status = SUCCEED;

    swid = SWcreate(swfid, sname);
    if(swid == -1)  {
	fprintf(stderr, "%d:SWcreate() failed.\n", __LINE__);
	return FAIL;        
    }
    /* Define dimension. */
    write_dimension(swid);

    /* Define geolocaiton fields. */
    write_field_1d(swid, geo_name_lat, 1, 8, geo_dim_name);
    write_field_1d(swid, geo_name_lon, 1, 8, geo_dim_name);

    /* Write field. */
    write_field_2d(swid, field_name, 0, field_dim_name);

    /* Close the Grid. */
    status =SWdetach(swid);
    if(status == FAIL) {
        fprintf(stderr, "%d:SWdetach() failed.\n", __LINE__);
    }
    
    return status;
}

int main(int argc, char *argv[])
{
    intn status = 0;
    char filename[]= "swath_1_2d_xy_dim_mismatch.hdf";
    char swathname[]= "Swath";

    /* Create an HDF-EOS2 file. */
    int32 swfid = SWopen(filename, DFACC_RDWR);
    if(swfid == -1){
        fprintf(stderr, "%d:SWopen() failed.\n", __LINE__);
        return 1;
    }

    /* Create a swath. */
    write_swath(swfid, swathname);

    /* Close the HDF-EOS2 file. */
    status= SWclose(swfid);
    if (status == -1) {
        fprintf(stderr, "%d:SWclose() failed.\n", __LINE__);
        return 1;
    }

    return 0;
}
