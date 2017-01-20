/*******************************************************************************
NAME                  LAMBERT AZIMUTHAL EQUAL-AREA

PURPOSE:	Transforms input Easting and Northing to longitude and
		latitude for the Lambert Azimuthal Equal Area projection. The
		Easting and Northing must be in meters.  The longitude
		and latitude values will be returned in radians.

PROGRAMMER                   DATE              REASON   
----------                   ----              ------
D. Steinwand, EROS           March, 1991   
S. Nelson,EROS		     Dec,   1993       changed asin() to asinz() because
					       NaN resulted expecting poles.
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
static double lat_center;	/* Center latitude (projection center) 	*/
static double R;		/* Radius of the earth (sphere) 	*/
static double sin_lat_o;	/* Sine of the center latitude 		*/
static double cos_lat_o;	/* Cosine of the center latitude 	*/
static double false_easting;	/* x offset in meters			*/
static double false_northing;	/* y offset in meters			*/
static double cosphi1;		/* cos of latitude of true scale	*/
static double sinphi1;		/* sin of latitude of true scale       	*/
static double qp;               /* q at 90 degrees.                     */
static double q1;               /* q at lat_center degrees              */
static double beta1, m1, Rq, D;
static double sin_beta1;
static double cos_beta1;
static long ind;		/* spherical flag        		*/
static double e, es, e_p4, e_p6;/* eccentricity constants               */

/* Initialize the General Lambert Azimuthal Equal Area projection
  --------------------------------------------------------------*/
int lamazinvint(
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
e_p4 = es * es;
e_p6 = e_p4 *es;

 if(e < 0.00001)
   {
     ind = 1; /* sphere */
     qp = 2.0;
     q1 = 2.0;
   }
 else
   {
     ind = 0; /* ellipsoid */
     qp = (1.0 - es)* (((1.0/(1.0 - es))-(1.0/(2.0*e))*log((1.0 - e)/(1.0 + e))));

     if((fabs(lat_center - HALF_PI) <=  EPSLN ) || (fabs(lat_center + HALF_PI) <=  EPSLN ))
       {
	 /* no other constants needed for LA with North and South Aspects lat_center = 90 or -90*/
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

/* Lambert Azimuthal Equal Area inverse equations--mapping x,y to lat,long 
  -----------------------------------------------------------------------*/
int lamazinv(
double x,		/* (I) X projection coordinate */
double y,		/* (I) Y projection coordinate */
double *lon,		/* (O) Longitude */
double *lat)		/* (O) Latitude */
{
double Rh;
double z;		/* Great circle dist from proj center to given point */
double sin_z;		/* Sine of z */
double cos_z;		/* Cosine of z */
double sin_ce;		/* Sine of ce */
double cos_ce;		/* Cosine of ce */
double temp;		/* Re-used temporary variable */
double ce;
double arg;
double rho, q, beta;
double q_test;
double phi, cos_phi, sin_phi,delta_phi;
double esin_phi, one_m_essin2_phi;
double one_m_es, one_over_2e;
/* double epsilon = 1e-6. Used in azimuthal_equal_area.c code */
 int it_max = 35;
 int i;

/* Inverse equations
  -----------------*/
x -= false_easting;
y -= false_northing;


 if( ind != 0) /* sphere */
   {
     Rh = sqrt(x * x + y * y);
     temp = Rh / (2.0 * R);
     if (temp > 1) 
       {
	 p_error("Input data error", "lamaz-inverse");
	 return(115);
       }
     z = 2.0 * asinz(temp);
     tsincos(z, &sin_z, &cos_z);
     *lon = lon_center;
     if (fabs(Rh) > EPSLN)
       {
	 *lat = asinz(sin_lat_o * cos_z + cos_lat_o * sin_z * y / Rh);
	 temp = fabs(lat_center) - HALF_PI;
	 if (fabs(temp) > EPSLN)
	   {
	     temp = cos_z - sin_lat_o * sin(*lat);
	     if(temp!=0.0)*lon=adjust_lon(lon_center+atan2(x*sin_z*cos_lat_o,temp*Rh));
	   }
	 else if (lat_center < 0.0) *lon = adjust_lon(lon_center - atan2(-x, y));
	 else *lon = adjust_lon(lon_center + atan2(x, -y));
       }
     else *lat = lat_center;
   }
 else /* ellipsoid */
   {
     if (fabs(fabs(lat_center) - HALF_PI) <= EPSLN)  /* if Northern or Southern Polar Aspect */
       {
	 rho = sqrt(x * x + y * y);
	 ce = rho / r_major; 
	 arg = 1.0 - (rho * rho)/(qp *(r_major * r_major));
	 beta = sign(lat_center) * asinz(arg);
	 q = sign(lat_center) * (qp - ce * ce);
	 *lon = adjust_lon(lon_center + atan2(x, sign(lat_center) * (-y)));
       }
     else
       {
	 rho = sqrt(((x*x)/(D*D))+(D*D*y*y));
	 ce = 2.0 * asinz(rho / (2.0 *Rq));
	 tsincos(ce, &sin_ce, &cos_ce);
	 *lon = adjust_lon(lon_center + 
			   atan2( x * sin_ce, (D * rho * cos_beta1 * cos_ce - 
					       D * D * y * sin_beta1 * sin_ce)));

	 if(fabs(rho) <= EPSLN)
	   {
	     q = qp * sin_beta1;
	   }
	 else
	   {
	     q = qp *((cos_ce * sin_beta1) + ( D * y * sin_ce * cos_beta1 / rho));
	   }
       }

     /* calculate phi by itteration using equation 3-16, Snyder 1987, p188 */
      
     q_test = 1.0 - (1.0 - es)/(2.0 * e) * log((1.0 - e) / (1.0 + e));

     if (fabs(fabs(q) - fabs(q_test)) < EPSLN) 
       {
	 *lat = sign(q) * PI / 2;
       } 
     else 
       {
	 phi = asinz(q / 2.0);
	 one_m_es = 1.0 - es;
	 one_over_2e = 1.0 / (2.0 * e);
	 
	 for (i = 0; i < it_max; i++) 
	   {
	     tsincos(phi, &sin_phi, &cos_phi);
	     if (cos_phi < EPSLN) 
	       {
		 phi = sign(q) * PI / 2;
		 break;
	       }
	     esin_phi = e * sin_phi;
	     one_m_essin2_phi = 1.0 - esin_phi * esin_phi;
	     delta_phi = one_m_essin2_phi * one_m_essin2_phi / (2.0 * cos_phi) *
	       (q / one_m_es - sin_phi / one_m_essin2_phi + one_over_2e *
		log((1.0 - esin_phi) / (1.0 + esin_phi)));
	     phi += delta_phi;
	     if (fabs(delta_phi) < EPSLN)
	       break;
	   }
	 *lat = phi;
       }
     
     /* Approximate calculation of lat using series

     if (fabs(lat_center - HALF_PI) <= EPSLN)  // if Norther Polar Aspect
       {
	 *lat = beta +((((es / 3.0) + ((31.0/180.0) * e_p4)+
			 ((517.0/5040.0) * e_p6)) * sin(2.0*beta))+
		       ((((23.0/360.0) * e_p4)+
			 ((251.0/3780.0) * e_p6)) * sin(4.0*beta))+
		       (((761.0/45360.0) * e_p6) * sin(6.0*beta)));
	 *lon = adjust_lon(lon_center + atan2(x, -y));
       }
       else if (fabs(lat_center + HALF_PI) <= EPSLN)  // if Souther Polar Aspect
       {
	 *lat = beta +((((es / 3.0) + ((31.0/180.0) * e_p4)+
			 ((517.0/5040.0) * e_p6)) * sin(2.0*beta))+
		       ((((23.0/360.0) * e_p4)+
			 ((251.0/3780.0) * e_p6)) * sin(4.0*beta))+
		       (((761.0/45360.0) * e_p6) * sin(6.0*beta)));
	 *lon = adjust_lon(lon_center + atan2(x, y));
       }
       else // with any other latitude of origin
       {
	 rho = sqrt(((x*x)/(D*D))+(D*D*y*y));
	 if(fabs(rho) <= EPSLN)
	   {
	     ce = 0.0;
	     q = qp * sin_beta1;
	     beta = beta1;
	     *lon = adjust_lon(lon_center);
	     *lat = beta +((((es / 3.0) + ((31.0/180.0) * e_p4)+
			     ((517.0/5040.0) * e_p6)) * sin(2.0*beta))+
			   ((((23.0/360.0) * e_p4)+
			     ((251.0/3780.0) * e_p6)) * sin(4.0*beta))+
			   (((761.0/45360.0) * e_p6) * sin(6.0*beta)));
	   }
	 else
	   {
	     ce = 2.0 * asinz(rho / (2.0 *Rq));
	     tsincos(ce, &sin_ce, &cos_ce);
	     q = qp *((cos_ce * sin_beta1) + ( D * y * sin_ce * cos_beta1 / rho));
	     beta = asinz(q/qp);
	     
	     *lon = adjust_lon(lon_center + 
			       atan2( x * sin_ce, (D * rho * cos_beta1 * cos_ce - 
						  D * D * y * sin_beta1 * sin_ce)));
	     *lat = beta +((((es / 3.0) + ((31.0/180.0) * e_p4)+
			     ((517.0/5040.0) * e_p6)) * sin(2.0*beta))+
			   ((((23.0/360.0) * e_p4)+
			     ((251.0/3780.0) * e_p6)) * sin(4.0*beta))+
			   (((761.0/45360.0) * e_p6) * sin(6.0*beta)));
	   }
       }
     */
   }

return(OK);
}
