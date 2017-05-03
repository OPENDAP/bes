/*******************************************************************************
NAME                            STATE PLANE 

PURPOSE:	Transforms input Easting and Northing to longitude and
		latitude for the State Plane projection.  The
		Easting and Northing must be in meters.  The longitude
		and latitude values will be returned in radians.

PROGRAMMER              DATE
----------              ----
T. Mittan		Apr, 1993

ALGORITHM REFERENCES

1.  Snyder, John P., "Map Projections--A Working Manual", U.S. Geological
    Survey Professional Paper 1395 (Supersedes USGS Bulletin 1532), United
    State Government Printing Office, Washington D.C., 1987.

2.  Snyder, John P. and Voxland, Philip M., "An Album of Map Projections",
    U.S. Geological Survey Professional Paper 1453 , United State Government
    Printing Office, Washington D.C., 1989.
*******************************************************************************/
#include <stdio.h>
#include "cproj.h"

static int id;
static long inzone = 0;
static long nad27[134] = {101,102,5010,5300,201,202,203,301,302,401,402,403,
		404,405,406,407,501,502,503,600,700,901,902,903,1001,1002,
		5101,5102,5103,5104,5105,1101,1102,1103,1201,1202,1301,1302,
		1401,1402,1501,1502,1601,1602,1701,1702,1703,1801,1802,1900,
		2001,2002,2101,2102,2103,2111,2112,2113,2201,2202,2203,2301,
		2302,2401,2402,2403,2501,2502,2503,2601,2602,2701,2702,2703,
		2800,2900,3001,3002,3003,3101,3102,3103,3104,3200,3301,3302,
		3401,3402,3501,3502,3601,3602,3701,3702,3800,3901,3902,4001,
		4002,4100,4201,4202,4203,4204,4205,4301,4302,4303,4400,4501, 
		4502,4601,4602,4701,4702,4801,4802,4803,4901,4902,4903,4904,
		5001,5002,5003,5004,5005,5006,5007,5008,5009,5201,5202,5400};

static long nad83[134] = {101,102,5010,5300,201,202,203,301,302,401,402,403,
		404,405,406,0000,501,502,503,600,700,901,902,903,1001,1002,
		5101,5102,5103,5104,5105,1101,1102,1103,1201,1202,1301,1302,
		1401,1402,1501,1502,1601,1602,1701,1702,1703,1801,1802,1900,
		2001,2002,2101,2102,2103,2111,2112,2113,2201,2202,2203,2301,
		2302,2401,2402,2403,2500,0000,0000,2600,0000,2701,2702,2703,
		2800,2900,3001,3002,3003,3101,3102,3103,3104,3200,3301,3302,
		3401,3402,3501,3502,3601,3602,3701,3702,3800,3900,0000,4001,
		4002,4100,4201,4202,4203,4204,4205,4301,4302,4303,4400,4501, 
		4502,4601,4602,4701,4702,4801,4802,4803,4901,4902,4903,4904,
		5001,5002,5003,5004,5005,5006,5007,5008,5009,5200,0000,5400};                                             
/* Initialize the State Plane projection
  ------------------------------------*/
int stplninvint(
long   zone,
long   sphere,
char   *fn27,
char   *fn83)
{
long ind;
long i;
long nadval;
double table[9];
char pname[33];
char buf[100];
double r_maj,r_min,scale_fact,center_lon;
double center_lat,false_east,false_north;
double azimuth,lat_orig,lon_orig,lon1,lat1,lon2,lat2;
long mode,iflg;
FILE *ptr;
#if defined(DEC_ALPHA) || defined(SCO) || defined(LINUX)
char  swap[16];
char* swapPtr;
long j;
#endif

ind = -1;
if (inzone != zone)
   inzone = zone;
else
   return(OK);
if (zone > 0)
   {
   if (sphere == 0)
      {
      for (i = 0; i < 134; i++)
	 {
	 if (zone == nad27[i])
	    {
            ind = i;
            break;
            }
	 }
      }
   else
   if (sphere == 8)
      {
      for (i = 0; i < 134; i++)
         {
         if (zone == nad83[i])
	    {
            ind = i;
            break;
            }
         }
      }
   }
   if (ind == -1)
      {
      sprintf(buf,"Illegal zone #%4ld  for spheroid #%4ld",zone,sphere);
      p_error(buf,"state-init");
      return(21);
      }
   
   if (sphere == 0)
      ptr = fopen(fn27,"r");
   else
      ptr = fopen(fn83,"r");
   if (ptr == NULL)
      {
      p_error("Error opening State Plane parameter file","state-inv");
      return(22);
      }
   fseek(ptr,ind  * 432, 0);
   ftell(ptr);
   fread(pname,sizeof(char),32,ptr);
   fread(&id,sizeof(int),1,ptr);
   fread(table,sizeof(double),9,ptr);
   fclose(ptr);
   
#if defined(DEC_ALPHA) || defined(SCO) || defined(LINUX)
   /* swap bytes as necessary */
   swapPtr = (char*) (&id);
   for (i=0;i<sizeof(int);i++)
   {
       swap[i] = swapPtr[sizeof(int) - 1 - i];
   }
   id = *((int*)swap);
   
   for (i=0;i<9;i++)
   {
       swapPtr = (char*) (table+i);
       for (j=0;j<sizeof(double);j++)
       {
	   swap[j] = swapPtr[sizeof(double) - 1 - j];
       }
       table[i] = *((double*)swap);
   }
#endif

   if (id <= 0)
      {
      sprintf(buf,"Illegal zone #%4ld  for spheroid #%4ld",zone,sphere);
      p_error(buf,"state-init");
      return(21);
      }

/* Report parameters to the user
  -----------------------------*/
ptitle("STATE PLANE"); 
genrpt_long(zone,"Zone:     ");
if (sphere == 0)
   nadval = 27;
else
   nadval = 83;
genrpt_long(nadval,"Datum:     NAD");
      
/* initialize proper projection
-----------------------------*/
r_maj = table[0];
r_min = (sqrt(1.0 - table[1])) * table[0];

if (id == 1)
   {
   scale_fact = table[3];
   center_lon = paksz(pakcz(table[2]),&iflg) * D2R;
   if (iflg != 0)
       return(iflg);
   center_lat = paksz(pakcz(table[6]),&iflg) * D2R;
   if (iflg != 0)
       return(iflg);
   false_east = table[7];
   false_north = table[8];
   tminvint(r_maj,r_min,scale_fact,center_lon,center_lat,false_east,
	    false_north);
   }
else
if (id == 2)
   {
   lat1 = paksz(pakcz(table[5]),&iflg) * D2R;
   if (iflg != 0)
       return(iflg);
   lat2 = paksz(pakcz(table[4]),&iflg) * D2R;
   if (iflg != 0)
       return(iflg);
   center_lon = paksz(pakcz(table[2]),&iflg) * D2R;
   if (iflg != 0)
       return(iflg);
   center_lat = paksz(pakcz(table[6]),&iflg) * D2R;
   if (iflg != 0)
       return(iflg);
   false_east = table[7];
   false_north = table[8];
   lamccinvint(r_maj,r_min,lat1,lat2,center_lon,center_lat,false_east,
	       false_north);
   }
else
if (id == 3)
   {
   center_lon = paksz(pakcz(table[2]),&iflg) * D2R;
   if (iflg != 0)
       return(iflg);
   center_lat = paksz(pakcz(table[3]),&iflg) * D2R;
   if (iflg != 0)
       return(iflg);
   false_east = table[4];
   false_north = table[5];
   polyinvint(r_maj,r_min,center_lon,center_lat,false_east,false_north);
   }
else
if (id == 4)
   {
   scale_fact = table[3];
   azimuth = paksz(pakcz(table[5]),&iflg) * D2R;
   if (iflg != 0)
       return(iflg);
   lon_orig = paksz(pakcz(table[2]),&iflg) * D2R;
   if (iflg != 0)
       return(iflg);
   lat_orig = paksz(pakcz(table[6]),&iflg) * D2R;
   if (iflg != 0)
       return(iflg);
   false_east = table[7];
   false_north = table[8];
   mode = 1;
   lon1 = 0.0;
   lon2 = 0.0;
   lat1 = 0.0; /* for mode > 0 setting lat1 and lat2 to zero do 
		  make any difference in omerinvint */
   lat2 = 0.0;
   omerinvint(r_maj,r_min,scale_fact,azimuth,lon_orig,lat_orig,false_east,
              false_north,lon1,lat1,lon2,lat2,mode);
   }

return(OK);
}


/* State Plane inverse equations--mapping x,y to lat/long
  -----------------------------------------------------*/
int stplninv(
double x,			/* (O) X projection coordinate 	*/
double y,			/* (O) Y projection coordinate 	*/
double *lon,			/* (I) Longitude 		*/
double *lat)			/* (I) Latitude 		*/
{
long iflg;

/* Inverse equations
  -----------------*/
if (id == 1)
   {
   if ((iflg = tminv(x, y, lon, lat)) != 0)
      return(iflg);
   }
else
if (id == 2)
   {
   if ((iflg = lamccinv(x, y, lon, lat)) != 0)
      return(iflg);
   }
else
if (id == 3)
   {
   if ((iflg = polyinv(x, y, lon, lat)) != 0)
      return(iflg);
   }
else
if (id == 4)
   {
   if ((iflg = omerinv(x, y, lon, lat)) != 0)
      return(iflg);
   }

return(OK);
}
