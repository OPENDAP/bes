/*******************************************************************************
NAME                  		SINUSOIDAL

PURPOSE:	Transforms input Easting and Northing to longitude and
		latitude for the Sinusoidal projection.  The
		Easting and Northing must be in meters.  The longitude
		and latitude values will be returned in radians.

PROGRAMMER                   DATE              REASON            
----------                   ----              ------  
D. Steinwand, EROS           May, 1991     
Abe Taaheri/Raytheon IIS     March, 2017       Modified to support both
                                               spherical and ellipsoidal
                                               models of earth for 
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
static double r_major, r_minor;	/* major axis 				*/
static double lon_center;	/* Center longitude (projection center) */
static double R;		/* Radius of the earth (sphere) 	*/
static double false_easting;	/* x offset in meters			*/
static double false_northing;	/* y offset in meters			*/
static long ind;		/* spherical flag       		*/
static double e, es, imu;       /* eccentricity constants               */
static double e1, e2, e3, e4, e5;

/* Initialize the Sinusoidal projection
  ------------------------------------*/
int sininvint(
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

 if(e < 0.00001)
   {
     ind = 1; /* sphere */
   }
 else
   {
     double e12, e13, e14;
     ind = 0; /* ellipsoid */
     e1 = (1.0 - sqrt(1.0 - es))/(1.0 + sqrt(1.0 - es));
     e12 = e1 * e1;
     e13 = e12 * e1;
     e14 = e13 * e1;
     imu = (1.0 - (es/4.0) - (3.0 * es * es / 64.0) - 
		       (5.0 *  es * es * es /256.0));
     e2 = ((3.0 * e1 /2.0) - (27.0 * e13 / 32.0));
     e3 = ((21.0 * e12 / 16.0) - (55.0 * e14 / 32.0));
     e4 = (151.0 * e13 / 96.0);
     e5 = (1097.0 * e14 / 512.0);
   }

/* Report parameters to the user
  -----------------------------*/
ptitle("SINUSOIDAL");
radius2(r_major, r_minor);
cenlon(center_long);
offsetp(false_easting,false_northing);
return(OK);
}

/* Sinusoidal inverse equations--mapping x,y to lat,long 
  -----------------------------------------------------*/
int sininv(
double x,		/* (I) X projection coordinate */
double y,		/* (I) Y projection coordinate */
double *lon,		/* (O) Longitude */
double *lat)		/* (O) Latitude */
{
  double temp;		/* Re-used temporary variable */
  double mu, ml;
  double sin_phi, cos_phi;
/* Inverse equations
  -----------------*/
  x -= false_easting;
  y -= false_northing;
  
  if (ind != 0) /* Spherical */
    {
      *lat = y / R;
      if (fabs(*lat) > HALF_PI) 
	{
	  p_error("Input data error","sinusoidal-inverse");
	  return(164);
	}
      temp = fabs(*lat) - HALF_PI;
      if (fabs(temp) > EPSLN)
	{
	  temp = lon_center + x / (R * cos(*lat));
	  *lon = adjust_lon(temp);
	}
      else *lon = lon_center;
      return(OK);
    }
  else /* Ellipsoidal */
    {
      ml = y;
      mu = ml / (r_major * imu);
      *lat = mu + 
	(e2 * sin(2.0 * mu)) + 
	(e3 * sin(4.0 * mu)) + 
	(e4 * sin(6.0 * mu)) + 
	(e5 * sin(8.0 * mu));

      if (fabs(*lat) > HALF_PI) 
	{
	  p_error("Input data error","sinusoidal-inverse");
	  return(164);
	}
      temp = fabs(*lat) - HALF_PI;
      if (fabs(temp) > EPSLN)
	{
	  sin_phi = sin(*lat);
	  cos_phi = cos(*lat);
	  temp = lon_center + 
	    x * sqrt(1.0 - es * SQUARE(sin_phi)) / (r_major * cos_phi);
	  *lon = adjust_lon(temp);
	}
     else *lon = lon_center;
     return(OK);
   }
}

