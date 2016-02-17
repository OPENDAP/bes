/*Copyright (C) 2016 The HDF Group */

#include     <HE5_HdfEosDef.h>


/*  The appendable field "Spectra" will be created.              */
/* ------------------------------------------------------------ */


#define RANK 3

int main()
{

  herr_t      status = FAIL;

  hid_t       swfid  = FAIL;
  hid_t       SWid   = FAIL;

  int         comp_level[ 5 ] = { 0, 0, 0, 0, 0 };
  int         comp_code;

  int         i, j, k;
  hsize_t     chunk_dims[ 3 ];

  hssize_t    start[3] = {0, 0, 0};
    
  hsize_t     count[3];
  hsize_t     dims[8] = {0, 0, 0, 0, 0, 0, 0, 0};


  double      plane[4][3][2];
  int         track, xtrack;
  float       lng[20][10], latcnt;
  float       lat[20][10], loncnt;


  char        dimlist[80];
  char        maxdimlist[80];   



  /* Open the file, "Swath.h5", using the H5F_ACC_RDWR access code */
  /* ------------------------------------------------------------- */
  swfid = HE5_SWopen("swath_unlim.h5", H5F_ACC_TRUNC);
  SWid = HE5_SWcreate(swfid, "Swath1");
  if (SWid != FAIL)
		{	  
		  /*
		   * We define seven fields.  The first three, "Time", "Longitude"
		   * and "Latitude" are geolocation fields and thus we use the
		   * geolocation dimensions "GeoTrack" and "GeoXtrack" in the field
		   * definitions.
		   * 
		   * The next four fields are data fields.  Note that either
		   * geolocation or data dimensions can be used.
		   */
	    
                  HE5_SWdefdim(SWid, "GeoTrack", 20);
                  HE5_SWdefdim(SWid, "GeoXtrack", 10);
                  status = HE5_SWdefdim(SWid, "Res2tr", 3);
                  status = HE5_SWdefdim(SWid, "Res2xtr", 2);
                  status = HE5_SWdefdim(SWid, "Bands", 4);
                  status = HE5_SWdefdim(SWid, "Unlim", H5S_UNLIMITED);
                   printf("Status returned by HE5_SWdefdim():      %d \n", status);


		  status = HE5_SWdefgeofield(SWid, "Longitude", "GeoTrack,GeoXtrack", NULL, H5T_NATIVE_FLOAT, 0);
		  printf("Status returned by HE5_SWdefgeofield(...\"Longitude\",...) :    %d\n",status);

		  status = HE5_SWdefgeofield(SWid, "Latitude", "GeoTrack,GeoXtrack", NULL, H5T_NATIVE_FLOAT, 0);
		  printf("Status returned by HE5_SWdefgeofield(...\"Latitude\",...) :     %d\n",status);


		  /* Define Appendable Field  */
		  /* ------------------------ */

		  /* ----------------------------------------  */
		  /*           First, define chunking          */
		  /* (the appendable dataset must be chunked)  */
		  /* ----------------------------------------  */
		  chunk_dims[0] = 4;
		  chunk_dims[1] = 3;
		  chunk_dims[2] = 2;
	    
             
		  status = HE5_SWdefchunk(SWid, RANK, chunk_dims);
		  printf("\tStatus returned by HE5_SWdefchunk() :                 %d\n",status);

		  comp_code     = 4;
		  comp_level[0] = 6;

		  status = HE5_SWdefcomp(SWid,comp_code, comp_level);
		  printf("\tStatus returned by HE5_SWdefcomp() :                  %d\n",status);

		  status = HE5_SWdefdatafield(SWid, "Spectra", "Bands,Res2tr,Res2xtr", "Unlim,Unlim,Unlim", H5T_NATIVE_DOUBLE, 0);
		  printf("Status returned by HE5_SWdefdatafield(...\"Spectra\",...) :     %d\n",status);


		}
    

  status = HE5_SWdetach(SWid);

  /* Populate lon/lat data arrays */
  /* ---------------------------- */
  latcnt = 1.;
  loncnt = 1.;
  track  = 0;
  xtrack = 0;
 
  while(track < 20) {
	while(xtrack < 10) {
	  lat[track][xtrack] = latcnt;
	  lng[track][xtrack] = loncnt;
	  loncnt = loncnt + 1.;
	  xtrack++;
	}
	latcnt = latcnt + 1.;
	loncnt = 1.;
	track++;
	xtrack = 0;
  }
 
  /* Popolate spectra data arrays */
  /* ---------------------------- */
  for (i = 0; i < 4; i++){
	for (j = 0; j < 3; j++)
	  for (k = 0; k < 2; k++)
		plane[i][j][k] = (double)(j + i);
  }

	  /* Attach the "Swath1" swath */
	  /* ------------------------- */
	  SWid = HE5_SWattach(swfid, "Swath1");
	  if (SWid != FAIL)
		{
		  count[0] = 20;
		  count[1] = 10;

		  /* Write "Longitute" field */
		  /* ----------------------- */
		  status = HE5_SWwritefield(SWid, "Longitude", start, NULL, count, lng);
		  printf("status returned by HE5_SWwritefield(\"Longitude\"):         %d\n", status);
	    
		  /* Write "Latitude" field */
		  /* ---------------------- */
		  status = HE5_SWwritefield(SWid, "Latitude", start, NULL, count, lat);
		  printf("status returned by HE5_SWwritefield(\"Latitude\"):          %d\n", status);


		  /*  Write "Spectra" field  1st time */
		  /* -------------------------------- */

		  count[0] = 4;  
		  count[1] = 3;  
		  count[2] = 2;

		  status = HE5_SWwritefield(SWid, "Spectra", start, NULL, count, plane);
		  printf("status returned by HE5_SWwritefield(\"Spectra\"):           %d\n", status);

		  /*  Write Spectra Field  2d time */
		  /* ----------------------------- */
		  start[0] = 0;  
		  start[1] = 0;  
		  start[2] = 2;

		  count[0] = 4;  
		  count[1] = 3;  
		  count[2] = 2;

		  status = HE5_SWwritefield(SWid, "Spectra", start, NULL, count, plane);
		  printf("status returned by HE5_SWwritefield(\"Spectra\"):           %d\n", status);

   }

   HE5_SWdetach(SWid);
  
   status = HE5_SWclose(swfid);
  
  return 0;
}



/*
 * References
 *  [1] hdfeos5/samples/he5_sw_defunlimfld.c
 *  */


