/*
  A simple test program that generates CEA(Cylindrical Equal-Area,EASE grid) projection Grid with
  1d variable.
  
  Compilation instruction:

  %h4cc -I/path/to/hdfeos/include  -L/path/to/hdfeos/lib -o cea \
        -lhdfeos -lGctp -lm cea.c

  To generate the test file, run

  %./cea

  Copyright The HDF Group
  
 */

#include "hdf.h"
#include "HdfEosDef.h"
#include <stdio.h>
#include <stdlib.h>


int main()
{
  int i,j,k;
  int32 fid, gdid;
  int32 grid1_xdim,grid1_ydim;
  int32* grid1_row;
  int32* grid1_col;
  float64* grid1_lon;
  float64* grid1_lat;
  int32 xdim,ydim;
  float64 uplft[2] = { -1.7e+07, 7.3e+06 }, lowrgt[2] = { 1.7e+07, -7.3e+06 } ;
  char dim_name[]="YDim,XDim";

  if ((fid = GDopen("cea.hdf", DFACC_CREATE)) < 0) {
      fprintf(stderr, "error: cannot open the HDF-EOS2 grid file\n");
      return -1;
  }

  
  xdim = 8;
  ydim = 8;
  grid1_xdim = 8;
  grid1_ydim = 8;
  if ((gdid = GDcreate(fid, "grid1", xdim, ydim, uplft, lowrgt)) < 0) {
      fprintf(stderr, "error: cannot create an HDF-EOS2 grid.\n");
      return -1;
  }
  
  
  int32 zonecode = -1, spherecode = -1;
  float64 projparm[16] = { 6371228, 0, 0, 0, 0, 300000000, 0, 0, };
  if (GDdefproj(gdid, GCTP_CEA, zonecode, spherecode, projparm) < 0){
      fprintf(stderr, "error: cannot define the HDF-EOS2 grid\n");
      return -1;
  } 

  float32 temp[8][8];
  /* Fill data. */
  for (i=0; i < 8; i++)
      for(j=0; j < 8; j++)
            temp[i][j] = (float32)(10 + i);
  if (GDdeffield(gdid, "temperature", dim_name, DFNT_FLOAT32, 0) <0) {
      fprintf(stderr, "error: cannot define the HDF-EOS2 grid field\n");
      return -1;
  }
  if (GDwritefield(gdid, "temperature", NULL, NULL, NULL, temp) <0) {
      fprintf(stderr, "error: cannot write the HDF-EOS2 grid field\n");
      return -1;
  }

 /* Allocate buffer for row */
  if ((grid1_row = malloc(sizeof(int32) * grid1_xdim * grid1_ydim)) == NULL) {
      fprintf(stderr, "error: cannot allocate memory for row\n");
      return -1;

  }
  /* Allocate buffer for column */
  if ((grid1_col = malloc(sizeof(int32) * grid1_xdim * grid1_ydim)) == NULL) {
    fprintf(stderr, "error: cannot allocate memory for column\n");
    return -1;
  }
  /* Fill two arguments, rows and columns */
  for (k = j = 0; j < grid1_ydim; ++j) {
    for (i = 0; i < grid1_xdim; ++i) {
      grid1_row[k] = j;
      grid1_col[k] = i;
      ++k;
    }
  }
  /* Allocate buffer for longitude */
  if ((grid1_lon = malloc(sizeof(float64) * grid1_xdim * grid1_ydim)) == NULL) {
    fprintf(stderr, "error: cannot allocate memory for longitude\n");
    return -1;
  }
  /* Allocate buffer for latitude */
  if ((grid1_lat = malloc(sizeof(float64) * grid1_xdim * grid1_ydim)) == NULL) {
    fprintf(stderr, "error: cannot allocate memory for latitude\n");
    return -1;
  }
  /* Retrieve lon/lat values for 'MonthlyRainTotal_GeoGrid' */
  if ((GDij2ll(GCTP_CEA, zonecode, projparm, spherecode, grid1_xdim, grid1_ydim, uplft, lowrgt, grid1_xdim * grid1_ydim, grid1_row, grid1_col, grid1_lon, grid1_lat, 0, 0)) == -1) {
    fprintf(stderr, "error: cannot retrieve lon/lat values for 'MonthlyRainTotal_GeoGrid'\n");
    return -1;
  }

  for (i = 0; i < xdim; ++i) {
    for (j = 0; j < ydim; ++j)
      printf("%5.1lf/%4.1lf, ", grid1_lon[j + grid1_xdim * i], grid1_lat[j + grid1_xdim * i]);
    printf("\n");
  }
  


  /* Release buffer for row */
  free(grid1_row);
  /* Release buffer for column */
  free(grid1_col);
  /* Release buffer for longitude */
  free(grid1_lon);
  /* Release buffer for latitude */
  free(grid1_lat);
  GDdetach(gdid);
  GDclose(fid);

  return 0;
}


/* vi:set ts=8 sw=2 sts=2: */
