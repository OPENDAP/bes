/*******************************************************************************
NAME                  		SINUSOIDAL

PURPOSE:	Transforms input longitude and latitude to Easting and
		Northing for the Sinusoidal projection.  The
		longitude and latitude must be in radians.  The Easting
		and Northing values will be returned in meters.

PROGRAMMER                   DATE              REASON   
----------                   ----              ------
D. Steinwand, EROS           May, 1991     
Abe Taaheri/Raytheon IIS     March, 2017       Modified to support both
                                               spherical and ellipsoidal
                                               models of earth for r 
                                               Sinusoidal projection

This function was adapted from the Sinusoidal projection code (FORTRAN) in the 
General Cartographic Transformation Package software which is available from 
the U.S. Geological Survey National Mapping Division.
 
ALGORITHM REFERENCES

1.  Snyder, John P., "Map Projections--A Working Manual", U.S. Geological
    Survey Professional Paper 1395 (Supersedes USGS Bulletin 1532), United
    State Government Printing Office, Washington D.C., 1987.

2.  "Software Documentation for GCTP General Cartographic Transformation
    Package", U.S. Geological Survey National Mapping Division, May 1982.
*******************************************************************************/
#include <stdio.h>
#include "cproj.h"

/* Variables common to all subroutines in this code file
  -----------------------------------------------------*/
static double r_major;		/* major axis 				*/
static double r_minor;	        /* minor axis    			*/
static double lon_center;	/* Center longitude (projection center) */
static double R;		/* Radius of the earth (sphere) 	*/
static double false_easting;	/* x offset in meters			*/
static double false_northing;	/* y offset in meters			*/
static long ind;		/* spherical flag       		*/
static double e, es;            /* eccentricity constants               */
static double e0,e1,e2,e3;	/* eccentricity constants		*/

/* Initialize the Sinusoidal projection
  ------------------------------------*/
int sinforint(
double r_maj,			/* major axis		        	*/
double r_min,			/* minor axis		        	*/
double center_long,		/* (I) Center longitude 		*/
double false_east,		/* x offset in meters			*/
double false_north)		/* y offset in meters			*/
{
/* Place parameters in static storage for common use
  -------------------------------------------------*/
  R = r_maj;
 if(fabs(r_min) < EPSLN ) /* sphere */
   {
     r_major = r_maj;
     r_minor = r_maj;
   }
 else /* sphere or ellipsoide */
   {
     r_major = r_maj;
     r_minor = r_min;
   }

lon_center = center_long;
false_easting = false_east;
false_northing = false_north;

es = 1.0 - SQUARE(r_minor / r_major);
e = sqrt(es);
e0 = e0fn(es);
e1 = e1fn(es);
e2 = e2fn(es);
e3 = e3fn(es);

 if(es < 0.00001)
   {
     ind = 1; /* sphere */   }
 else
   {
     ind = 0; /* ellipsoid */
   }

/* Report parameters to the user
  -----------------------------*/
ptitle("SINUSOIDAL"); 
radius2(r_major, r_minor);
cenlon(center_long);
offsetp(false_easting,false_northing);
return(OK);
}

/* Sinusoidal forward equations--mapping lat,long to x,y
  -----------------------------------------------------*/
int sinfor(
double lon,			/* (I) Longitude */
double lat,			/* (I) Latitude */
double *x,			/* (O) X projection coordinate */
double *y)			/* (O) Y projection coordinate */
{
  double delta_lon;	  /* Delta longitude (Given longitude - center  */
  double sin_phi, cos_phi;/* sin and cos value				*/
  double con, ml;	  /* cone constant, small m			*/
/* Forward equations
  -----------------*/
  if (ind != 0) /* Spherical */
  {
    delta_lon = adjust_lon(lon - lon_center);
    *x = R * delta_lon * cos(lat) + false_easting;
    *y = R * lat + false_northing;
    return(OK);
  }
  else /* Ellipsoidal */
   {
     delta_lon = adjust_lon(lon - lon_center);
     tsincos(lat, &sin_phi, &cos_phi);
     con = 1.0 - es * SQUARE(sin_phi);
     ml = r_major * mlfn(e0, e1, e2, e3, lat);
     *x = r_major * delta_lon * cos_phi / sqrt(con) + false_easting;
     *y = ml + false_northing;
     return(OK);
   }
}
