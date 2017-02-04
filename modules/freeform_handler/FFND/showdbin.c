/* FILENAME: showdbin.c
 *
 * CONTAINS: 
 * Public functions:
 *
 * db_ask
 *
 * Private functions:
 *
 * add_format_info
 * create_format_info
 * create_format_info_list
 * set_bufsize
 * dbask_format_summary
 * show_format_info_list
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

/*
 * NAME:        db_ask
 *              
 * PURPOSE:     db_ask shows various attributes of a data bin.
 *
 * USAGE:       db_ask(DATA_BIN_PTR, attribute, [args], attribute, args,... NULL)
 *
 * RETURNS:     0 if all goes well.
 *
 * DESCRIPTION: Provides information about a data bin according to message code.
 * The message codes and their arguments are:
 *
 * DBASK_PROCESS_INFO: FF_TYPES_t format_type, PROCESS_INFO_LIST_HANDLE hpinfo_list
 * DBASK_FORMAT_SUMMARY: FF_BUFSIZE_HANDLE hbufsize
 * DBASK_VAR_NAMES: FF_TYPES_t format_type, int *num_names, char **names_vector
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * AUTHOR:      T. Habermann, NGDC, (303) 497 - 6472, haber@mail.ngdc.noaa.gov
 *
 * COMMENTS:
 *                                 
 * KEYWORDS: databins
 *
*/

#include <freeform.h>

#define IS_LAST_EOL_VAR(vlist) ((vlist) ? IS_EOL(FF_VARIABLE(vlist)) && FF_VARIABLE((vlist)->next) == NULL : FALSE)

static int get_format_type_and_title
	(
	 FORMAT_PTR format,
	 FF_BUFSIZE_PTR bufsize
	)
{
	FF_TYPES_t save_format_type = 0;

	FF_VALIDATE(format);
	FF_VALIDATE(bufsize);

	/* print format name */
	if (bufsize->total_bytes - bufsize->bytes_used + strlen(format->name) < LOGGING_QUANTA)
	{
		if (ff_resize_bufsize(bufsize->total_bytes + strlen(format->name) + LOGGING_QUANTA, &bufsize))
			return(ERR_MEM_LACK);
	}

	save_format_type = format->type;
#if 0
	if (IS_ASCII(format))
	{
		format->type &= ~FFF_ASCII;
		format->type |= FFF_FLAT;
	}
#endif
	sprintf(bufsize->buffer + bufsize->bytes_used, "%s\t\"%s\"\n", ff_lookup_string(format_types, FFF_FORMAT_TYPE(format)), strchr(format->name, '\b') ? strchr(format->name, '\b') + 1 : format->name);
	bufsize->bytes_used += strlen(bufsize->buffer + bufsize->bytes_used);

	format->type = save_format_type;

	return 0;
}

/*
 * NAME:	ff_format_info
 *			   
 * PURPOSE:	To get the header information from a format.
 *				     
 * USAGE:	ff_format_info(FORMAT *format, char *scratch_buffer)
 *		  
 * RETURNS:	Zero on success, an error code on failure
 *
 * DESCRIPTION:	This function prints the information from a format header
 *				into a character buffer. This information includes:
 *					the number of variables
 *					the maximum length of the format
 *					the format type
 *					
 * Any new text is appended to bufsize.
 *
 * SYSTEM DEPENDENT FUNCTIONS:	
 *
 * ERRORS:
 *
 * AUTHOR:  Mark Ohrenschall, NGDC, (303) 497-6124, mao@ngdc.noaa.gov
 *
 * COMMENTS:	The buffer must be large enough to hold the information.
 *
 * KEYWORDS:	
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "ff_format_info"

static int ff_format_info(PROCESS_INFO_PTR pinfo, FF_BUFSIZE_PTR bufsize)
{
	int error = 0;

	FF_VALIDATE(pinfo);
	FF_VALIDATE(bufsize);

	if (bufsize->total_bytes - bufsize->bytes_used < LOGGING_QUANTA)
	{
		if (ff_resize_bufsize(bufsize->total_bytes + LOGGING_QUANTA, &bufsize))
			return(ERR_MEM_LACK);
	}

	sprintf(bufsize->buffer + bufsize->bytes_used,"\n(%s) ", PINFO_ORIGIN(pinfo));

	bufsize->bytes_used += strlen(bufsize->buffer + bufsize->bytes_used);
	if (bufsize->total_bytes - bufsize->bytes_used < LOGGING_QUANTA)
	{
		if (ff_resize_bufsize(bufsize->total_bytes + LOGGING_QUANTA, &bufsize))
			return(ERR_MEM_LACK);
	}

	error = get_format_type_and_title(PINFO_FORMAT(pinfo), bufsize);
	if (error)
		return error;

	if (bufsize->total_bytes - bufsize->bytes_used < LOGGING_QUANTA)
	{
		if (ff_resize_bufsize(bufsize->total_bytes + LOGGING_QUANTA, &bufsize))
			return(ERR_MEM_LACK);
	}

	if (PINFO_IS_FILE(pinfo))
		sprintf(bufsize->buffer + bufsize->bytes_used, "File %s", PINFO_FNAME(pinfo));
	else
		sprintf(bufsize->buffer + bufsize->bytes_used, "Program memory");

	bufsize->bytes_used += strlen(bufsize->buffer + bufsize->bytes_used);
	if (bufsize->total_bytes - bufsize->bytes_used < LOGGING_QUANTA)
	{
		if (ff_resize_bufsize(bufsize->total_bytes + LOGGING_QUANTA, &bufsize))
			return(ERR_MEM_LACK);
	}

	sprintf(bufsize->buffer + bufsize->bytes_used,
	        " contains %ld %s%s%s %s%s (%ld byte%s)\n",
	        (long)(PINFO_ARRAY_MAP(pinfo) ? PINFO_SUPER_ARRAY_ELS(pinfo) : 0),
			IS_REC_HEADER(PINFO_FORMAT(pinfo)) ? "(or more) " : "",
	        !FD_IS_NATIVE_BYTE_ORDER(PINFO_FD(pinfo)) && IS_BINARY(PINFO_FORMAT(pinfo)) ? "byteswapped " : "",
			IS_DATA(PINFO_FORMAT(pinfo)) ? "data" : "header",
	        IS_ARRAY(PINFO_FORMAT(pinfo)) ? "element" : "record",
	        PINFO_ARRAY_MAP(pinfo) ? (PINFO_SUPER_ARRAY_ELS(pinfo) == 1 ? "" : "s") : "s",
	        (long)(PINFO_ARRAY_MAP(pinfo) ? PINFO_ARRAY_BYTES(pinfo) : 0),
	        PINFO_ARRAY_MAP(pinfo) ? (PINFO_ARRAY_BYTES(pinfo) == 1 ? "" : "s") : "s");

	bufsize->bytes_used += strlen(bufsize->buffer + bufsize->bytes_used);

	if (bufsize->total_bytes - bufsize->bytes_used < LOGGING_QUANTA)
	{
		if (ff_resize_bufsize(bufsize->total_bytes + LOGGING_QUANTA, &bufsize))
			return(ERR_MEM_LACK);
	}


	sprintf(bufsize->buffer + bufsize->bytes_used,
	        "Each %s contains %u field%s and is %u %s%s long.\n",
			  IS_ARRAY_TYPE(PINFO_TYPE(pinfo)) ? "element" : "record",
		      (unsigned)PINFO_NUMVARS(pinfo), PINFO_NUMVARS(pinfo) == 1 ? "" : "s",
		      (unsigned)PINFO_RECL(pinfo),
		      IS_BINARY_TYPE(PINFO_TYPE(pinfo)) ? "byte" : "character",
		      PINFO_RECL(pinfo) > 1 ? "s" : ""
		     );
	bufsize->bytes_used += strlen(bufsize->buffer + bufsize->bytes_used);

	return(0);
}

static PROCESS_INFO_PTR create_process_info(FF_ARRAY_DIPOLE_PTR pole)
{
	PROCESS_INFO_PTR pinfo = NULL;
	
	FF_VALIDATE(pole);
	
	pinfo = (PROCESS_INFO_PTR)memMalloc(sizeof(PROCESS_INFO), "pinfo");
	if (pinfo)
	{
#ifdef FF_CHK_ADDR
		pinfo->check_address = (void *)pinfo;
#endif
		pinfo->pole = pole;

		pinfo->name = memStrdup(pole->name, "pole->name");
		if (!pinfo->name)
		{
			memFree(pinfo, "pinfo");
			pinfo = NULL;
			err_push(ERR_MEM_LACK, "");
		}

#ifdef FF_CHK_ADDR
		pinfo->locked_buffer = NULL;
#endif

		if (pole->mate)
		{
			pinfo->mate = (PROCESS_INFO_PTR)memMalloc(sizeof(PROCESS_INFO), "pinfo->mate");
			if (pinfo->mate)
			{
#ifdef FF_CHK_ADDR
				pinfo->mate->check_address = (void *)pinfo->mate;
#endif
				pinfo->mate->pole = pole->mate;
				pinfo->mate->mate = pinfo; /* This may not be necessary */

				pinfo->mate->name = memStrdup(pole->mate->name, "pole->mate->name");
				if (!pinfo->mate->name)
				{
					memFree(pinfo->name, "pinfo->name");
					memFree(pinfo, "pinfo");
					pinfo = NULL;
					err_push(ERR_MEM_LACK, "");
				}

#ifdef FF_CHK_ADDR
				pinfo->mate->locked_buffer = NULL;
#endif
			}
			else
			{
				memFree(pinfo, "pinfo");
				pinfo = NULL;
				err_push(ERR_MEM_LACK, NULL);
			}
		}
		else
			pinfo->mate = NULL;
	}
	else
		err_push(ERR_MEM_LACK, "");

	return(pinfo);
}

#undef ROUTINE_NAME
#define ROUTINE_NAME "dbask_format_summary"

/*
 * NAME: dbask_format_summary
 *
 * PURPOSE: This function lists the formats in a format list into a buffer.
 *
 * AUTHOR: T. Habermann, NGDC, (303) 497 - 6472, haber@ngdc.noaa.gov
 *
 * USAGE: db_ask_format_list(FORMAT_LIST format_list, char *buffer)
 *
 * DESCRIPTION: 
 *
 * COMMENTS:  
 *
 * RETURNS:     
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */

static int dbask_format_summary
	(
	 DATA_BIN_PTR dbin,
	 FF_TYPES_t format_type,
	 FF_BUFSIZE_HANDLE hbufsize
	)
{
	int error;

	PROCESS_INFO_LIST pinfo_list = NULL;
	PROCESS_INFO_PTR  pinfo = NULL;
	
	FF_VALIDATE(dbin);

	assert(hbufsize);
	assert(*hbufsize == NULL);

	*hbufsize = ff_create_bufsize(LOGGING_QUANTA);
	if (!*hbufsize)
		return(ERR_MEM_LACK);
			
	error = db_ask(dbin, DBASK_PROCESS_INFO, format_type, &pinfo_list);
	if (error)
		return(error);

	pinfo_list = dll_first(pinfo_list);
	pinfo = FF_PI(pinfo_list);
	while (pinfo)
	{
		FF_VALIDATE(pinfo);
		
		error = ff_format_info(pinfo, *hbufsize);
		if (error)
			return(error);

		pinfo_list = dll_next(pinfo_list);
		pinfo = FF_PI(pinfo_list);
	}

	ff_destroy_process_info_list(pinfo_list);

	return(0);
}

static long digit_count(long l)
{
	return( l ? (long)(log10(labs(l)) + 1) : 1);
}

typedef struct vdf_struct_t
{
	int var_fw      ; /* name field width */
	int start_pos_fw; /* start pos field width */
	int end_pos_fw  ; /* end pos field width */
	int type_fw     ; /* type field width */
	int prec_fw     ; /* precision field width */
	int sb_fw       ; /* separated by field width */
} VDF, *VDF_PTR;

static void init_vdf(VDF_PTR vdf)
{
	vdf->var_fw        = 0;
	vdf->start_pos_fw  = 0;
	vdf->end_pos_fw    = 0;
	vdf->type_fw       = 0;
	vdf->prec_fw       = 0;
	vdf->sb_fw         = 0;
}

static void get_var_desc_formatting
	(
	 int array_offset,
	 FORMAT_PTR format,
	 VDF_PTR vdf
	)
{
	VARIABLE_LIST vlist = NULL;
	VARIABLE_PTR var = NULL;

	FF_VALIDATE(format);

	vlist = FFV_FIRST_VARIABLE(format);
	var = FF_VARIABLE(vlist);
	while (var)
	{
		if (!IS_INTERNAL_VAR(var))
		{
			vdf->var_fw = max(vdf->var_fw, (int)strlen(IS_EOL(var) ? "EOL" : var->name));
			vdf->start_pos_fw = max(vdf->start_pos_fw, (int)digit_count(var->start_pos + array_offset));
			vdf->end_pos_fw = max(vdf->end_pos_fw, (int)digit_count(var->end_pos + array_offset));
			vdf->type_fw = max(vdf->type_fw, (int)strlen(ff_lookup_string(variable_types, FFV_DATA_TYPE(var))));
			vdf->prec_fw = max(vdf->prec_fw, (int)digit_count(var->precision));
			vdf->sb_fw = max(vdf->sb_fw, (int)digit_count(FORMAT_LENGTH(format) - FF_VAR_LENGTH(var)));
		}

		vlist = dll_next(vlist);
		var = FF_VARIABLE(vlist);
	}
}

BOOLEAN okay_to_write_array_desc(void)
{
#ifdef GEOVU
	return FALSE;
#else
	return TRUE;
#endif
}

static int display_var_desc
	(
	 int array_offset,
	 FORMAT_PTR format,
	 VDF_PTR vdf,
	 FF_BUFSIZE_PTR bufsize
	)
{
	int error = 0;

	VARIABLE_LIST vlist = NULL;
	VARIABLE_PTR var = NULL;

	FF_VALIDATE(format);
	FF_VALIDATE(bufsize);

	vlist = FFV_FIRST_VARIABLE(format);
	var   = FF_VARIABLE(vlist);
	while (var && !error)
	{
		FF_VALIDATE(var);

		if (IS_INTERNAL_VAR(var) || (!IS_ARRAY(format) && IS_ASCII(format) && IS_LAST_EOL_VAR(vlist)))
		{
			vlist = dll_next(vlist);
			var = FF_VARIABLE(vlist);

			continue;
		}

		if (bufsize->total_bytes - bufsize->bytes_used + strlen(var->name) < LOGGING_QUANTA)
		{
			if (ff_resize_bufsize(bufsize->total_bytes + strlen(var->name) + LOGGING_QUANTA, &bufsize))
				return(ERR_MEM_LACK);
		}

		os_str_replace_unescaped_char1_with_char2(' ', '%', var->name);

		sprintf(bufsize->buffer + bufsize->bytes_used, "%-*s %*d %*d ",
				  vdf->var_fw, IS_EOL(var) ? "EOL" : var->name,
				  vdf->start_pos_fw, (int)var->start_pos + array_offset,
				  vdf->end_pos_fw, (int)var->end_pos + array_offset);
		bufsize->bytes_used += strlen(bufsize->buffer + bufsize->bytes_used);

		os_str_replace_unescaped_char1_with_char2('%', ' ', var->name);

		if (IS_ARRAY(var) && okay_to_write_array_desc())
		{
			strcat(bufsize->buffer, var->array_desc_str);
			bufsize->bytes_used += strlen(bufsize->buffer + bufsize->bytes_used);

			strcpy(bufsize->buffer + bufsize->bytes_used, " OF ");
			bufsize->bytes_used += strlen(bufsize->buffer + bufsize->bytes_used);
		}

		sprintf(bufsize->buffer + bufsize->bytes_used, "%s %d\n", ff_lookup_string(variable_types, FFV_DATA_TYPE(var)), var->precision);
		bufsize->bytes_used += strlen(bufsize->buffer + bufsize->bytes_used);

		vlist = FFV_NEXT_VARIABLE(vlist);
		var   = FF_VARIABLE(vlist);
	}

	return error;
}

#ifdef ROUTINE_NAME
#undef ROUTINE_NAME
#endif
#define ROUTINE_NAME "display_array_variable_list"

/*****************************************************************************
 * NAME: display_array_variable_list
 *
 * PURPOSE: Display variables in a list to a text buffer
 *
 * USAGE:  error = display_variable_list(pinfo, bufsize_ptr);
 *
 * RETURNS:  Zero on success, an error code on failure
 *
 * DESCRIPTION:  Appends to the bufsize a text representation of a variable list.
 *
 * AUTHOR:  Mark Ohrenschall, NGDC, (303) 497-6124, mao@ngdc.noaa.gov
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * GLOBALS:
 *
 * COMMENTS:
 *
 * KEYWORDS:
 *
 * ERRORS:
 ****************************************************************************/

static int display_variable_list
	(
	 FF_ARRAY_OFFSET_t array_offset,
	 FORMAT_PTR format,
	 FF_BUFSIZE_PTR bufsize
	)
{
	VDF vdf;

	int error = 0;

	FF_VALIDATE(format);
	FF_VALIDATE(bufsize);

	init_vdf(&vdf);

	get_var_desc_formatting(array_offset, format, &vdf);
	
	error = display_var_desc(array_offset, format, &vdf, bufsize);
	
	return(error);
}

#ifdef ROUTINE_NAME
#undef ROUTINE_NAME
#endif
#define ROUTINE_NAME "display_array_format"

/*****************************************************************************
 * NAME: display_format
 *
 * PURPOSE: Display a format to a text buffer for debugging
 *
 * USAGE:  error = display_format(format, bufsize_ptr);
 *
 * RETURNS:  Zero on success, an error code on failure
 *
 * DESCRIPTION:  Appends to the bufsize a text representation of a format.
 * The format looks like:
 *
 * Format: <name>
 * <type name>
 * <num_in_list>
 * <max_length>
 * <check_address>
 * <variables...>
 *
 * AUTHOR:  Mark Ohrenschall, NGDC, (303) 497-6124, mao@ngdc.noaa.gov
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * GLOBALS:
 *
 * COMMENTS:
 *
 * KEYWORDS:
 *
 * ERRORS:
 ****************************************************************************/

static int display_format
	(
	 FF_ARRAY_OFFSET_t array_offset,
	 FORMAT_PTR format,
	 FF_BUFSIZE_PTR bufsize
	)
{
	int error = 0;

	FF_VALIDATE(format);
	FF_VALIDATE(bufsize);

	format->type &= ~FFF_VARIED;

	error = get_format_type_and_title(format, bufsize);
	if (error)
		return error;

	error = display_variable_list(array_offset, format, bufsize);

	sprintf(bufsize->buffer + bufsize->bytes_used, "\n");
	bufsize->bytes_used += strlen(bufsize->buffer + bufsize->bytes_used);
	
	return(error);
}

static int display_format_to_user
	(
	 FF_ARRAY_OFFSET_t array_offset,
	 FORMAT_PTR format,
	 FF_BUFSIZE_PTR bufsize
	)
{
	int error = 0;

	FF_VALIDATE(format);
	FF_VALIDATE(bufsize);

	error = get_format_type_and_title(format, bufsize);
	if (error)
		return error;

	if (IS_VARIED(format))
	{
		sprintf(bufsize->buffer + bufsize->bytes_used, "create_format 0 0 text 0\n");
		bufsize->bytes_used += strlen(bufsize->buffer + bufsize->bytes_used);
	}
	else
		error = display_variable_list(array_offset, format, bufsize);

	sprintf(bufsize->buffer + bufsize->bytes_used, "\n");
	bufsize->bytes_used += strlen(bufsize->buffer + bufsize->bytes_used);
	
	return(error);
}

static int display_record_variable_list
	(
	 FF_ARRAY_OFFSET_t array_offset,
	 FORMAT_PTR format,
	 FF_BUFSIZE_PTR bufsize
	)
{
	int error = 0;

	char *cp = NULL;

	VDF vdf;

	VARIABLE_LIST vlist = NULL;
	VARIABLE_PTR var = NULL;

	FF_VALIDATE(format);

	init_vdf(&vdf);

	get_var_desc_formatting(0, format, &vdf);

	vlist = FFV_FIRST_VARIABLE(format);
	var = FF_VARIABLE(vlist);
	while (var && !error)
	{
		FF_VALIDATE(var);

		if (IS_INTERNAL_VAR(var) || (IS_ASCII(format) && IS_LAST_EOL_VAR(vlist)))
		{
			vlist = dll_next(vlist);
			var = FF_VARIABLE(vlist);

			continue;
		}

		os_str_replace_unescaped_char1_with_char2(' ', '%', var->name);

		sprintf(bufsize->buffer + bufsize->bytes_used, "%*s %*d %*d ",
				  vdf.var_fw, IS_EOL(var) ? "EOL" : var->name,
				  vdf.start_pos_fw, (int)(var->start_pos + array_offset),
				  vdf.end_pos_fw, (int)(var->end_pos + array_offset));
		bufsize->bytes_used += strlen(bufsize->buffer + bufsize->bytes_used);

		os_str_replace_unescaped_char1_with_char2('%', ' ', var->name);

		if (bufsize->total_bytes - bufsize->bytes_used < LOGGING_QUANTA)
		{
			if (ff_resize_bufsize(bufsize->total_bytes + LOGGING_QUANTA, &bufsize))
			{
				error = ERR_MEM_LACK;
				goto display_record_variable_list_exit;
			}
		}

		strcpy(bufsize->buffer + bufsize->bytes_used, var->array_desc_str);
		cp = strrchr(bufsize->buffer + bufsize->bytes_used, ']');
		sprintf(cp, " %s %*d]", NDARR_SB_KEY0, (int)vdf.sb_fw, (int)(FORMAT_LENGTH(format) - FF_VAR_LENGTH(var)));
		bufsize->bytes_used += strlen(bufsize->buffer + bufsize->bytes_used);

		if (bufsize->total_bytes - bufsize->bytes_used < LOGGING_QUANTA)
		{
			if (ff_resize_bufsize(bufsize->total_bytes + LOGGING_QUANTA, &bufsize))
			{
				error = ERR_MEM_LACK;
				goto display_record_variable_list_exit;
			}
		}

		sprintf(bufsize->buffer + bufsize->bytes_used, " OF %*s %*d\n",
				  vdf.type_fw, ff_lookup_string(variable_types, FFV_DATA_TYPE(var)),
				  vdf.prec_fw, var->precision);
		bufsize->bytes_used += strlen(bufsize->buffer + bufsize->bytes_used);

		vlist = dll_next(vlist);
		var = FF_VARIABLE(vlist);
	}

	sprintf(bufsize->buffer + bufsize->bytes_used, "\n");
	bufsize->bytes_used += strlen(bufsize->buffer + bufsize->bytes_used);

display_record_variable_list_exit:

	return error;
}

static int display_record_format
	(
	 FF_ARRAY_OFFSET_t array_offset,
	 FORMAT_PTR format,
	 FF_BUFSIZE_PTR bufsize
	)
{
	int error = 0;

	FF_VALIDATE(format);
	FF_VALIDATE(bufsize);

	error = get_format_type_and_title(format, bufsize);
	if (error)
		return error;

	if (IS_VARIED(format))
	{
		sprintf(bufsize->buffer + bufsize->bytes_used, "create_format 0 0 text 0\n");
		bufsize->bytes_used += strlen(bufsize->buffer + bufsize->bytes_used);
	}
	else
		error = display_record_variable_list(array_offset, format, bufsize);

	sprintf(bufsize->buffer + bufsize->bytes_used, "\n");
	bufsize->bytes_used += strlen(bufsize->buffer + bufsize->bytes_used);
	
	return(error);
}

static int display_array_list
	(
	 PROCESS_INFO_LIST plist,
	 FF_BUFSIZE_PTR bufsize
	)
{
	VDF vdf;
	PROCESS_INFO_PTR pinfo = NULL;

	int error = 0;

	FF_VALIDATE(bufsize);

	init_vdf(&vdf);

	plist = dll_first(plist);
	pinfo = FF_PI(plist);
	while (pinfo)
	{
		FF_VALIDATE(pinfo);

		if (IS_ARRAY(PINFO_FORMAT(pinfo)))
			get_var_desc_formatting(PINFO_FIRST_ARRAY_OFFSET(pinfo), PINFO_FORMAT(pinfo), &vdf);

		plist = dll_next(plist);
		pinfo = FF_PI(plist);
	}
		
	plist = dll_first(plist);
	pinfo = FF_PI(plist);
	while (pinfo)
	{
		FF_VALIDATE(pinfo);

		if (IS_ARRAY(PINFO_FORMAT(pinfo)))
			display_var_desc(PINFO_FIRST_ARRAY_OFFSET(pinfo), PINFO_FORMAT(pinfo), &vdf, bufsize);

		plist = dll_next(plist);
		pinfo = FF_PI(plist);
	}
	
	return(error);
}

static int dbask_format_list_description
	(
	 PROCESS_INFO_LIST plist,
	 FF_BUFSIZE_PTR bufsize
	)
{
	int error = 0;

	PROCESS_INFO_PTR pinfo = NULL;

	FF_VALIDATE(bufsize);

	plist = dll_first(plist);
	pinfo = FF_PI(plist);
	while (pinfo)
	{
		FF_VALIDATE(pinfo);

		if (IS_RECORD_FORMAT(PINFO_FORMAT(pinfo)))
			error = display_record_format(PINFO_FIRST_ARRAY_OFFSET(pinfo), PINFO_FORMAT(pinfo), bufsize);
		else if (IS_ARRAY(PINFO_FORMAT(pinfo)))
		{
			error = get_format_type_and_title(PINFO_FORMAT(pinfo), bufsize);
			if (error)
				break;

			error = display_array_list(plist, bufsize);
			break;
		}
		else
			error = display_format(PINFO_FIRST_ARRAY_OFFSET(pinfo), PINFO_FORMAT(pinfo), bufsize);

		plist = dll_next(plist);
		pinfo = FF_PI(plist);
	}

	return error;
}

static int dbask_format_list_description_to_user
	(
	 PROCESS_INFO_LIST plist,
	 FF_BUFSIZE_PTR bufsize
	)
{
	int error = 0;

	PROCESS_INFO_PTR pinfo = NULL;

	FF_VALIDATE(bufsize);

	plist = dll_first(plist);
	pinfo = FF_PI(plist);
	while (pinfo)
	{
		FF_VALIDATE(pinfo);

		if (IS_RECORD_FORMAT(PINFO_FORMAT(pinfo)))
			error = display_record_format(PINFO_FIRST_ARRAY_OFFSET(pinfo), PINFO_FORMAT(pinfo), bufsize);
		else if (IS_ARRAY(PINFO_FORMAT(pinfo)))
		{
			error = get_format_type_and_title(PINFO_FORMAT(pinfo), bufsize);
			if (error)
				break;

			error = display_array_list(plist, bufsize);
			break;
		}
		else
			error = display_format_to_user(PINFO_FIRST_ARRAY_OFFSET(pinfo), PINFO_FORMAT(pinfo), bufsize);

		plist = dll_next(plist);
		pinfo = FF_PI(plist);
	}

	return error;
}

static int dbask_format_description
	(
	 FF_ARRAY_OFFSET_t array_offset,
	 FORMAT_PTR format,
	 FF_BUFSIZE_PTR bufsize
	)
{
	int error = 0;

	FF_VALIDATE(format);
	FF_VALIDATE(bufsize);

	if (IS_RECORD_FORMAT(format))
		error = display_record_format(array_offset, format, bufsize);
	else
		error = display_format(array_offset, format, bufsize);

	return error;
}

static int dbask_format_description_to_user
	(
	 FF_ARRAY_OFFSET_t array_offset,
	 FORMAT_PTR format,
	 FF_BUFSIZE_PTR bufsize
	)
{
	int error = 0;

	FF_VALIDATE(format);
	FF_VALIDATE(bufsize);

	if (IS_RECORD_FORMAT(format))
		error = display_record_format(array_offset, format, bufsize);
	else
		error = display_format_to_user(array_offset, format, bufsize);

	return error;
}

static int dbask_tab_to_array_format_description
	(
	 DATA_BIN_PTR dbin, 
	 FF_BUFSIZE_PTR bufsize
	)
{
	int error = 0;

	PROCESS_INFO_LIST plist = NULL;
	PROCESS_INFO_PTR pinfo = NULL;

	FF_VALIDATE(dbin);

	FF_VALIDATE(bufsize);

	error = db_ask(dbin, DBASK_PROCESS_INFO, 0, &plist);
	if (!error)
	{
		plist = dll_first(plist);
		pinfo = FF_PI(plist);
		while (pinfo)
		{
			if (/*IS_DATA(PINFO_FORMAT(pinfo)) &&*/ !IS_ARRAY(PINFO_FORMAT(pinfo)))
			{
				VDF vdf;

				VARIABLE_LIST vlist = NULL;
				VARIABLE_PTR var = NULL;

				error = get_format_type_and_title(PINFO_FORMAT(pinfo), bufsize);
				if (error)
					return error;

				if (IS_VARIED(PINFO_FORMAT(pinfo)))
				{
					sprintf(bufsize->buffer + bufsize->bytes_used, "create_format 0 0 text 0\n");
					bufsize->bytes_used += strlen(bufsize->buffer + bufsize->bytes_used);

					plist = dll_next(plist);
					pinfo = FF_PI(plist);

					continue;
				}
				
				if (bufsize->total_bytes - bufsize->bytes_used < LOGGING_QUANTA)
				{
					if (ff_resize_bufsize(bufsize->total_bytes + LOGGING_QUANTA, &bufsize))
					{
						error = ERR_MEM_LACK;
						goto dbask_tab_to_array_format_description_exit;
					}
				}

				init_vdf(&vdf);

				get_var_desc_formatting(0, PINFO_FORMAT(pinfo), &vdf);

				vlist = FFV_FIRST_VARIABLE(PINFO_FORMAT(pinfo));
				var = FF_VARIABLE(vlist);
				while (var && !error)
				{
					FF_VALIDATE(var);

					if (IS_INTERNAL_VAR(var))
					{
						vlist = dll_next(vlist);
						var = FF_VARIABLE(vlist);

						continue;
					}

					os_str_replace_unescaped_char1_with_char2(' ', '%', var->name);

					sprintf(bufsize->buffer + bufsize->bytes_used, "%*s %*d %*d ARRAY",
					        vdf.var_fw, IS_EOL(var) ? "EOL" : var->name,
					        vdf.start_pos_fw, (int)var->start_pos,
					        vdf.end_pos_fw, (int)var->end_pos);
					bufsize->bytes_used += strlen(bufsize->buffer + bufsize->bytes_used);

					os_str_replace_unescaped_char1_with_char2('%', ' ', var->name);

					if (bufsize->total_bytes - bufsize->bytes_used < LOGGING_QUANTA)
					{
						if (ff_resize_bufsize(bufsize->total_bytes + LOGGING_QUANTA, &bufsize))
						{
							error = ERR_MEM_LACK;
							goto dbask_tab_to_array_format_description_exit;
						}
					}

					sprintf(bufsize->buffer + bufsize->bytes_used, "[\"t\" %ld to %ld %s %*ld]",
					        (long)(IS_INPUT(PINFO_FORMAT(pinfo)) ? PINFO_SUPER_ARRAY(pinfo)->start_index[0] : PINFO_MATE_SUB_ARRAY(pinfo)->start_index[0]),
					        (long)(IS_INPUT(PINFO_FORMAT(pinfo)) ? PINFO_SUPER_ARRAY(pinfo)->end_index[0] : PINFO_MATE_SUB_ARRAY(pinfo)->end_index[0]),
					        NDARR_SB_KEY0,
					        vdf.sb_fw, (long)(PINFO_RECL(pinfo) - FF_VAR_LENGTH(var)));
					bufsize->bytes_used += strlen(bufsize->buffer + bufsize->bytes_used);
		
					if (bufsize->total_bytes - bufsize->bytes_used < LOGGING_QUANTA)
					{
						if (ff_resize_bufsize(bufsize->total_bytes + LOGGING_QUANTA, &bufsize))
						{
							error = ERR_MEM_LACK;
							goto dbask_tab_to_array_format_description_exit;
						}
					}

					sprintf(bufsize->buffer + bufsize->bytes_used, " OF %*s %*d\n",
					        vdf.type_fw, ff_lookup_string(variable_types, FFV_DATA_TYPE(var)),
					        vdf.prec_fw, var->precision);
					bufsize->bytes_used += strlen(bufsize->buffer + bufsize->bytes_used);

					vlist = dll_next(vlist);
					var = FF_VARIABLE(vlist);
				}

				sprintf(bufsize->buffer + bufsize->bytes_used, "\n");
				bufsize->bytes_used += strlen(bufsize->buffer + bufsize->bytes_used);
			}
			else
				error = dbask_format_description(0, PINFO_FORMAT(pinfo), bufsize);

			plist = dll_next(plist);
			pinfo = FF_PI(plist);
		}
	}

dbask_tab_to_array_format_description_exit:

	ff_destroy_process_info_list(plist);

	return error;
}

static int add_process_info
	(
	 FF_ARRAY_DIPOLE_PTR pole,
	 PROCESS_INFO_LIST pinfo_list
	)
{
	PROCESS_INFO_LIST step_list = NULL;
	PROCESS_INFO_PTR pinfo = NULL;
	
	pinfo = create_process_info(pole);
	if (!pinfo)
	{
		ff_destroy_process_info_list(pinfo_list);
		return(ERR_MEM_LACK);
	}
			
	step_list = dll_add(pinfo_list);
	if (!step_list)
	{
		ff_destroy_process_info(pinfo);
		ff_destroy_process_info_list(pinfo_list);
		return(ERR_MEM_LACK);
	}
			
	dll_assign(pinfo, DLL_PI, step_list);

	return(0);
}

#ifdef ROUTINE_NAME
#undef ROUTINE_NAME
#endif
#define ROUTINE_NAME "create_process_info_list"

static int create_process_info_list
	(
	 DATA_BIN_PTR dbin,
	 FF_TYPES_t format_type,
	 PROCESS_INFO_LIST_HANDLE hpinfo_list
	)
{
	FF_ARRAY_CONDUIT_LIST conduit_list = NULL;
	FF_ARRAY_CONDUIT_PTR conduit = NULL;
	BOOLEAN added = FALSE;
	int error = 0;

	if (dbin->array_conduit_list)
		FF_VALIDATE(dbin->array_conduit_list);
	else
		return(ERR_GENERAL);

	*hpinfo_list = dll_init();
	if (!*hpinfo_list)
		return(err_push(ERR_MEM_LACK, NULL));

	conduit_list = dll_first(dbin->array_conduit_list);
	conduit = FF_AC(conduit_list);
	while (conduit)
	{
		FF_VALIDATE(conduit);
		
		if (conduit->input && (conduit->input->fd->format->type & format_type) == format_type)
		{
			error = add_process_info(conduit->input, *hpinfo_list);
			if (error)
				return(error);
			
			added = TRUE;
		}
			
		if (conduit->output && (conduit->output->fd->format->type & format_type) == format_type)
		{
			error = add_process_info(conduit->output, *hpinfo_list);
			if (error)
				return(error);
			
			added = TRUE;
		}
			
		conduit_list = dll_next(conduit_list);
		conduit = FF_AC(conduit_list);
	}
	
	if (!added)
	{
		dll_free_list(*hpinfo_list);
		*hpinfo_list = NULL;
		return(ERR_GENERAL);
	}
	
	return(0);
}

static int dbask_process_info
	(
	 DATA_BIN_PTR dbin,
	 FF_TYPES_t format_type,
	 PROCESS_INFO_LIST_HANDLE hpinfo_list
	)
{
	int error = 0;
	
	assert(hpinfo_list);
	
	*hpinfo_list = NULL;
	
	error = create_process_info_list(dbin, format_type, hpinfo_list);

	return(error);
}

/*****************************************************************************
 * NAME:  dbask_var_names
 *
 * PURPOSE:  Return a string vector with formats' variables' names.
 *
 * USAGE:  error = dbask_var_names(dbin, format_type, num_names, names_vector);
 *
 * RETURNS:  Zero on success, an error code on failure.
 *
 * DESCRIPTION:  Application programmers do not call this function directly,
 * instead they call db_ask with DBASK_VAR_NAMES.
 *
 * This function finds all formats matching format_type and builds a string
 * vector with the names of the matching formats' variables' names.  This string
 * vector and its companion int num_names is like int argc and char **argv.
 *
 * Calling code should free names_vector but not its elements.
 *
 * AUTHOR:  Mark Ohrenschall, NGDC, (303) 497-6124, mao@ngdc.noaa.gov
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * GLOBALS:
 *
 * COMMENTS:  Array variables will NOT have format title and colon-colon "::" prepended
 * to the variable name.
 *
 * Are there arrays of records?  For example:
 *
 * dbase_record "one-two"
 * one 1 1 text 0
 * two 2 2 text 0
 * 
 * ASCII_input_data "fu"
 * x 1 2 ARRAY["x" 1 to 1] OF "one-two" 
 * y 3 4 ARRAY["y" 1 to 1] OF "one-two" 
 *
 * So what names do we want?  "one", "two", or "x", "y"?
 *
 * KEYWORDS:
 *
 * ERRORS:
 ****************************************************************************/

static int dbask_var_names
	(
	 DATA_BIN_PTR dbin,
	 FF_TYPES_t format_type,
	 int *num_names,
	 char **names_vector[]
	)
{
	int error = 0;
	PROCESS_INFO_LIST pinfo_list = NULL;
	PROCESS_INFO_PTR pinfo = NULL;

	int num_vars = 0;

	*num_names = 0;

	FF_VALIDATE(dbin);
	assert(format_type);
	assert(num_names);
	assert(names_vector);

	if (!dbin || !format_type || !num_names || !names_vector)
		return(err_push(ERR_API, "function argument is undefined (NULL value)"));

	error = create_process_info_list(dbin, format_type, &pinfo_list);
	if (!error)
	{
		pinfo_list = dll_first(pinfo_list);
		pinfo = FF_PI(pinfo_list);
		while (pinfo)
		{
			if (PINFO_IS_ARRAY(pinfo))
				num_vars++;
			else
				num_vars += PINFO_NUMVARS(pinfo);

			pinfo_list = dll_next(pinfo_list);
			pinfo = FF_PI(pinfo_list);
		}

		*names_vector = (char **)memMalloc(sizeof(char *) * (num_vars + 1), "*names_vector");
		if (!*names_vector)
			error = err_push(ERR_MEM_LACK, "Allocating vector of %d strings", *num_names);
	}

	if (!error)
	{
		int i = 0;

		pinfo_list = dll_first(pinfo_list);
		pinfo = FF_PI(pinfo_list);
		while (pinfo)
		{
			if (PINFO_IS_ARRAY(pinfo))
			{
#if 0
				(*names_vector)[(*num_names)++] = strstr(PINFO_NAME(pinfo), "::") + 2;
#else
				(*names_vector)[(*num_names)++] = PINFO_NAME(pinfo);
#endif
			}
			else
			{
				FORMAT_PTR format = NULL;
				VARIABLE_LIST vlist = NULL;
				VARIABLE_PTR var = NULL;

				format = PINFO_FORMAT(pinfo);
				FF_VALIDATE(format);

				vlist = FFV_FIRST_VARIABLE(format);
				var = FF_VARIABLE(vlist);
				while (var)
				{
					if (FFV_DATA_TYPE(var))
						(*names_vector)[(*num_names)++] = var->name;

					vlist = FFV_NEXT_VARIABLE(vlist);
					var = FF_VARIABLE(vlist);
				}
			}

			pinfo_list = dll_next(pinfo_list);
			pinfo = FF_PI(pinfo_list);
		}

		for (i = *num_names; i <= num_vars; i++)
			(*names_vector)[i] = NULL;

		assert(*num_names <= num_vars);
	}

	if (pinfo_list)
		ff_destroy_process_info_list(pinfo_list);

	return(error);
}

/*****************************************************************************
 * NAME:  dbask_array_dim_names
 *
 * PURPOSE:  Return a string vector with the array variable's dimension names.
 *
 * USAGE:  error = dbask_array_dim_names(dbin, array_var_name, num_dim_names, dim_names_vector);
 *
 * RETURNS:  Zero on success, an error code on failure.
 *
 * DESCRIPTION:  Application programmers do not call this function directly,
 * instead they call db_ask with DBASK_ARRAY_DIM_NAMES.
 *
 * This function finds all array variables with the given name and builds a string
 * vector with the names of its dimensions' names.  This string
 * vector and its companion int num_names is like int argc and char **argv.
 *
 * Calling code should free dim_names_vector but not its elements.
 *
 * AUTHOR:  Mark Ohrenschall, NGDC, (303) 497-6124, mao@ngdc.noaa.gov
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * GLOBALS:
 *
 * COMMENTS:  Input and output formats with the same title will cause ambiguity problems.
 *
 * KEYWORDS:
 *
 * ERRORS:
 ****************************************************************************/

static int dbask_array_dim_names
	(
	 DATA_BIN_PTR dbin,
	 char *array_var_name,
	 int *num_dim_names,
	 char **dim_names_vector[]
	)
{
	int error = 0;
	PROCESS_INFO_LIST pinfo_list = NULL;
	PROCESS_INFO_PTR pinfo = NULL;

	int num_dims = 0;

	*num_dim_names = 0;

	FF_VALIDATE(dbin);
	assert(array_var_name);
	assert(num_dim_names);
	assert(dim_names_vector);

	/* Has *dim_names_vector already been allocated?  If so, this would be a memory leak.
	   Do we automatically free *dim_names_vector, or do we force the AP to clean up?
	*/

	assert(*dim_names_vector == NULL);

	if (!dbin || !array_var_name || !num_dim_names || !dim_names_vector)
		return(err_push(ERR_API, "function argument is undefined (NULL value)"));

	error = create_process_info_list(dbin, 0, &pinfo_list);
	if (!error)
	{
		pinfo_list = dll_first(pinfo_list);
		pinfo = FF_PI(pinfo_list);
		while (pinfo)
		{
			if (PINFO_IS_ARRAY(pinfo) && !strcmp(array_var_name, PINFO_NAME(pinfo)))
			{
				num_dims += PINFO_NUM_DIMS(pinfo);
			}

			pinfo_list = dll_next(pinfo_list);
			pinfo = FF_PI(pinfo_list);
		}

		*dim_names_vector = (char **)memMalloc(sizeof(char *) * (num_dims + 1), "*dim_names_vector");
		if (!*dim_names_vector)
			error = err_push(ERR_MEM_LACK, "Allocating vector of %d strings", *num_dim_names);
	}

	if (!error)
	{
		int i = 0;

		pinfo_list = dll_first(pinfo_list);
		pinfo = FF_PI(pinfo_list);
		while (pinfo)
		{
			if (PINFO_IS_ARRAY(pinfo) && !strcmp(array_var_name, PINFO_NAME(pinfo)))
			{
				for (i = 0; i < PINFO_NUM_DIMS(pinfo); i++)
					(*dim_names_vector)[(*num_dim_names)++] = PINFO_DIM_NAME(pinfo, i);
			}

			pinfo_list = dll_next(pinfo_list);
			pinfo = FF_PI(pinfo_list);
		}

		for (i = *num_dim_names; i <= num_dims; i++)
			(*dim_names_vector)[i] = NULL;

		assert(*num_dim_names <= num_dims);
	}

	if (pinfo_list)
		ff_destroy_process_info_list(pinfo_list);

	return(error);
}

/*****************************************************************************
 * NAME:  dbask_array_dimension_info
 *
 * PURPOSE:  Return a structure with start and end indices, granularity, separation,
 * and grouping for the given array variable and dimension name.
 *
 * USAGE:  error = dbask_array_dimension_info(dbin, array_var_name, dim_names, array_dim_info);
 *
 * RETURNS:  Zero on success, an error code on failure.
 *
 * DESCRIPTION:  Application programmers do not call this function directly,
 * instead they call db_ask with DBASK_ARRAY_DIM_INFO.
 *
 * Calling code should free array_dim_info.
 *
 * AUTHOR:  Mark Ohrenschall, NGDC, (303) 497-6124, mao@ngdc.noaa.gov
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * GLOBALS:
 *
 * COMMENTS:  Input and output formats with the same title will cause ambiguity problems.
 *
 * KEYWORDS:
 *
 * ERRORS:
 ****************************************************************************/

static int dbask_array_dim_info
	(
	 DATA_BIN_PTR dbin,
	 char *array_var_name,
	 char *dim_name,
	 FF_ARRAY_DIM_INFO_HANDLE array_dim_info
	)
{
	PROCESS_INFO_LIST pinfo_list = NULL;
	PROCESS_INFO_PTR pinfo = NULL;

	int i = 0;
	int error = 0;

	FF_VALIDATE(dbin);
	assert(array_var_name);
	assert(dim_name);
	assert(array_dim_info);

	/* Has *array_dim_info already been allocated?  If so, this would be a memory leak.
	   Do we automatically free *array_dim_info, or do we force the AP to clean up?
	*/
	assert(*array_dim_info == NULL);

	*array_dim_info = NULL;

	if (!dbin || !array_var_name || !dim_name || !array_dim_info)
		return(err_push(ERR_API, "function argument is undefined (NULL value)"));

	error = create_process_info_list(dbin, 0, &pinfo_list);
	if (!error)
	{
		pinfo_list = dll_first(pinfo_list);
		pinfo = FF_PI(pinfo_list);
		while (pinfo)
		{
			if (PINFO_IS_ARRAY(pinfo) && !strcmp(array_var_name, PINFO_NAME(pinfo)))
			{
				i = 0;
				while (i < PINFO_NUM_DIMS(pinfo))
				{
					if (!strcmp(dim_name, PINFO_DIM_NAME(pinfo,i)))
					{
						*array_dim_info = (FF_ARRAY_DIM_INFO_PTR)memMalloc(sizeof(FF_ARRAY_DIM_INFO), "array_dim_info");
						if (!*array_dim_info)
							error = err_push(ERR_MEM_LACK, "");
						else
						{
#ifdef FF_CHK_ADDR
							(*array_dim_info)->check_address = *array_dim_info;
#endif
							(*array_dim_info)->start_index = PINFO_DIM_START_INDEX(pinfo, i);
							(*array_dim_info)->end_index = PINFO_DIM_END_INDEX(pinfo, i);
							(*array_dim_info)->granularity = PINFO_DIM_GRANULARITY(pinfo, i);
							(*array_dim_info)->separation = PINFO_DIM_SEPARATION(pinfo, i);
							(*array_dim_info)->grouping = PINFO_DIM_GROUPING(pinfo, i);
							(*array_dim_info)->num_array_elements = PINFO_SUPER_ARRAY_ELS(pinfo);
						}

						break;
					}

					i++;
				}

				break;
			}

			pinfo_list = dll_next(pinfo_list);
			pinfo = FF_PI(pinfo_list);
		}
	}

	if (pinfo_list)
		ff_destroy_process_info_list(pinfo_list);

	if (!*array_dim_info)
		error = err_push(ERR_GENERAL, "Couldn't get array dimension information for %s=>%s", array_var_name, dim_name);

	return(error);
}

static int dbask_var_minmaxs
	(
	 char *mm_spec,
	 DATA_BIN_PTR dbin,
	 FF_TYPES_t mm_type,
	 int num_names,
	 char *names_vector[],
	 void **mm_vector[]
	)
{
	int error = 0;
	int error_state = 0;
	int i = 0;
	char name_buffer[MAX_NAME_LENGTH];

	size_t mm_vector_size = 0;
	size_t mm_data_size = 0;
	size_t mm_type_size = 0;

	FF_VALIDATE(dbin);
	assert(mm_type);
	assert(num_names);
	assert(names_vector);
	assert(mm_vector);

	/* Has *mm_vector already been allocated?  If so, this would be a memory leak.
	   Do we automatically free *mm_vector, or do we force the AP to clean up?
	*/
	assert(*mm_vector == NULL);

	if (!dbin || !mm_type || !num_names || !names_vector || !mm_vector)
		return(err_push(ERR_API, "function argument is undefined (NULL value)"));

	mm_type_size = ffv_type_size(mm_type);
	mm_vector_size = (num_names + 1) * sizeof(double *);
	mm_data_size = num_names * mm_type_size;

	*mm_vector = (void **)memMalloc(mm_vector_size + mm_data_size, "*mm_vector");
	if (!*mm_vector)
		return(err_push(ERR_MEM_LACK, "Cannot allocate vector of %d %simums", num_names, mm_spec));

	((double **)*mm_vector)[num_names] = NULL;

	for (i = 0; i < num_names; i++)
	{
		void *data_dest = NULL;

		data_dest = (char *)*mm_vector + mm_vector_size + (i * mm_type_size);

		((double **)*mm_vector)[i] = NULL;

		sprintf(name_buffer, "%s_%simum", strstr(names_vector[i], "::") ? strstr(names_vector[i], "::") + 2 : names_vector[i], mm_spec);
		error = nt_ask(dbin, NT_INPUT, name_buffer, mm_type, data_dest);
		if (error && error != ERR_NT_KEYNOTDEF)
			error_state = err_push(error, "Problem retrieving value for %s", name_buffer);

		if (error)
		{
			sprintf(name_buffer, "%s_%s", strstr(names_vector[i], "::") ? strstr(names_vector[i], "::") + 2 : names_vector[i], mm_spec);
			error = nt_ask(dbin, NT_INPUT, name_buffer, mm_type, data_dest);
			if (error && error != ERR_NT_KEYNOTDEF)
				error_state = err_push(error, "Problem retrieving value for %s", name_buffer);
		}

		if (error)
		{
			sprintf(name_buffer, "band_%d_%s", i + 1, mm_spec);
			error = nt_ask(dbin, NT_INPUT, name_buffer, mm_type, data_dest);
			if (error && error != ERR_NT_KEYNOTDEF)
				error_state = err_push(error, "Problem retrieving value for %s", name_buffer);
		}

		if (error)
		{
			sprintf(name_buffer, "%simum_value", mm_spec);
			error = nt_ask(dbin, NT_INPUT, name_buffer, mm_type, data_dest);
			if (error && error != ERR_NT_KEYNOTDEF)
				error_state = err_push(error, "Problem retrieving value for %s", name_buffer);
		}

		if (!error)
			((double **)*mm_vector)[i] = data_dest;
	}

	if (error && error != ERR_NT_KEYNOTDEF)
		error_state = error;

	return(error_state);
}

/*****************************************************************************
 * NAME:  dbask_var_mins
 *
 * PURPOSE:  Return a vector of minimum values corresponding to the given
 * string vector of variable names.
 *
 * USAGE:  error = dbask_var_mins(dbin, mins_type, num_names, names_vector, mins_vector);
 *
 * RETURNS:  Zero on success, an error code on failure.
 *
 * DESCRIPTION:  Application programmers do not call this function directly,
 * instead they call db_ask with DBASK_VAR_MINS.
 *
 * This function calls nt_ask to see if a minimum value is defined in a header,
 * constant part of an equivalence section, or environment for each name
 * given in names_vector.  num_names and names_vector should be built with a 
 * call to db_ask:DBASK_VAR_NAMES.
 * 
 * If a minimum value is defined for a name then the corresponding pointer in the
 * vector is the address of memory containing that minimum value, with the data type
 * given by mins_type.  If no minimum value is defined then the corresponding pointer
 * will be NULL.  For example:
 *
 * double **mins_vector = NULL;
 * error = db_ask(dbin, DBASK_VAR_MINS, FFV_DOUBLE, num_names, names_vector, &mins_vector);
 * if (error)
 *    err_disp(std_args);
 *
 * if (mins_vector)
 * {
 *    int i = 0;
 *    for (i = 0; i < num_names; i++)
 *    {
 *      if (mins_vector[i])
 *        printf("Minimum of %s is %lf\n", names_vector[i], *(mins_vector[i]));
 *    }
 *
 *   free(mins_vector);
 * }
 *
 * Calling code should free mins_vector but not its elements.
 *
 * db_ask will return an error if there was a problem getting a value for any variable (but
 * not if the value is simply not defined) but other variables may have had no error.  Thus
 * an error return indicates a problem ocurred, but the presence of a NULL pointer in the
 * vector indicates a failure to get a value (either because it was not defined, or there
 * was an actual error.
 *
 * Minimums are not calculated as in Checkvar.  Instead a sequence of nt_ask calls are made
 * to see if minimum value is defined in a header, constant table, or environment.  First
 * a call for the name appended with "_minimum" is made, the a call for the name appended
 * with "_min", then a call for "band_"#"_min" where
 * # is the sequence number of the name in names_vector, and last a call for "minimum_value".
 *
 * AUTHOR:  Mark Ohrenschall, NGDC, (303) 497-6124, mao@ngdc.noaa.gov
 *
 * SYSTEM DEPENDENT FUNCTIONS:  This code assumes that pointers are the same
 * size regardless of the data type they point to.
 *
 * GLOBALS:
 *
 * COMMENTS:
 *
 * KEYWORDS:
 *
 * ERRORS:
 ****************************************************************************/

static int dbask_var_mins
	(
	 DATA_BIN_PTR dbin,
	 FF_TYPES_t mm_type,
	 int num_names,
	 char *names_vector[],
	 void **mm_vector[]
	)
{
	return dbask_var_minmaxs("min", dbin, mm_type, num_names, names_vector, mm_vector);
}

/*****************************************************************************
 * NAME:  dbask_var_maxs
 *
 * PURPOSE:  Return a vector of maximum values corresponding to the given
 * string vector of variable names.
 *
 * USAGE:  error = dbask_var_maxs(dbin, maxs_type, num_names, names_vector, maxs_vector);
 *
 * RETURNS:  Zero on success, an error code on failure.
 *
 * DESCRIPTION:  Application programmers do not call this function directly,
 * instead they call db_ask with DBASK_VAR_MAXS.
 *
 * This function calls nt_ask to see if a maximum value is defined in a header,
 * constant part of an equivalence section, or environment for each name
 * given in names_vector.  num_names and names_vector should be built with a 
 * call to db_ask:DBASK_VAR_NAMES.
 * 
 * If a maximum value is defined for a name then the corresponding pointer in the
 * vector is the address of memory containing that maximum value, with the data type
 * given by maxs_type.  If no maximum value is defined then the corresponding pointer
 * will be NULL.  For example:
 *
 * double **maxs_vector = NULL;
 * error = db_ask(dbin, DBASK_VAR_MAXS, FFV_DOUBLE, num_names, names_vector, &maxs_vector);
 * if (error)
 *   err_disp(std_args);
 *
 * if (maxs_vector)
 * {
 *    int i = 0;
 *    for (i = 0; i < num_names; i++)
 *    {
 *      if (maxs_vector[i])
 *        printf("Minimum of %s is %lf\n", names_vector[i], *(maxs_vector[i]));
 *    }
 *
 *   free(maxs_vector);
 * }
 *
 * Calling code should free maxs_vector but not its elements.
 *
 * db_ask will return an error if there was a problem getting a value for any variable (but
 * not if the value is simply not defined) but other variables may have had no error.  Thus
 * an error return indicates a problem ocurred, but the presence of a NULL pointer in the
 * vector indicates a failure to get a value (either because it was not defined, or there
 * was an actual error.
 *
 * Maximums are not calculated as in Checkvar.  Instead a sequence of nt_ask calls are made
 * to see if maximum value is defined in a header, constant table, or environment.  First
 * a call for the name appended with "_maximum" is made, then a call for the name appended
 * with "_max", then a call for "band_"#"_max" where
 * # is the sequence number of the name in names_vector, and last a call for "maximum_value".
 *
 * AUTHOR:  Mark Ohrenschall, NGDC, (303) 497-6124, mao@ngdc.noaa.gov
 *
 * SYSTEM DEPENDENT FUNCTIONS:  This code assumes that pointers are the same
 * size regardless of the data type they point to.
 *
 * GLOBALS:
 *
 * COMMENTS:
 *
 * KEYWORDS:
 *
 * ERRORS:
 ****************************************************************************/

static int dbask_var_maxs
	(
	 DATA_BIN_PTR dbin,
	 FF_TYPES_t mm_type,
	 int num_names,
	 char *names_vector[],
	 void **mm_vector[]
	)
{
	return dbask_var_minmaxs("max", dbin, mm_type, num_names, names_vector, mm_vector);
}

/*****************************************************************************
 * NAME:  dbask_var_units
 *
 * PURPOSE:  Return a vector of unit definitions corresponding to the given
 * string vector of variable names.
 *
 * USAGE:  error = dbask_var_units(dbin, num_names, names_vector, units_vector);
 *
 * RETURNS:  Zero on success, an error code on failure.
 *
 * DESCRIPTION:  Application programmers do not call this function directly,
 * instead they call db_ask with DBASK_VAR_UNITS.
 *
 * This function calls nt_ask to see if a unit definition is defined in a header,
 * constant part of an equivalence section, or environment for each name
 * given in names_vector.  num_names and names_vector should be built with a 
 * call to db_ask:DBASK_VAR_NAMES.
 * 
 * If a unit definition is defined for a name then the corresponding pointer in the
 * vector is the address of memory containing that unit definition.  If no unit
 * definition is defined then the corresponding pointer will be NULL.  For example:
 *
 * char **units_vector = NULL;
 * error = db_ask(dbin, DBASK_VAR_UNITS, num_names, names_vector, &units_vector);
 * if (error)
 *    err_disp(std_args);
 *
 * if (units_vector)
 * {
 *    int i = 0;
 *    for (i = 0; i < num_names; i++)
 *    {
 *      if (units_vector[i])
 *        printf("Units of %s are %s\n", names_vector[i], units_vector[i]);
 *    }
 *
 *   free(units_vector);
 * }
 *
 * db_ask will return an error if there was a problem getting a value for any variable (but
 * not if the value is simply not defined) but other variables may have had no error.  Thus
 * an error return indicates a problem ocurred, but the presence of a NULL pointer in the
 * vector indicates a failure to get a value (either because it was not defined, or there
 * was an actual error.
 *
 * Calling code should free units_vector but not its elements.
 *
 * A sequence of nt_ask calls are made	to see if a unit definition is defined in a header,
 * constant table, or environment.  First a call for the name appended with "_unit" is made,
 * then a call for "band_"#"_unit" where # is the sequence number of the name in names_vector,
 * and last a call for "value_unit".
 *
 * AUTHOR:  Mark Ohrenschall, NGDC, (303) 497-6124, mao@ngdc.noaa.gov
 *
 * SYSTEM DEPENDENT FUNCTIONS:  This code assumes that pointers are the same
 * size regardless of the data type they point to.
 *
 * GLOBALS:
 *
 * COMMENTS:
 *
 * KEYWORDS:
 *
 * ERRORS:
 ****************************************************************************/

static int dbask_var_units
	(
	 DATA_BIN_PTR dbin,
	 int num_names,
	 char *names_vector[],
	 char **units_vector[]
	)
{
	int i = 0;
	int error = 0;
	int error_state = 0;
	size_t units_vector_size = 0;
	size_t units_data_size = 0;

	char *data_dest = NULL;
	char name_buffer[MAX_NAME_LENGTH];

	FF_VALIDATE(dbin);
	assert(num_names);
	assert(names_vector);
	assert(units_vector);

	/* Has *units_vector already been allocated?  If so, this would be a memory leak.
	   Do we automatically free *units_vector, or do we force the AP to clean up?
	*/
	assert(*units_vector == NULL);

	if (!dbin || !num_names || !names_vector || !units_vector)
		return(err_push(ERR_API, "function argument is undefined (NULL value)"));

	units_vector_size = (num_names + 1) * sizeof(char *);
	units_data_size = num_names * MAX_NAME_LENGTH;

	*units_vector = (char **)memMalloc(units_vector_size + units_data_size, "*units_vector");
	if (!*units_vector)
		return(err_push(ERR_MEM_LACK, "Cannot allocate vector of %d strings", num_names));

	(*units_vector)[num_names] = NULL;

	data_dest = (char *)*units_vector + units_vector_size;
	for (i = 0; i < num_names; i++)
	{
		(*units_vector)[i] = NULL;

		snprintf(name_buffer, sizeof(name_buffer), "%s_unit", strstr(names_vector[i], "::") ? strstr(names_vector[i], "::") + 2 : names_vector[i]);
		error = nt_ask(dbin, NT_INPUT, name_buffer, FFV_TEXT, data_dest);
		if (error && error != ERR_NT_KEYNOTDEF)
			error_state = err_push(error, "Problem retrieving value for %s", name_buffer);

		if (error)
		{
			snprintf(name_buffer, sizeof(name_buffer), "band_%d_unit", i + 1);
			error = nt_ask(dbin, NT_INPUT, name_buffer, FFV_TEXT, data_dest);
			if (error && error != ERR_NT_KEYNOTDEF)
				error_state = err_push(error, "Problem retrieving value for %s", name_buffer);
		}

		if (error)
		{
			snprintf(name_buffer, sizeof(name_buffer), "value_unit");
			error = nt_ask(dbin, NT_INPUT, name_buffer, FFV_TEXT, data_dest);
			if (error && error != ERR_NT_KEYNOTDEF)
				error_state = err_push(error, "Problem retrieving value for %s", name_buffer);
		}

		if (!error)
		{
			(*units_vector)[i] = data_dest;
			data_dest += strlen(data_dest) + 1;
		}
	}

	return(error_state);
}

/*****************************************************************************
 * NAME:  dbask_var_flags
 *
 * PURPOSE:  Return a vector of missing flag values corresponding to the given
 * string vector of variable names.
 *
 * USAGE:  error = dbask_var_flags(dbin, flags_type, num_names, names_vector, flags_vector);
 *
 * RETURNS:  Zero on success, an error code on failure.
 *
 * DESCRIPTION:  Application programmers do not call this function directly,
 * instead they call db_ask with DBASK_VAR_FLAGS.
 *
 * This function calls nt_ask to see if a missing flag value is defined in a header,
 * constant part of an equivalence section, or environment for each name
 * given in names_vector.  num_names and names_vector should be built with a 
 * call to db_ask:DBASK_VAR_NAMES.
 * 
 * If a flag value is defined for a name then the corresponding pointer in the
 * vector is the address of memory containing that flag value, with the data type
 * given by flags_type.  If no flag value is defined then the corresponding pointer
 * will be NULL.  For example:
 *
 * double **flags_vector = NULL;
 * error = db_ask(dbin, DBASK_VAR_FLAGS, FFV_DOUBLE, num_names, names_vector, &flags_vector);
 * if (error)
 *   err_disp(std_args);
 *
 * if (flags_vector)
 * {
 *    int i = 0;
 *    for (i = 0; i < num_names; i++)
 *    {
 *      if (flags_vector[i])
 *        printf("Flag value of %s is %f\n", names_vector[i], *(flags_vector[i]));
 *    }
 *
 *   free(flags_vector);
 * }
 *
 * db_ask will return an error if there was a problem getting a value for any variable (but
 * not if the value is simply not defined) but other variables may have had no error.  Thus
 * an error return indicates a problem ocurred, but the presence of a NULL pointer in the
 * vector indicates a failure to get a value (either because it was not defined, or there
 * was an actual error.
 *
 * Calling code should free flags_vector but not its elements.
 *
 * A sequence of nt_ask calls are made	to see if a flag value is defined in a header,
 * constant table, or environment.  First a call for the name appended with "_missing_flag" is made,
 * then a call for "band_"#"_missing_flag" where # is the sequence number of the name in names_vector,
 * and last a call for "missing_flag".
 *
 * AUTHOR:  Mark Ohrenschall, NGDC, (303) 497-6124, mao@ngdc.noaa.gov
 *
 * SYSTEM DEPENDENT FUNCTIONS:  This code assumes that pointers are the same
 * size regardless of the data type they point to.
 *
 * GLOBALS:
 *
 * COMMENTS:
 *
 * KEYWORDS:
 *
 * ERRORS:
 ****************************************************************************/

static int dbask_var_flags
	(
	 DATA_BIN_PTR dbin,
	 FF_TYPES_t flags_type,
	 int num_names,
	 char *names_vector[],
	 void **flags_vector[]
	)
{
	int error = 0;
	int error_state = 0;
	int i = 0;
	char name_buffer[MAX_NAME_LENGTH];

	size_t flags_vector_size = 0;
	size_t flags_data_size = 0;
	size_t flags_type_size = 0;

	FF_VALIDATE(dbin);
	assert(flags_type);
	assert(num_names);
	assert(names_vector);
	assert(flags_vector);

	/* Has *flags_vector already been allocated?  If so, this would be a memory leak.
	   Do we automatically free *flags_vector, or do we force the AP to clean up?
	*/
	assert(*flags_vector == NULL);

	if (!dbin || !flags_type || !num_names || !names_vector || !flags_vector)
		return(err_push(ERR_API, "function argument is undefined (NULL value)"));

	flags_type_size = ffv_type_size(flags_type);
	flags_vector_size = (num_names + 1) * sizeof(double);
	flags_data_size = num_names * flags_type_size;

	*flags_vector = (void **)memMalloc(flags_vector_size + flags_data_size, "*flags_vector");
	if (!*flags_vector)
		return(err_push(ERR_MEM_LACK, "Cannot allocate vector of %d flag values", num_names));

	((double **)*flags_vector)[num_names] = NULL;

	for (i = 0; i < num_names; i++)
	{
		void *data_dest = NULL;

		data_dest = (char *)*flags_vector + flags_vector_size + (i * flags_type_size);

		((double **)*flags_vector)[i] = NULL;

		snprintf(name_buffer, sizeof(name_buffer), "%s_missing_flag", strstr(names_vector[i], "::") ? strstr(names_vector[i], "::") + 2 : names_vector[i]);
		error = nt_ask(dbin, NT_INPUT, name_buffer, flags_type, data_dest);
		if (error && error != ERR_NT_KEYNOTDEF)
			error_state = err_push(error, "Problem retrieving value for %s", name_buffer);

		if (error)
		{
			snprintf(name_buffer, sizeof(name_buffer), "band_%d_missing_flag", i + 1);
			error = nt_ask(dbin, NT_INPUT, name_buffer, flags_type, data_dest);
			if (error && error != ERR_NT_KEYNOTDEF)
				error_state = err_push(error, "Problem retrieving value for %s", name_buffer);
		}

		if (error)
		{
			snprintf(name_buffer, sizeof(name_buffer), "missing_flag");
			error = nt_ask(dbin, NT_INPUT, name_buffer, flags_type, data_dest);
			if (error && error != ERR_NT_KEYNOTDEF)
				error_state = err_push(error, "Problem retrieving value for %s", name_buffer);
		}

		if (!error)
			((double **)*flags_vector)[i] = data_dest;
	}

	if (error && error != ERR_NT_KEYNOTDEF)
		error_state = error;

	return(error_state);
}

static long dbask_bytes_to_process
	(
	 DATA_BIN_PTR dbin,
	 FF_TYPES_t pinfo_type
	)
{
	int error = 0;
	long bytes_to_process = 0;
	PROCESS_INFO_LIST pinfo_list = NULL;
	PROCESS_INFO_PTR pinfo = NULL;

	FF_VALIDATE(dbin);

	error = db_ask(dbin, DBASK_PROCESS_INFO, pinfo_type, &pinfo_list);
	if (error)
		return -1;

	pinfo_list = dll_first(pinfo_list);
	pinfo      = FF_PI(pinfo_list);
	while (pinfo)
	{
		bytes_to_process += PINFO_BYTES_LEFT(pinfo);

		pinfo_list = dll_next(pinfo_list);
		pinfo      = FF_PI(pinfo_list);
	}

	ff_destroy_process_info_list(pinfo_list);

	return(bytes_to_process);
}

#ifdef ROUTINE_NAME
#undef ROUTINE_NAME
#endif
#define ROUTINE_NAME "Show_dbin"
#define LINE_LENGTH             256

int db_ask(DATA_BIN_PTR dbin, int message, ...)
{
	va_list args;

	int error = 0;

	FF_VALIDATE(dbin);
	
	va_start(args, message);

	switch (message)
	{
		case DBASK_PROCESS_INFO:
		{
			FF_TYPES_t format_type = va_arg(args, FF_TYPES_t);
			PROCESS_INFO_LIST_HANDLE hpinfo_list = va_arg(args, PROCESS_INFO_LIST_HANDLE);

			error = dbask_process_info(dbin, format_type, hpinfo_list);
		}
		break;

	case DBASK_FORMAT_SUMMARY:
	{
		FF_TYPES_t format_type = va_arg(args, FF_TYPES_t);
		FF_BUFSIZE_HANDLE hbufsize = va_arg(args, FF_BUFSIZE_HANDLE);

		error = dbask_format_summary(dbin, format_type, hbufsize);
	}
	break;

	case DBASK_FORMAT_DESCRIPTION:
	{
		FF_ARRAY_OFFSET_t array_offset = va_arg(args, FF_ARRAY_OFFSET_t); 
		FORMAT_PTR format = va_arg(args, FORMAT_PTR);
		FF_BUFSIZE_PTR bufsize = va_arg(args, FF_BUFSIZE_PTR);

		error = dbask_format_description(array_offset, format, bufsize);
	}
	break;

	case DBASK_FORMAT_LIST_DESCRIPTION:
	{
		PROCESS_INFO_LIST plist = va_arg(args, PROCESS_INFO_LIST);
		FF_BUFSIZE_PTR bufsize = va_arg(args, FF_BUFSIZE_PTR);

		error = dbask_format_list_description(plist, bufsize);
	}
	break;

	case DBASK_FORMAT_DESCRIPTION_TO_USER:
	{
		FF_ARRAY_OFFSET_t array_offset = va_arg(args, FF_ARRAY_OFFSET_t); 
		FORMAT_PTR format = va_arg(args, FORMAT_PTR);
		FF_BUFSIZE_PTR bufsize = va_arg(args, FF_BUFSIZE_PTR);

		error = dbask_format_description_to_user(array_offset, format, bufsize);
	}
	break;

	case DBASK_FORMAT_LIST_DESCRIPTION_TO_USER:
	{
		PROCESS_INFO_LIST plist = va_arg(args, PROCESS_INFO_LIST);
		FF_BUFSIZE_PTR bufsize = va_arg(args, FF_BUFSIZE_PTR);

		error = dbask_format_list_description_to_user(plist, bufsize);
	}
	break;

	case DBASK_TAB_TO_ARRAY_FORMAT_DESCRIPTION:
	{
		FF_BUFSIZE_PTR bufsize = va_arg(args, FF_BUFSIZE_PTR);

		error = dbask_tab_to_array_format_description(dbin, bufsize);
	}
	break;

	case DBASK_VAR_NAMES:
	{
		FF_TYPES_t format_type = va_arg(args, FF_TYPES_t);
		int *num_names = va_arg(args, int *);
		char ***names_vector = va_arg(args, char ***);

		error = dbask_var_names(dbin, format_type, num_names, names_vector);
	}
	break;

	case DBASK_ARRAY_DIM_NAMES:
	{
		char *array_var_name = va_arg(args, char *);
		int *num_dims = va_arg(args, int *);
		char ***dim_names_vector = va_arg(args, char ***);

		error = dbask_array_dim_names(dbin, array_var_name, num_dims, dim_names_vector);
	}
	break;

	case DBASK_ARRAY_DIM_INFO:
	{
		char *array_var_name = va_arg(args, char *);
		char *dim_name = va_arg(args, char *);
		FF_ARRAY_DIM_INFO_HANDLE array_dim_info = va_arg(args, FF_ARRAY_DIM_INFO_HANDLE);

		error = dbask_array_dim_info(dbin, array_var_name, dim_name, array_dim_info);
	}
	break;

	case DBASK_VAR_MINS:
	{
		FF_TYPES_t mins_type = va_arg(args, FF_TYPES_t);
		int num_names = va_arg(args, int);
		char **names_vector = va_arg(args, char **);
		void ***mins_vector = va_arg(args, void ***);

		error = dbask_var_mins(dbin, mins_type, num_names, names_vector, mins_vector);
	}
	break;

	case DBASK_VAR_MAXS:
	{
		FF_TYPES_t maxs_type = va_arg(args, FF_TYPES_t);
		int num_names = va_arg(args, int);
		char **names_vector = va_arg(args, char **);
		void ***maxs_vector = va_arg(args, void ***);

		error = dbask_var_maxs(dbin, maxs_type, num_names, names_vector, maxs_vector);
	}
	break;

	case DBASK_VAR_UNITS:
	{
		int num_names = va_arg(args, int);
		char **names_vector = va_arg(args, char **);
		char ***units_vector = va_arg(args, char ***);

		error = dbask_var_units(dbin, num_names, names_vector, units_vector);
	}
	break;

	case DBASK_VAR_FLAGS:
	{
		FF_TYPES_t flags_type = va_arg(args, FF_TYPES_t);
		int num_names = va_arg(args, int);
		char **names_vector = va_arg(args, char **);
		void ***flags_vector = va_arg(args, void ***);

		error = dbask_var_flags(dbin, flags_type, num_names, names_vector, flags_vector);
	}
	break;

	case DBASK_BYTES_TO_PROCESS:
	{
		FF_TYPES_t pinfo_type = va_arg(args, FF_TYPES_t);

		error = dbask_bytes_to_process(dbin, pinfo_type);
	}
	break;

	default:
		assert(!ERR_SWITCH_DEFAULT);
		error = err_push(ERR_SWITCH_DEFAULT, "%s, %s:%d", ROUTINE_NAME, os_path_return_name(__FILE__), __LINE__);
	}               

	va_end(args);
	return(error);
}


