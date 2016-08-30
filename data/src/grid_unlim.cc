
#include  <HE5_HdfEosDef.h>

#define BUFSIZE  256

int main()
{
   FILE       *fp;

   herr_t     status        = FAIL;

   hid_t      gdfidc_simple = FAIL;
   hid_t      GDid_UTM      = FAIL;

   int        i, j;
   int        ZoneCode;
   int        SphereCode;
   int        tilerank, tilecode;
   int        compcode      = 4;
   int        compparm[5]   = { 0, 0, 0, 0, 0};

   long       index     = 0;
   long       xdim      = 0;
   long       ydim      = 0;
   
   float      veg[200][120];
   
   double     uplft[2]      = {0, 0};
   double     lowrgt[2]     = {0, 0};
   double     *ProjParm;
   
   hssize_t   start[2]  = { 0, 0 };

   hsize_t    ndims          = 0;  
   hsize_t    edge[2]        = {1, 1 };
   hsize_t    dims[8]        = {0, 0, 0, 0, 0, 0, 0, 0 };
   hsize_t    tiledims[8];

   size_t       size       = 0;
 
   /* Fill veg array */
   for (i = 0; i < 200; i++)
	 for (j = 0; j < 120; j++)
	   veg[i][j] = (float)(10+i);
   
   
   gdfidc_simple = HE5_GDopen("SimpleGrid.h5", H5F_ACC_TRUNC);
   
   xdim      = 120;
   ydim      = 200;
   
   uplft[0]  = -512740.28306;
   uplft[1]  = 2733747.62890;
   
   lowrgt[0] = -12584.57301;
   lowrgt[1] = 1946984.64021;
   
   GDid_UTM = HE5_GDcreate(gdfidc_simple, "UTM", xdim, ydim, uplft, lowrgt);
   if (GDid_UTM == FAIL)
	 {
         printf("HE5_GDcreate fails. \n");
         return -1;
	 }
   
   ZoneCode   = -13;
   SphereCode = 0;
   
   for (i = 0; i < 16; i++)
	 {
       ProjParm[i] = 0.0;
	 }
   
   status = HE5_GDdefproj(GDid_UTM, HE5_GCTP_UTM, ZoneCode, SphereCode, ProjParm);
   
   status = HE5_GDdefpixreg( GDid_UTM, HE5_HDFE_CORNER);
   
   status = HE5_GDdeforigin(GDid_UTM, HE5_HDFE_GD_UL );
   
   status = HE5_GDdefdim(GDid_UTM, "Unlim", H5S_UNLIMITED);
   
   HE5_GDdetach(GDid_UTM);
   status = HE5_GDclose(gdfidc_simple);
   gdfidc_simple = HE5_GDopen("SimpleGrid.h5", H5F_ACC_RDWR);
   GDid_UTM=HE5_GDattach(gdfidc_simple, "UTM");
   
   tilerank    = 2;
   tiledims[0] = 10;
   tiledims[1] = 10;
   compparm[0] = 5;
   
   status = HE5_GDdeftile(GDid_UTM, HE5_HDFE_TILE, tilerank, tiledims);
   
   status = HE5_GDdefcomp(GDid_UTM, compcode, compparm);
   
   status = HE5_GDdeffield(GDid_UTM,"Vegetation","YDim,XDim","Unlim,Unlim",H5T_NATIVE_FLOAT,0);
   
   HE5_GDdetach(GDid_UTM);
   status = HE5_GDclose(gdfidc_simple);
   gdfidc_simple = HE5_GDopen("SimpleGrid.h5", H5F_ACC_RDWR);
   GDid_UTM=HE5_GDattach(gdfidc_simple, "UTM");
   
   edge[0]   = 200;
   edge[1]   = 120;
   
   status = HE5_GDwritefield(GDid_UTM, "Vegetation", start, NULL, edge, veg);
   if (status == FAIL)
	 {
         printf("HE5_GDwritefield fails \n");
         return -1;
	 }
   
   status = HE5_GDdetach(GDid_UTM);
   
   
   GDid_UTM  = HE5_GDattach(gdfidc_simple,"UTM"); 
   if (GDid_UTM == FAIL)
	 {
         printf("HE5_GDattach fails. \n");
         return -1;
	 }
   
   start[0]  = 100;
   start[1]  = 200;
   
   edge[0]   = 200;
   edge[1]   = 120;
  

/*
   start[0]  = 100;
   start[1]  = 119;
   
   edge[0]   = 200;
   edge[1]   = 120;
*/

   status = HE5_GDwritefield(GDid_UTM, "Vegetation", start, NULL, edge, veg);
   if (status == FAIL)
	 {
       printf("HE5_GDwritefield fails \n");
       return -1;
	 }
   
   status = HE5_GDdetach(GDid_UTM);
   
   status = HE5_GDclose(gdfidc_simple);
  
   return status;

}

/*
References
 [1] hdfeos5/testdrivers/grid/TestGrid.c 
*/











