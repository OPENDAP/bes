/*******************************************************************************
NAME                           SPHDZ 

PURPOSE:	This function assigns values to the semimajor axis, semiminor
		axis, and radius of sphere.  If the datum code is negative,
		the first two values in the parameter array (parm) are used
		to define the values as follows:

		--If parm[0] is a non-zero value and parm[1] is greater than
		  one, the semimajor axis and radius are set to parm[0] and
		  the semiminor axis is set to parm[1]. 

		--If parm[0] is nonzero and parm[1] is greater than zero but
		  less than or equal to one, the semimajor axis and radius
		  are set to parm[0] and the semiminor axis is computed
		  from the eccentricity squared value parm[1].  This
		  algorithm is given below.

		--If parm[0] is nonzero and parm[1] is equal to zero, the
		  semimajor axis, radius, and semiminor axis are set to
		  parm[0].

		--If parm[0] equals zero and parm[1] is greater than zero,
		  the default Clarke 1866 is used to assign values to the
		  semimajor axis, radius and semiminor axis.

		--If parm[0] and parm[1] equals zero, the semimajor axis
		  and radius are set to 6370997.0 (This value is represented
	 	  as the next to last value in the datum code array) and the
		  semiminor axis is set to zero. For datum (-20), used for 
                  BCEA and/or CEA projections, the 
                  corresponding values will be 6371228.0 m.

		if a datum code is zero or greater, the semimajor and
		semiminor axis are defined by the datum code as found
		in Table A and the radius is set to 6370997.0 (except for 
                datum code of 20, where it will be 6371228.0).  If the
		datum code is greater than DATMCT+1 the default datum,
		Clarke 1866, is used to define the semimajor
		and semiminor axis and radius is set to 6370997.0 (except for 
                datum code of 20, where it will be 6371228.0). 

		The algorithm to define the semiminor axis using the
		eccentricity squared value is as follows:

		      semiminor = sqrt(1.0 - ES) * semimajor   where
		      ES = eccentricity squared

		Table A:
				SUPPORTED DATUMS
		
		 0: Clarke 1866 (default)	 1: Clarke 1880
		 2: Bessel			 3: International 1967
		 4: International 1909		 5: WGS 72
		 6: Everest			 7: WGS 66
		 8: GRS 1980/WGS 84		 9: Airy
		10: Modified Everest		11: Modified Airy
		12: Walbeck			13: Southeast Asia
		14: Australian National		15: Krassovsky
		16: Hough			17: Mercury 1960
		18: Modified Mercury 1968	19: Sphere of Radius
						    6370997 meters
                20: Sphere of Radius            21: Sphere of Radius
                    6371228 meters                  6371007.181 meters
                                                22: Hughes 1980
                
Note: Datum code of 20 is added by Abe Taaheri for 25 km global EASE grid that
      assumes an earth radius of 6371228 meters.

Note: Datum code of 21 is added by Sharon Ni for MODIS Sinusoidal data that assumes
      an earth radius of 6371007.181 meters.

Note: Datum code of 22 is added by Abe Taaheri for Hughes 1980 datum used in AMSR_E Sea Ice
      products (Semi Major Axis 6378273.0 meters, Semi Minor Axis 6356889.4485 meters) 

PROGRAMMER              DATE
----------              ----
T. Mittan	      MARCH, 1993

ALGORITHM REFERENCES

1.  Snyder, John P., "Map Projections--A Working Manual", U.S. Geological
    Survey Professional Paper 1395 (Supersedes USGS Bulletin 1532), United
    State Government Printing Office, Washington D.C., 1987.

2.  Snyder, John P. and Voxland, Philip M., "An Album of Map Projections",
    U.S. Geological Survey Professional Paper 1453 , United State Government
    Printing Office, Washington D.C., 1989.
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "cproj.h"
#include "proj.h"

static double major[23] = {6378206.4, 6378249.145, 6377397.155, 6378157.5,
                    6378388.0, 6378135.0, 6377276.3452, 6378145.0,
                    6378137.0, 6377563.396, 6377304.063, 6377340.189,
                    6378137.0, 6378155.0, 6378160.0, 6378245.0,
		    6378270.0, 6378166.0, 6378150.0, 
		    6370997.0, 6371228.0, 6371007.181, 6378273.0};

static double minor[23] = {6356583.8, 6356514.86955, 6356078.96284, 6356772.2,
                    6356911.94613, 6356750.519915, 6356075.4133,
                    6356759.769356, 6356752.31414, 6356256.91,
                    6356103.039, 6356034.448, 6356752.314245,
                    6356773.3205, 6356774.719, 6356863.0188,
                    6356794.343479, 6356784.283666, 6356768.337303,
		    6370997.0, 6371228.0, 6371007.181, 6356889.4485};


/* Finds the correct ellipsoid axis
---------------------------------*/

int sphdz(
long isph,		/* spheroid code number also known as datum	*/
double *parm,		/* projection parameters			*/
double *r_major,	/* major axis					*/
double *r_minor,	/* minor axis					*/
double *radius)		/* radius					*/
{

double t_major;		/* temporary major axis				*/
double t_minor;		/* temporary minor axis				*/
long jsph;		/* spheroid code number				*/

if (isph < 0)
   {
   t_major = fabs(parm[0]);
   t_minor = fabs(parm[1]);
   
   if (t_major  > 0.0) 
     {
     if (t_minor > 1.0)
        {
        *r_major = t_major;
  	*r_minor = t_minor;
	*radius = t_major;
        } 
     else
     if (t_minor > 0.0)
        {
        *r_major = t_major;
	*radius = t_major;
        *r_minor = (sqrt(1.0 - t_minor)) * t_major; 
        }
     else
        {
        *r_major = t_major;
	*radius = t_major;
        *r_minor = t_major;
        }
     }
   else /* t_major <= 0 */
   if (t_minor > 0.0)
     {
     *r_major = major[0];
     *radius = major[0];
     *r_minor = minor[0];
     }
   else /* t_minor <= 0 */
   if(isph == -22 ) /*Hughes 1980 */
         {
           *r_major = major[DATMCT+ 2];
	   *radius = major[DATMCT+ 2];
	   *r_minor = minor[DATMCT+ 2];
	 }
   else if(isph == -21 ) /* MODIS  Sphere*/
          {
           *r_major = major[DATMCT + 1];
           *radius = major[DATMCT + 1];
           *r_minor = 6371007.181;
          }
   else
   if(isph == -20 ) /*BCEA or CEA */
         {
           *r_major = major[DATMCT];
	   *radius = major[DATMCT];
	   *r_minor = 6371228.0;
	 }
   else
         {
	   *r_major = major[DATMCT - 1];
	   *radius = major[DATMCT - 1];
	   *r_minor = 6370997.0;
	 }
     }
else		/* isph >= 0 */
  {
  jsph = labs(isph);
  if (jsph > 22)
     {
     p_error("Invalid spheroid selection","INFORMATIONAL");
     p_error("Reset to 0","INFORMATIONAL");
     isph = 1;
     jsph = 0;
     }
  else 
  if (jsph == 22) /* check for Hughes 1980*/
     {
      *r_major = major[jsph];
      *r_minor = minor[jsph];
      *radius = major[DATMCT - 1]; /* standard sphere */
     }
  else 
  if (jsph == 21) /* check for the Modis sphere*/
     {
      *r_major = major[jsph];
      *r_minor = minor[jsph];
      *radius = major[jsph];
     }
  else 
  if (jsph == 20) /* BCEA defaults to spherical earth with radius 6371228.0 */  
       {
        *r_major = major[jsph];
        *r_minor = minor[jsph];
        *radius = major[DATMCT];
       }
  else
       {
        *r_major = major[jsph];
        *r_minor = minor[jsph];
        *radius = major[DATMCT - 1]; /* standard sphere */
       }
  }
return(OK);
}
