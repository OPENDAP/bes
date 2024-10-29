/*
  A simple test program that generates two Grids that have different sizes.
  
  To compile this program, run

  %h4cc -I/path/to/hdfeos5/include -L/path/to/hdfeos2/lib -o grid_2_2d_size_ps.c \
        -lhdfeos -lgctp

  To generate the test file, run

  %./grid_2_2d_size

  
*/
#include <stdio.h>
#include <mfhdf.h>
#include <HdfEosDef.h>

//intn write_attr(int gdid, char* field_name, char* attr_name, char* value);
intn write_field_2d(int gdid, char* field_name, long xdim, long ydim);
intn write_grid(int gdfid, char* gname, long xdim, long ydim);

#if 0
/* Write a local attribute. See HDF-EOS5 testdrivers/grid/TestGrid.c.  */
intn write_attr(int gdid, char* field_name, char* attr_name, char* value)
{
    int count[1];
    count[0] = strlen(value);
    return GDwriteattr(gdid, field_name, attr_name, DFNT_CHAR8,
                              count, value);
    
    
}
#endif

intn write_field_2d(int gdid, char* field_name, long xdim, long ydim)
{
    int i = 0;
    int j = 0;

    int x = 0;
    int y = 0;

    float temp[4][3];
    float temp2[5][4];
    float val = 0.0;

    int edge[2];
    int start[2];

    x = (int) xdim;
    y = (int) ydim;

    /* Fill data. */
    for (i=0; i < y; i++){
        for(j=0; j < x; j++){
            if(xdim == 3)
                temp[i][j] = -10.0+val+i;
            if(xdim == 4)
                temp2[i][j] = -20.0+val+2*i;
            ++val; 
        }
    }

    start[0] = 0; 
    start[1] = 0;
    edge[0] = ydim;
    edge[1] = xdim;

    /* Create a field. */
    GDdeffield(gdid, field_name, "YDim,XDim",
                   DFNT_FLOAT32, 0);
    
    if(xdim == 3)
        GDwritefield(gdid, field_name, start, NULL, edge, temp);
    if(xdim == 4)
        GDwritefield(gdid, field_name, start, NULL, edge, temp2);

    /* Write attribute. */
    //return write_attr(gdid, field_name, "units", "C");
    return 0;
}


intn write_grid1(int gdfid, char* gname, long xdim, long ydim)
{
    double upleft[2];
    double lowright[2];
    double projparm[16] = { 6378273,-0.006694,0,0,-45000000,70000000,0,0, };
    int spherecode = -1;
    int zonecode = -1;

    intn status = FAIL;    

    int  gdid = FAIL;
    

    /* Set corner points. */
    upleft[0] = -3850000.000000; 
    upleft[1]  = 5850000.000000; 
    lowright[0] = 3750000.000000; 
    lowright[1] = -5350000.000000;          


    /* Create Grids. */
    gdid  = GDcreate(gdfid, gname, xdim, ydim, upleft, lowright);

    /* Set projection. */
    status = GDdefproj(gdid, GCTP_PS, zonecode, spherecode, projparm);
	
    /* Write field. */
    write_field_2d(gdid, "Temperature", xdim, ydim);


    /* Close the Grid. */
    status = GDdetach(gdid);
    return status;
}

intn write_grid2(int gdfid, char* gname, long xdim, long ydim)
{
    double upleft[2];
    double lowright[2];
    double projparm[16] = { 6378273,-0.006694,0,0,0,-70000000,0,0,0,0, };
    int spherecode = -1;
    int zonecode = -1;

    intn status = FAIL;    

    int  gdid = FAIL;
    

    /* Set corner points. */
    upleft[0] = -3950000.000000; 
    upleft[1]  = 4350000.000000; 
    lowright[0] = 3950000.000000; 
    lowright[1] = -3950000.000000;          


    /* Create Grids. */
    gdid  = GDcreate(gdfid, gname, xdim, ydim, upleft, lowright);

    /* Set projection. */
    status = GDdefproj(gdid, GCTP_PS, zonecode, spherecode, projparm);
	
    /* Write field. */
    write_field_2d(gdid, "Temperature", xdim, ydim);


    /* Close the Grid. */
    status = GDdetach(gdid);
    return status;
}
int main()
{
    intn status = FAIL;    
    int  gdfid = FAIL;

    /* Create a file. */
    gdfid = GDopen("grid_2_2d_ps.hdf", DFACC_CREATE);

    /* Write two identical grids except names. */
    write_grid1(gdfid, "NPGrid", 4, 5);
    write_grid2(gdfid, "SPGrid", 3, 4);

    /* Close the file. */
    status = GDclose(gdfid);
    return 0;
}

