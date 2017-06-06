/*
 * FILENAME:  ff2psdts.h
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

#ifndef FF2PSDTS_H__
#define FF2PSDTS_H__

#define MODULE_COUNT 20

typedef struct fips_struct_module_stats_struct_t
{
	char mnrf[5]; /* name of module */
	char mntf[27]; /* type of module */
	FILE *fp; /* FILE pointer for modules being appended to */
	int  nrec; /* number of records in module */
	int  nsad; /* number of spatial addresses in module */
} FIPS_MODULE_STATS, *FIPS_MODULE_STATS_PTR;

#define fips_MAX_FILE_PATH 256

typedef struct attr_info_ptr_struct_t
{
	char *name;
	enum {ATYP_ALPHABET = 1, ATYP_INTEGER = 2, ATYP_ALPHANUM = 3, ATYP_REAL = 4, ATYP_GRCHARS = 5} atyp;
} FIPS_ATTR_INFO, *FIPS_ATTR_INFO_PTR;

typedef struct fips_bucket_struct_t
{
	char *transfer_name;
	FIPS_ATTR_INFO_PTR attr_info;
	FIPS_MODULE_STATS module_stats[MODULE_COUNT];
} FIPS_BUCKET, *FIPS_BUCKET_PTR;

#endif
