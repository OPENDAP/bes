/*
 * FILENAME: err.h
 *              
 * PURPOSE: contains error defined constants and prototypes for error.c 
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


/* Avoid multiple includes */
#ifndef ERR_H__
#define ERR_H__

#include <errno.h>

/* General Errors */

#define ERR_GENERAL             500
#define ERR_OPEN_FILE           501
#define ERR_READ_FILE           502
#define ERR_WRITE_FILE          503
#define ERR_PTR_DEF             504 /* for menu library */
#define ERR_MEM_LACK            505
#define ERR_UNKNOWN             506 /* for menu library */
#define ERR_FIND_FILE           507
#define ERR_FILE_DEFINED        508
#define ERR_OUT_OF_RANGE        510
#define ERR_PROCESS_DATA        515
#define ERR_NUM_TOKENS          519
#define ERR_FILE_EXISTS         522
#define ERR_CREATE_FILE         523
#define ERR_WILL_OVERWRITE_FILE 524
#define ERR_REMOVE_FILE         525
#define ERR_WONT_OVERWRITE_FILE 526


/* Freeform Errors */
#define ERR_UNKNOWN_VAR_TYPE    1000
#define ERR_UNKNOWN_PARAMETER   1001
#define ERR_CONVERT             1003
#define ERR_MAKE_FORM           1004
#define ERR_NO_VARIABLES        1012

#define ERR_FIND_FORM           1021
#define ERR_CONVERT_VAR         1022
#define ERR_ORPHAN_VAR          1023
#define ERR_POSSIBLE            1024

/* Binview Errors */

#define ERR_SET_DBIN            1509
#define ERR_PARTIAL_RECORD      1517
#define ERR_NT_DEFINE           1520
#define ERR_UNKNOWN_FORMAT_TYPE 1525

#define ERR_EQN_SET             2000

#define ERR_SDTS                2500
#define ERR_SDTS_BYE_ATTR       2501

/* String database and menu systems */

#define ERR_MAKE_MENU_DBASE     3000
#define ERR_NO_SUCH_SECTION     3001
#define ERR_GETTING_SECTION     3002
#define ERR_MENU                3003

#define ERR_MN_BUFFER_TRUNCATED 3500
#define ERR_MN_SEC_NFOUND       3501
#define ERR_MN_FILE_CORRUPT     3502
#define ERR_MN_REF_FILE_NFOUND  3503

/* Parameters and parsing */

#define ERR_MISSING_TOKEN       4001
#define ERR_PARAM_VALUE         4006
#define ERR_UNKNOWN_OPTION      4013
#define ERR_IGNORED_OPTION      4014
#define ERR_VARIABLE_DESC       4015
#define ERR_VARIABLE_SIZE       4016
#define ERR_NO_EOL              4017

/* ADTLIB errors */

#define ERR_FIND_MAX_MIN        6001
#define ERR_PARSE_EQN           6002
#define ERR_EE_VAR_NFOUND       6003
#define ERR_CHAR_IN_EE          6004
#define ERR_EE_DATA_TYPE        6005
#define ERR_NDARRAY             6006
#define ERR_GEN_QUERY           6007

/* NAME_TABLE errors */

#define ERR_EXPECTING_SECTION_START  7001
#define ERR_EXPECTING_SECTION_END    7002
#define ERR_MISPLACED_SECTION_START  7003
#define ERR_MISPLACED_SECTION_END    7004
#define ERR_NT_MERGE                 7005
#define ERR_NT_KEYNOTDEF             7006
#define ERR_EQV_CONTEXT              7007

/* FreeForm ND errors */
#define ERR_GEN_ARRAY                7501

/* internal messaging errors */

#define ERR_API                      7900
#define ERR_SWITCH_DEFAULT           7901
#define ERR_ASSERT_FAILURE           7902
#define ERR_NO_NAME_TABLE            7903
#define ERR_API_BUF_LOCKED           7904
#define ERR_API_BUF_NOT_LOCKED       7905

/* Do NOT create any error codes that exceed those below */

#define ERR_WARNING_ONLY             16000 /* Don't change this number either */

typedef int  ERR_BOOLEAN;

#ifdef TRUE /* CodeWarrior for Mac is picky about this -rf01 */
#undef TRUE
#endif	/* end #ifdef TRUE -rf01 */
#define TRUE                    1
#ifdef FALSE /* CodeWarrior for Mac is picky about this -rf01 */
#undef FALSE
#endif	/* end #ifdef FALSE -rf01 */
#define FALSE                   0

int   err_count(void);
int   err_pop(void);
void  err_clear(void);
int   err_disp(FF_STD_ARGS_PTR std_args);
void  err_end(void);

int err_push(const int, const char *, ...);
int verr_push(const int ercode, const char *format, va_list va_args);

ERR_BOOLEAN err_state(void);

#endif /* (NOT) ERR_H__ */
