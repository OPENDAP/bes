/*
 * FILENAME: eval_eqn.h
 *
 * PURPOSE: Define equation constants and data structures, and prototype functions
 *
 * AUTHOR:      Kevin Frender (kbf@ngdc.noaa.gov)
 *
 * USAGE:       #include <eval_eqn.h>
 *
 * COMMENTS:	
 *              
 * CAVEAT:
 * No claims are made as to the suitability of the accompanying
 * source code for any purpose.  Although this source code has been
 * used by the NOAA, no warranty, expressed or implied, is made by
 * NOAA or the United States Government as to the accuracy and
 * functioning of this source code, nor shall the fact of distribution
 * constitute any such endorsement, and no responsibility is assumed
 * by NOAA in connection therewith.  The source code contained
 * within was developed by an agency of the U.S. Government.
 * NOAA's National Geophysical Data Center has no objection to the
 * use of this source code for any purpose since it is not subject to
 * copyright protection in the U.S.  If this source code is incorporated
 * into other software, a statement identifying this source code may be
 * required under 17 U.S.C. 403 to appear with any copyright notice.
 */

/* avoid multiple includes */
#ifndef EE_ERR_UNKNOWN

#define EE_SCRATCH_EQN_LEN 1024

/* Errors occuring in the ee_ functions */
#define EE_ERR_UNKNOWN          1
#define EE_ERR_ODD_NUM_PARENS   2
#define EE_ERR_ODD_NUM_BRACKETS 3
#define EE_ERR_MEM_LACK         4
#define EE_ERR_NO_VARS          5
#define EE_ERR_TOO_MANY_VARS    6
#define EE_ERR_DOMAIN           7
#define EE_ERR_MEM_CORRUPT      8
#define EE_ERR_POUND_SIGN       9
#define EE_ERR_DOLLAR_SIGN		10
#define EE_ERR_EQN_BAD          11
#define EE_ERR_ODD_NUM_QUOTES	12
#define EE_ERR_VAR_NAME_BAD		13
#define EE_ERR_BAD_OP_ON_CHAR	14
#define EE_ERR_BAD_OP_ON_NUM	15
#define EE_ERR_UNSUPPORTED		16
#define EE_ERR_EQN_TOO_LONG   17

/* The variable types that ee_ functions can handle */
#define EE_VAR_TYPE_UNKNOWN 0
#define EE_VAR_TYPE_NUMERIC 1
#define EE_VAR_TYPE_CHAR	2

typedef struct eqninfstruct {
#ifdef FF_CHK_ADDR
   struct eqninfstruct *check_address;
#endif
   unsigned char       *equation;       /* This is where the step-by-step instructions are */
   unsigned char       *variable_type;  /* The type of the various variables */
   void               **variable_ptr;   /* Used as a VARIABLE_PTR to VAR structs */
   char               **variable;       /* The array of variable names */
   double              *eqn_vars;       /* The "workspace" for the ee_ functions */
   int                  ee_scratch_int; /* Used internally by the ee_ parsing functions */
   int                  eqn_len;        /* Length of the instruction set */
   unsigned char        num_vars;       /* The number of variables */
   unsigned char        numconst;       /* The number of constants (numeric & text) */
   unsigned char        num_work;       /* The number of "working" variables */
   unsigned char        result;         /* The position in the array where result is found */
   unsigned char        num_strc;       /* The number of string constants */
} EQUATION_INFO, *EQUATION_INFO_PTR;

/* Functions in eval_eqn.c (There are many more, but only these should be called) */
void				ee_show_err_mesg(char *buffer, int error);
double				ee_evaluate_equation(EQUATION_INFO_PTR einfo, int *error);
EQUATION_INFO_PTR 	ee_clean_up_equation(char *eqn, int *error);

#ifdef FREEFORM
/* Functions in eqn_util.c */
#include <freeform.h>

int	ee_check_vars_exist(EQUATION_INFO_PTR einfo, FORMAT_PTR eqn_format);
int ee_set_var_values(EQUATION_INFO_PTR einfo, void *record, FORMAT_PTR eqn_format);
int ee_set_var_types(char *eqn, FORMAT_PTR eqn_format);
int ee_free_einfo(EQUATION_INFO_PTR einfo);
EQUATION_INFO_PTR ee_make_std_equation(char *equation, FORMAT_PTR eqn_format);
#endif

#ifndef FF_VALIDATE
#ifdef FF_CHK_ADDR
#define FF_VALIDATE(o) assert(o);assert((void *)o == (o)->check_address);
#else
#define FF_VALIDATE(o) assert(o);
#endif
#endif

#endif /* End of include file */

