/*
 *	FILENAME: name_tab.h
 *
 *	CONTAINS: This is the include file for much of the name table
 *		  code: symbols, structures, and prototypes. 
 *			More is found in include file
 *		  databin.h, which includes this file.
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
 
#ifndef NAME_TAB_H__
#define NAME_TAB_H__

#define NTKN_INPUT_EQV         "input_eqv"
#define NTKN_OUTPUT_EQV        "output_eqv"
#define NTKN_BEGIN_CONSTANT    "begin constant"
#define NTKN_BEGIN_NAME_EQUIV  "begin name_equiv"
#define NTKN_END_CONSTANT      "end constant"
#define NTKN_END_NAME_EQUIV    "end name_equiv"

/* NAME_TABLE values' buffer is in multiples of 256 */
#define NAME_TABLE_QUANTA 256

#define INPUT_NAME_TABLE_EXISTS(dbin) ( (dbin) ? !fd_get_format_data(dbin->table_list, FFF_INPUT | FFF_TABLE, NULL) : 0)

/* Name Table Prototypes */
int nt_parse(char *origin, FF_BUFSIZE_PTR, NAME_TABLE_HANDLE);
NAME_TABLE_PTR nt_create(char *origin);

int nt_merge_name_table(NAME_TABLE_LIST_HANDLE, NAME_TABLE_PTR);

void nt_free_trans(TRANSLATOR_PTR trans);

BOOLEAN nt_copy_translator_sll(VARIABLE_PTR source_var, VARIABLE_PTR target_var);
BOOLEAN nt_comp_translator_sll(VARIABLE_PTR var1, VARIABLE_PTR var2);

int nt_show(NAME_TABLE_PTR, FF_BUFSIZE_PTR);

BOOLEAN nt_get_geovu_value
	(
	 NAME_TABLE_PTR table,
	 char *gvalue_name,
	 void *user_value,
	 FF_TYPES_t uvalue_type,
	 void *value,
	 FF_TYPES_t *value_type
	);

BOOLEAN nt_get_user_value
	(
	 NAME_TABLE_PTR table,
	 char *gvalue_name,
	 void *geovu_value,
	 FF_TYPES_t gvalue_type, 
	 void *user_value,
	 FF_TYPES_t *uvalue_type
	);

int ff_string_to_binary
	(
	 char *variable_str,
	 FF_TYPES_t output_type,
	 char *destination
	);

#endif /* (NOT) NAME_TAB_H__ */

