/*******************************************************************************
NAME                  LAMBERT AZIMUTHAL EQUAL-AREA
 
PURPOSE:	Transforms input longitude and latitude to Easting and
		Northing for the Lambert Azimuthal Equal-Area projection. The
		longitude and latitude must be in radians.  The Easting
		and Northing values will be returned in meters.

PROGRAMMER                   DATE              REASON 
----------                   ----              ------
D. Steinwand, EROS           March, 1991   
Abe Taaheri/Raytheon IIS     Nov,   2013       Modified to support both
                                               spherical and ellipsoidal
                                               models of earth for 
                                               Lambert Azimuthal  Equal Area
                                               projection to support EASE-Grid2

This function was adapted from the Lambert Azimuthal Equal Area projection
code (FORTRAN) in the General Cartographic Transformation Package software
which is available from the U.S. Geological Survey National Mapping Division.
 
ALGORITHM REFERENCES

1.  "New Equal-Area Map Projections for Noncircular Regions", John P. Snyder,
    The American Cartographer, Vol 15, No. 4, October 1988, pp. 341-355.

2.  Snyder, John P., "Map Projections--A Working Manual", U.S. Geological
    Survey Professional Paper 1395 (Supersedes USGS Bulletin 1532), United
    State Government Printing Office, Washington D.C., 1987.

3.  "Software Documentation for GCTP General Cartographic Transformation
    Package", U.S. Geological Survey National Mapping Division, May 1982.

4.  "EASE-Grid 2.0: Incremental but Significant Improvements for Earth-Gridded Data
     Sets", Mary J. Brodzik ?, Brendan Billingsley, Terry Haran , Bruce Raup and 
     Matthew H.Savoie, National Snow & Ice Data Center, Cooperative Institute of 
     Environmental Sciences, University of Colorado, 449 UCB, Boulder, CO 80309, USA;
     E-Mails: brendan.billingsley@nsidc.org (B.B.); tharan@nsidc.org (T.H.); 
     braup@nsidc.org (B.R.); savoie@nsidc.org (M.H.S.)
*******************************************************************************/
#include <stdio.h>
#include "cproj.h"

/* Variables common to all subroutines in this code file
  -----------------------------------------------------*/
static double r_major;		/* major axis 				*/
static double r_minor;	        /* minor axis    			*/
static double lon_center;	/* Center longitude (projection center) */
static double lat_center;	/* Center latitude (projection center)  */
static double R;		/* Radius of the earth (sphere)	 	*/
static double sin_lat_o;	/* Sine of the center latitude 		*/
static double cos_lat_o;	/* Cosine of the center latitude 	*/
static double sinphi1;	        /* Sine of the center latitude 		*/
static double cosphi1;	        /* Cosine of the center latitude 	*/
static double false_easting;	/* x offset in meters			*/
static double false_northing;	/* y offset in meters			*/
static double qp;               /* q at 90 degrees.                     */
static double q1;               /* q at lat_center degrees              */
static double beta1, m1, Rq, D;
static double sin_beta1;
static double cos_beta1;
static long ind;		/* spherical flag       		*/
static double e, es;            /* eccentricity constants               */

/* Initialize the General Lambert Azimuthal Equal Area projection
  --------------------------------------------------------------*/
int lamazforint(
double r_maj,			/* major axis		        	*/
double r_min,			/* minor axis		        	*/
double center_long,		/* (I) Center longitude 		*/
double center_lat,		/* (I) Center latitude 			*/
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
lat_center = center_lat;
false_easting = false_east;
false_northing = false_north;
tsincos(center_lat, &sin_lat_o, &cos_lat_o);
sinphi1 = sin_lat_o; 
cosphi1 = cos_lat_o;

es = 1.0 - SQUARE(r_minor / r_major);
e = sqrt(es);

 if(es < 0.00001)
   {
     ind = 1; /* sphere */
     qp = 2.0;
     q1 = 2.0;
   }
 else
   {
     ind = 0; /* ellipsoid */
     qp = (1.0 - es)* (((1.0/(1.0 - es))-(1.0/(2.0*e))*log((1.0 - e)/(1.0 + e))));
     
     if((fabs (lat_center - HALF_PI) <=  EPSLN ) || (fabs (lat_center + HALF_PI) <=  EPSLN ))
       {
	 /* no other constants needed for LA with North and South polar Aspects lat_center = 90 or -90*/
       }
     else
       {
	 tsincos(lat_center, &sinphi1, &cosphi1);
	 q1 = (1.0 - es) * ((sinphi1 / (1.0 - es * sinphi1 * sinphi1))
				- (1.0 / (2.0 * e)) * 
				log((1.0 - e * sinphi1)/(1.0 + e * sinphi1)));
	 Rq = r_major * sqrt(qp/2.0);
	 if(fabs(q1) >= fabs(qp))
	   {
	     beta1 = HALF_PI * (fabs(q1/qp)/(q1/qp));
	   }
	 else
	   {
	     beta1 = asinz(q1/qp);
	   }
	 tsincos(beta1, &sin_beta1, &cos_beta1);
	 m1 = cosphi1 / sqrt(1.0 - es * sinphi1 * sinphi1);
	 D = (r_major * m1)/ (Rq * cos_beta1);
       }
   }

/* Report parameters to the user
  -----------------------------*/
ptitle("LAMBERT AZIMUTHAL EQUAL-AREA"); 
radius2(r_major, r_minor);
cenlon(center_long);
cenlat(center_lat);
offsetp(false_easting,false_northing);
return(OK);
}

/* Lambert Azimuthal Equal Area forward equations--mapping lat,long to x,y
  -----------------------------------------------------------------------*/
int lamazfor(
double lon,			/* (I) Longitude */
double lat,			/* (I) Latitude */
double *x,			/* (O) X projection coordinate */
double *y)			/* (O) Y projection coordinate */
{
double delta_lon;	/* Delta longitude (Given longitude - center 	*/
double sin_delta_lon;	/* Sine of the delta longitude 			*/
double cos_delta_lon;	/* Cosine of the delta longitude 		*/
double sinphi;	        /* Sine of the center latitude 	        	*/
double cosphi;	        /* Cosine of the center latitude        	*/
double sin_lat;		/* Sine of the given latitude 			*/
double cos_lat;		/* Cosine of the given latitude 		*/
double g;		/* temporary varialbe				*/
double ksp;		/* heigth above elipsiod			*/
char mess[60];
double qpmq, qppq, rho; 
double beta, sin_beta, cos_beta;
double q, B;

/* Forward equations
   -----------------*/
 if( ind != 0) /* sphere */
   {
     delta_lon = adjust_lon(lon - lon_center);
     tsincos(lat, &sin_lat, &cos_lat);
     tsincos(delta_lon, &sin_delta_lon, &cos_delta_lon);
     g = sin_lat_o * sin_lat + cos_lat_o * cos_lat * cos_delta_lon;
     if (g == -1.0) 
       {
	 sprintf(mess, "Point projects to a circle of radius = %lf\n", 2.0 * R);
	 p_error(mess, "lamaz-forward");
	 return(113);
       }
     ksp = R * sqrt(2.0 / (1.0 + g));
     *x = ksp * cos_lat * sin_delta_lon + false_easting;
     *y = ksp * (cos_lat_o * sin_lat - sin_lat_o * cos_lat * cos_delta_lon) + 
       false_northing;
   }
 else /* ellipsoid */
   {
     delta_lon = adjust_lon(lon - lon_center);
     tsincos(lat, &sinphi, &cosphi);
     q = (1.0 - es) * ((sinphi / (1.0 - es * sinphi * sinphi))
		       - (1.0 / (2.0 * e)) * 
		       log((1.0 - e * sinphi)/(1.0 + e * sinphi)));
     
     if (fabs(lat_center - HALF_PI) <= EPSLN)  /* if Northern Polar Aspect */
       {
	 qpmq = qp - q;
	 if (fabs(qpmq) <= EPSLN)
	   {
	     rho = 0.0;
	   }
	 else
	   {
	     rho = r_major * sqrt(qpmq);
	   }
	 *x = false_easting  + rho * sin(delta_lon);
	 *y = false_northing - rho * cos(delta_lon);
       }
     else if (fabs(lat_center + HALF_PI) <= EPSLN)  /* if Southern Polar Aspect */
       {
	 qppq = qp + q;
	 if (fabs(qppq) <= EPSLN)
	   {
	     rho = 0.0;
	   }
	 else
	   {
	     rho = r_major * sqrt(qppq);
	   }
	 *x =  false_easting  + rho * sin(delta_lon);
	 *y =  false_northing + rho * cos(delta_lon);
       }
     else /* with any other latitude of origin */
       {
	 beta = asinz(q/qp);
	 tsincos(beta, &sin_beta, &cos_beta);
	 B = Rq * sqrt((2.0/(1.0 + sin_beta1 * sin_beta + cos_beta1 * cos_beta * cos(delta_lon))));

	 *x = false_easting  + (B*D) * (cos_beta * sin(delta_lon));
	 *y = false_northing + (B/D) * (cos_beta1 * sin_beta - sin_beta1 * cos_beta * cos(delta_lon));
       }
   }

 return(OK);
}
