/*
  A simple test program that generates one Geographic projection Grid with
  2d variable.
  
  Compilation instruction:

  %h4cc -I/path/to/hdfeos/include  -L/path/to/hdfeos/lib -o grid_1_3d_xy \
        -lhdfeos -lgctp grid_1_3d_xy.c

  To generate the test file, run

  %./grid_1_3d_xy

  Copyright (C) 2012 The HDF Group
  
 */

#include <stdio.h>
#include <mfhdf.h>
#include <HdfEosDef.h>

intn  write_grid(int32 gdfid, char* gname);
intn  write_field_3d(int32 gdid, char* field_name);
intn  write_attr(char *fname);

intn write_attr(char *fname)
{
    int32 sd_id, sds_id;
    intn status;

    /*
     * Open the file and initialize the SD interface.
     */
    sd_id = SDstart (fname, DFACC_WRITE);
    if (sd_id == FAIL) {
    	fprintf(stderr, "SDstart() failed.\n");
        return -1;
    }

    /* Get the identifier for the first data set. */
    sds_id = SDselect(sd_id, 0);
    if (sds_id == FAIL) {
	fprintf(stderr, "SDselect() failed.\n");
        return -1;
    }

    /* Set an attribute. */
    status  = SDsetattr(sds_id, "units", DFNT_CHAR8, 13,  (VOIDP)"degrees_north");
    if (status == FAIL) {
	fprintf(stderr, "SDsetattr() failed.\n");
        return -1;
    } 

    /*
     * Terminate access to the data set.
     */
    status = SDendaccess (sds_id);

    /* Get the identifier for the first data set. */
    sds_id = SDselect(sd_id, 1);
    if (sds_id == FAIL) {
        fprintf(stderr, "SDselect() failed.\n");
        return -1;
    }

    /* Set an attribute. */
    status  = SDsetattr(sds_id, "units", DFNT_CHAR8, 12,  (VOIDP)"degrees_east");
    if (status == FAIL) {
        fprintf(stderr, "SDsetattr() failed.\n");
        return -1;
    }

    /*
     * Terminate access to the data set.
     */
    status = SDendaccess (sds_id);

    /*
     * Terminate access to the SD interface and close the file.
     */
    status = SDend (sd_id);

    return 0;    
}

intn write_field_3d(int32 gdid, char* field_name)
{
    char dim_name[]= "ZDim,YDim,XDim";
    int i = 0;
    int j = 0;
    int k = 0;
    intn status = 0;

    float32 temp[2][4][8];          /* longitude-xdim first. */
    float32 temp1[4] = {1,2,3,4};
    float32 temp2[8] = {1,2,3,4,5,6,7,8};

    int32 edge[3];
    int32 start[3];

    /* Fill data. */
    for (i=0; i < 8; i++)
        for(j=0; j < 2; j++)
	   for(k=0; k < 4; k++)
               temp[j][k][i] = (float32)(i + j + k);

    start[0] = 0; 
    start[1] = 0;
    start[2] = 0;
    edge[0] = 2; /* latitude-ydim first */ 
    edge[1] = 4;
    edge[2] = 8;

    /* Create a field. */
    GDdeffield(gdid, "Latitude", "YDim", DFNT_FLOAT32, 0);
    GDdeffield(gdid, "Longitude", "XDim", DFNT_FLOAT32, 0);
    GDdeffield(gdid, field_name, dim_name, DFNT_FLOAT32, 0); 

    /* status = GDwritefield(gdid, field_name, start, NULL, edge, temp); */
    status = GDwritefield(gdid, field_name, NULL, NULL, NULL, temp); 
    if(status == -1){
        fprintf(stderr, "GDwritefield(...,\"temperature\",...) failed.\n");
	return -1;        
    }

    status = GDwritefield(gdid, "Latitude", NULL, NULL, NULL, temp1);
    if(status == -1){
        fprintf(stderr, "GDwritefield(...,\"lat\",...) failed.\n");
        return -1;
    }

    status = GDwritefield(gdid, "Longitude", NULL, NULL, NULL, temp2);
    if(status == -1){
        fprintf(stderr, "GDwritefield(...,\"lon\",...) failed.\n");
        return -1;
    }

    /* Write attribute. */
    /*  return write_attr(gdid, field_name, "units", "K"); */
    return 0;
}

intn write_grid(int32 gdfid, char* gname)
{
    char fieldname[]= "temperature";

    int GDid = 0;
    int32 xdim=8, ydim=4;
    intn status = 0;

    float64  upleft[2], lowright[2];

    /* Upper Left and Lower Right points */
    upleft[0] = EHconvAng(0.,  HDFE_DEG_DMS); /* left */
    upleft[1] = EHconvAng((float)ydim, HDFE_DEG_DMS); /* up */
    lowright[0] = EHconvAng((float)xdim, HDFE_DEG_DMS); /* right */
    lowright[1] = EHconvAng(0., HDFE_DEG_DMS);          /* low */

    GDid = GDcreate(gdfid, gname, xdim, ydim, upleft, lowright);
    if(GDid == -1)  {
	fprintf(stderr, "GDcreate() failed.\n");
	return -1;        
    }
    status = GDdefproj(GDid, GCTP_GEO, -1, -1, NULL);
    if(status == -1){
	fprintf(stderr, "GDdefproj() failed.\n");
	return -1;                
    }
    status = GDdeforigin(GDid, HDFE_GD_UR);
    if(status == -1){
	fprintf(stderr, "GDdeforigin() failed.\n");
	return -1;                
    }
   
    /* Define "ZDim" Dimension */
    status = GDdefdim(GDid, "ZDim", 2);
    if (status == -1)
    {
        fprintf(stderr, "GDdefdim() failed.\n");
        return -1;
    }
 
    /* Write field. */
    write_field_3d(GDid, fieldname);

    /* Close the Grid. */
    GDdetach(GDid);
    return 0;
}

int main(int argc, char *argv[])
{
    intn status = 0;
    char filename[]= "grid_1_3d_xy.hdf";
    char gridname[]= "GeoGrid";

    /* Create an HDF-EOS2 file. */
    int32 gdfid = GDopen(filename, DFACC_RDWR);
    if(gdfid == -1){
        fprintf(stderr, "GDopen() failed.\n");
        return 1;
    }

    /* Write two identical grids except names. */
    write_grid(gdfid, gridname);


    /* Close the HDF-EOS2 file. */
    status= GDclose(gdfid);
    if (status == -1) {
        fprintf(stderr, "GDclose() failed.\n");
        return 1;
    }
  
    /* Write attriubtes using generic HDF4 SDS interface*/
    write_attr(filename);

    return 0;
}
