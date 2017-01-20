/*******************************************************************************
NAME                    Projection support routines listed below

PURPOSE:	The following functions are included in REPORT.C	

		INIT:
			Initializes the output device for error messages and
		  	report headings.

		P_ERROR:
			Reports errors to the terminal, a specified file, or
			both.

		PTITLE, RADIUS, RADIUS2, CENLON, CENLONMER, CENLAT, ORIGIN,
		STANPARL, STPARL1, OFFSET, GENRPT, GENRPT_LONG, PBLANK:
			Reports projection parameters to the terminal,
			specified file, or both. 


PROGRAMMER              DATE		REASON
----------              ----		------
D. Steinwand, EROS      July, 1991	Initial development.
T. Mittan		Mar,  1993	Adapted code to new "C" version of
					GCTP library.
S. Nelson		Jun, 1993	Added inline code. 
					Added error messages if no filename
					was specified.
Carol S. W. Tsai        Sep,  1997      Added code for p_errort, a function to report  
                                        error, to generate the status meassages that is  
                                        relative to the message file created for
                                        PGS GCT tools if it is  PGS_TOOLKIT_COMPILE      

*******************************************************************************/
#ifdef PGS_TOOLKIT_COMPILE /* only TOOLKIT compilation */
#include <PGS_SMF.h>
#include <PGS_GCT_12.h>
#endif /* only TOOLKIT compilation */
#include <stdio.h>
#include <string.h>
#include "cproj.h"

#define TRUE 1
#ifndef FALSE
#define FALSE 0
#endif

static long terminal_p;		/* flag for printing parameters to terminal */
static long terminal_e;		/* flag for printing errors to terminal */
static long file_e;		/* flag for printing errors to terminal */
static long file_p;		/* flag for printing parameters to file */
static FILE  *fptr_p=NULL;
static FILE  *fptr_e=NULL;
static char parm_file[256];
static char err_file[256];

/* Function to report errors
  -------------------------*/
void p_error(
   char *what,
   char *where)
   {
#ifdef PGS_TOOLKIT_COMPILE
   PGSt_SMF_status returnStatus = PGSGCT_E_GCTP_ERROR;
 
   if(strcmp(what, "Too many iterations in inverse") == 0) returnStatus = PGSGCT_E_ITER_EXCEEDED;
   else if(strncmp(what, "Point projects into a circle of radius", strlen("Point projects into a circle of radius")) == 0) returnStatus = PGSGCT_E_POINT_PROJECT;
   else if(strcmp(what, "Input data error") == 0) returnStatus = PGSGCT_E_INPUT_DATA_ERROR;
   else if(strcmp(what, "Point projects into infinity") == 0) returnStatus = PGSGCT_E_INFINITE;
   else if(strcmp(what, "Iteration failed to converge") == 0) returnStatus = PGSGCT_E_ITER_FAILED;
   else if(strcmp(what, "Point can not be projected") == 0) returnStatus = PGSGCT_E_PROJECT_FAILED;
   else if(strcmp(what, "Point cannot be projected") == 0) returnStatus = PGSGCT_E_PROJECT_FAILED;
   else if(strcmp(what, "Transformation cannot be computed at the poles") == 0) returnStatus = PGSGCT_E_POINTS_ON_POLES;
   else if(strcmp(what, "50 iterations without conv") == 0) returnStatus = PGSGCT_E_ITER_SOM;
   else if(strcmp(what, "50 iterations without convergence") == 0) returnStatus = PGSGCT_E_ITER_SOM;
   PGS_SMF_SetDynamicMsg(returnStatus,what,where);
#else
   if (terminal_e)
      printf("[%s] %s\n",where,what);
   if (file_e)
      {
	fptr_e = (FILE *)fopen(err_file,"a");
	fprintf(fptr_e,"[%s] %s\n",where,what);
	fclose(fptr_e);
	fptr_e = NULL;
      }
#endif
   }
 
/* initialize output device
-------------------------*/
int init(
long ipr,		/* flag for printing errors (0,1,or 2)		*/
long jpr,		/* flag for printing parameters (0,1,or 2)	*/
char *efile,		/* name of error file				*/
char *pfile)		/* name of parameter file			*/

{
if (ipr == 0)
   {
   terminal_e = TRUE;
   file_e = FALSE;
   }
else
if (ipr == 1)
   {
   terminal_e = FALSE;
   if (strlen(efile) == 0)
      {
      return(6);
      }
   file_e = TRUE;
   strcpy(err_file,efile);
   }
else
if (ipr == 2)
   {
   terminal_e = TRUE;
   if (strlen(efile) == 0)
      {
      file_e = FALSE;
      p_error("Output file name not specified","report-file");
      return(6);
      }
   file_e = TRUE;
   strcpy(err_file,efile);
   }
else
   {
   terminal_e = FALSE;
   file_e = FALSE;
   }
if (jpr == 0)
   {
   terminal_p = TRUE;
   file_p = FALSE;
   }
else
if (jpr == 1)
   {
   terminal_p = FALSE;
   if (strlen(pfile) == 0)
      {
      return(6);
      }
   file_p = TRUE;
   strcpy(parm_file,pfile);
   }
else
if (jpr == 2)
   {
   terminal_p = TRUE;
   if (strlen(pfile) == 0)
      {
      file_p = FALSE;
      p_error("Output file name not specified","report-file");
      return(6);
      }
   file_p = TRUE;
   strcpy(parm_file,pfile);
   }
else
   {
   terminal_p = FALSE;
   file_p = FALSE;
   }
return(0);
}

void close_file()
{
if (fptr_e != NULL)
  {
   fclose(fptr_e);
   fptr_e = NULL;
  }
if (fptr_p != NULL)
  {
   fclose(fptr_p);
   fptr_p = NULL;
  }
}

/* Functions to report projection parameters
  -----------------------------------------*/
void ptitle(char *A) 
      {  
      if (terminal_p)
           printf("\n%s PROJECTION PARAMETERS:\n\n",A); 
      if (file_p)
	   {
           fptr_p = (FILE *)fopen(parm_file,"a");
           fprintf(fptr_p,"\n%s PROJECTION PARAMETERS:\n\n",A); 
	   fclose(fptr_p);
	   }
      }

void radius(double A)
      {
      if (terminal_p)
         printf("   Radius of Sphere:     %lf meters\n",A); 
      if (file_p)
	 {
         fptr_p = (FILE *)fopen(parm_file,"a");
         fprintf(fptr_p,"   Radius of Sphere:     %lf meters\n",A); 
	 fclose(fptr_p);
	 }
      }

void radius2(double A, double B)
      {
      if (terminal_p)
         {
         printf("   Semi-Major Axis of Ellipsoid:     %lf meters\n",A);
         printf("   Semi-Minor Axis of Ellipsoid:     %lf meters\n",B);
         }
      if (file_p)
         {
         fptr_p = (FILE *)fopen(parm_file,"a");
         fprintf(fptr_p,"   Semi-Major Axis of Ellipsoid:     %lf meters\n",A);
         fprintf(fptr_p,"   Semi-Minor Axis of Ellipsoid:     %lf meters\n",B); 
	 fclose(fptr_p);
         }
      }

void cenlon(double A)
   { 
   if (terminal_p)
       printf("   Longitude of Center:     %lf degrees\n",A*R2D);
   if (file_p)
       {
       fptr_p = (FILE *)fopen(parm_file,"a");
       fprintf(fptr_p,"   Longitude of Center:     %lf degrees\n",A*R2D);
       fclose(fptr_p);
       }
   }
 
void cenlonmer(double A)
   { 
   if (terminal_p)
     printf("   Longitude of Central Meridian:     %lf degrees\n",A*R2D);
   if (file_p)
     {
     fptr_p = (FILE *)fopen(parm_file,"a");
    fprintf(fptr_p,"   Longitude of Central Meridian:     %lf degrees\n",A*R2D);
     fclose(fptr_p);
     }
   }

void cenlat(double A)
   {
   if (terminal_p)
      printf("   Latitude  of Center:     %lf degrees\n",A*R2D);
   if (file_p)
      {
      fptr_p = (FILE *)fopen(parm_file,"a");
      fprintf(fptr_p,"   Latitude of Center:     %lf degrees\n",A*R2D);
      fclose(fptr_p);
      }
   }

void origin(double A)
   {
   if (terminal_p)
      printf("   Latitude of Origin:     %lf degrees\n",A*R2D);
   if (file_p)
      {
      fptr_p = (FILE *)fopen(parm_file,"a");
      fprintf(fptr_p,"   Latitude  of Origin:     %lf degrees\n",A*R2D);
      fclose(fptr_p);
      }
   }
void true_scale(double A)
   {
   if (terminal_p)
      printf("   Latitude of True Scale:     %lf degrees\n",A*R2D);
   if (file_p)
      {
      fptr_p = (FILE *)fopen(parm_file,"a");
      fprintf(fptr_p,"   Latitude  of True Scale:     %lf degrees\n",A*R2D);
      fclose(fptr_p);
      }
   return;
   }
void stanparl(double A, double B)
   {
   if (terminal_p)
      {
      printf("   1st Standard Parallel:     %lf degrees\n",A*R2D);
      printf("   2nd Standard Parallel:     %lf degrees\n",B*R2D);
      }
   if (file_p)
      {
      fptr_p = (FILE *)fopen(parm_file,"a");
      fprintf(fptr_p,"   1st Standard Parallel:     %lf degrees\n",A*R2D);
      fprintf(fptr_p,"   2nd Standard Parallel:     %lf degrees\n",B*R2D);
      fclose(fptr_p);
      }
   }

void stparl1(double A)
   {
   if (terminal_p)
      {
      printf("   Standard Parallel:     %lf degrees\n",A*R2D);
      }
   if (file_p)
      {
      fptr_p = (FILE *)fopen(parm_file,"a");
      fprintf(fptr_p,"   Standard Parallel:     %lf degrees\n",A*R2D);
      fclose(fptr_p);
      }
   }

void offsetp(double A, double B)
   {
   if (terminal_p)
      {
      printf("   False Easting:      %lf meters \n",A);
      printf("   False Northing:     %lf meters \n",B);
      }
   if (file_p)
      {
      fptr_p = (FILE *)fopen(parm_file,"a");
      fprintf(fptr_p,"   False Easting:      %lf meters \n",A);
      fprintf(fptr_p,"   False Northing:     %lf meters \n",B);
      fclose(fptr_p);
      }      
   }

void genrpt(double A, char *S)
   {
   if (terminal_p)
      printf("   %s %lf\n", S, A);
   if (file_p)
      {
      fptr_p = (FILE *)fopen(parm_file,"a");
      fprintf(fptr_p,"   %s %lf\n", S, A);
      fclose(fptr_p);
      }
   }
void genrpt_long(long A, char *S)
   {
   if (terminal_p)
      printf("   %s %ld\n", S, A);
   if (file_p)
      {
      fptr_p = (FILE *)fopen(parm_file,"a");
      fprintf(fptr_p,"   %s %ld\n", S, A);
      fclose(fptr_p);
      }
   }
void pblank() 
   {
   if (terminal_p)
      printf("\n");
   if (file_p)
      {
      fptr_p = (FILE *)fopen(parm_file,"a");
      fprintf(fptr_p,"\n");
      fclose(fptr_p);
      }
   }

