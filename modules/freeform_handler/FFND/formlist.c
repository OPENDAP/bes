/* FILENAME:  formlist.c
 *
 * CONTAINS:    Functions for maintaining format list for data bin:
 * db_format_list_mark_io()
 * db_destroy_format_list()
 * db_find_format()
 * fd_find_format_data
 * db_find_format_is_isnot(FORMAT_LIST, ...)
 * fd_get_header
 * fd_get_data
 * fl_extract_format
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

#include <freeform.h>

#define MARK_FOR_DELETION(f) f->type = FFF_DELETE_ME
#define DELETE_MARKED_FORMATS(fl) db_delete_all_formats(fl, FFF_DELETE_ME)

static FORMAT_DATA_PTR fd_get_fd
	(
	 DATA_BIN_PTR dbin,
	 FF_TYPES_t major_type,
	 FF_TYPES_t minor_type
	)
{
	FORMAT_DATA_PTR fd = NULL;
	PROCESS_INFO_LIST finfo_list = NULL;
	PROCESS_INFO_PTR finfo = NULL;

	if (!db_ask(dbin, DBASK_PROCESS_INFO, major_type | minor_type, &finfo_list))
	{
		finfo = FF_PI(dll_first(finfo_list));
		if (finfo)
		{
			FF_VALIDATE(finfo);
		
			fd = finfo->pole->fd;
			FF_VALIDATE(fd);
		}
		
		ff_destroy_process_info_list(finfo_list);
	}
	
	return(fd);
}

static FORMAT_PTR db_find_format_is_isnot(FORMAT_LIST f_list, ...)
/*****************************************************************************
 * NAME:  db_find_format_is_isnot()
 *
 * PURPOSE:  Locate the first format in f_list whose type matches is_ftype
 * and not isnot_ftype
 *
 * USAGE:  format = db_find_format_is_isnot(f_list, ...);
 *
 * RETURNS:  FORMAT_PTR if found, NULL if not found
 *
 * DESCRIPTION:  Similar in role to db_find_format() except that format titles
 * are not searched on.  The addition of the isnot_ftype argument allows
 * formats that match the desired bit pattern to be disqualified if they have
 * any bits matching the isnot_ftype bit pattern.  This means I can search
 * for formats that are this, but not that.
 *
 * There is a subtle difference in how is_ftype and isnot_ftype are used.
 * A format must match all bits in is_ftype, but cannot match ANY bit in
 * isnot_ftype in order to be returned by this function.
 *
 * The following code snippet finds all file header formats in f_list that
 * do not have either the FFF_OUTPUT or the FFF_INPUT bit set, and sets the
 * FFF_INPUT bit (thus marking all read/write ambiguous file header formats
 * to read).
 *
 * while (format = db_find_format_is_isnot(f_list, FFF_GROUP, FFF_HEADER | FFF_FILE,
 *                                         FFF_OUTPUT | FFF_INPUT))
 * {
 *   format->type |= FFF_INPUT;
 * }
 *
 * Note that this would be an infinite loop were it not for marking all
 * returned formats in such a way that they fail the search criteria.
 *
 * If FFF_NAME is used as the second argument, then the first format matching
 * the given format type but not having the given title is returned.
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

#undef ROUTINE_NAME
#define ROUTINE_NAME "db_find_format_is_isnot"

{
	va_list args;
	FF_TYPES_t attribute, is_ftype, isnot_ftype;
	char *key_name = NULL;
	FORMAT_PTR format = NULL;
	
	assert(f_list);
	
	va_start(args, f_list);

	/* Read the arguments */
	/* this really should not be a while(), but I'm allowing a future
	   implementation of both attribute types in one call somehow */
	
	is_ftype = isnot_ftype = FFF_NULL;
	attribute = (FF_TYPES_t)va_arg(args, FF_TYPES_t);
	if (attribute == FFF_GROUP)
	{
		is_ftype = (FF_TYPES_t)va_arg (args, FF_TYPES_t);
		isnot_ftype = (FF_TYPES_t)va_arg (args, FF_TYPES_t);
				
		assert(is_ftype);
		assert(isnot_ftype);
		if (!is_ftype || !isnot_ftype)
		{
			err_push(ERR_PARAM_VALUE, "zero value format type(s)");
			return(NULL);
		}
	}
	else if (attribute == FFF_NAME_CASE)
	{
		is_ftype = (FF_TYPES_t)va_arg (args, FF_TYPES_t);
		key_name = va_arg (args, char *);

		assert(is_ftype);
		assert(key_name);
		if (!is_ftype || !key_name)
		{
			err_push(ERR_PARAM_VALUE, "zero value format type/name");
			return(NULL);
		}
	}
	else
	{
		err_push(ERR_PARAM_VALUE, "undefined search type");
		return(NULL);
	}
		
	va_end(args);

	f_list = dll_first(f_list);
	format = FF_FORMAT(f_list);
	while (format)
	{
		FF_VALIDATE(format);
		
		if ( ((format->type & is_ftype) == is_ftype) &&
		     ((attribute == FFF_GROUP && !(format->type & isnot_ftype)) ||
			    (attribute == FFF_NAME_CASE && strcmp(format->name, key_name))))
			break;
    
		f_list = dll_next(f_list);
		format = FF_FORMAT(f_list);
	}

	return(format);
}

/*****************************************************************************
 * NAME: fd_get_header
 *
 * PURPOSE: Get a header format data
 *
 * USAGE:  header_format_data = fd_get_header(dbin, header_type);
 *
 * RETURNS:  A pointer to a FORMAT_DATA with type FFF_HEADER | header_type
 *
 * DESCRIPTION:  Do not call this function until the dbin->array_conduit_list
 * has been created.
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

FORMAT_DATA_PTR fd_get_header
	(
	 DATA_BIN_PTR dbin,
	 FF_TYPES_t header_type
	)
{
	FORMAT_DATA_PTR hd_fd = NULL;
	
	hd_fd = fd_get_fd(dbin, FFF_HEADER, header_type);
	
	return(hd_fd);
}

/*****************************************************************************
 * NAME:
 *
 * PURPOSE: Get a data format data
 *
 * USAGE:  data_format_data = fd_get_data(dbin, io_type);
 *
 * RETURNS:  A pointer to a FORMAT_DATA with type FFF_DATA | io_type
 *
 * DESCRIPTION:  Do not call this function until the dbin->array_conduit_list
 * has been created.
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

FORMAT_DATA_PTR fd_get_data
	(
	 DATA_BIN_PTR dbin,
	 FF_TYPES_t data_type
	)
{
	FORMAT_DATA_PTR data_fd = NULL;
	
	data_fd = fd_get_fd(dbin, FFF_DATA, data_type);
	
	return(data_fd);
}

static int set_keys
	(
	 va_list *args,
	 FF_TYPES_t *search_key,
	 char **key_name
	)
{
	int error = 0;
	FF_TYPES_t attribute;

	attribute = (FF_TYPES_t)va_arg(*args, FF_TYPES_t);
	switch (attribute)
	{
		case FFF_GROUP: /* Argument FF_TYPES_t */
			*search_key = (FF_TYPES_t)va_arg (*args, FF_TYPES_t);
				
			assert(*search_key);
			if (!*search_key)
				error = err_push(ERR_PARAM_VALUE, "zero value search_key");
		break;
	
		case FFF_NAME_CASE:  /* Argument: char* */
			*key_name = va_arg (*args, char *);

			assert(*key_name);
			if (!*key_name)
				error = err_push(ERR_PARAM_VALUE, "NULL key_name");
		break;
	
		default:
			assert(!ERR_SWITCH_DEFAULT);
			error = err_push(ERR_SWITCH_DEFAULT, "%s, %s:%d", ROUTINE_NAME, os_path_return_name(__FILE__), __LINE__);
		break;
	}
	
	return(error);
}

static BOOLEAN test_keys
	(
	 FORMAT_PTR format,
	 FF_TYPES_t search_key,
	 char *key_name
	)
{
	FF_VALIDATE(format);
		
	if (search_key && key_name)
	{
		if ((search_key & format->type) == search_key &&
		    !strcmp(format->name, key_name))
			return(TRUE);
	}
	else if (search_key && ( (search_key & format->type) == search_key) )
		return(TRUE);
	else if (key_name && !strcmp(format->name, key_name))
		return(TRUE);
     
	return(FALSE);
}

/*
 * NAME: db_find_format
 *              
 * PURPOSE: To search a format list for a format whose attribute matches a key.
 *
 * USAGE: FORMAT_PTR format = db_find_format(FORMAT_LIST, attrib, key, ... NULL)
 *
 * RETURNS: A pointer to the format or NULL if no format is found.
 *
 * DESCRIPTION: db_find_format searches a format list looking for a format
 * with an attribute matching a key. The attributes are
 * specified in a list of arguments followed by the
 * key. The format type identifiers are given in freeform.h:
 *  
 * The attributes which db_find_format identifies are
 * FFF_GROUP and FFF_NAME
 *
 * form = db_find_format(list, FFF_GROUP, FFF_INPUT | FFF_HEADER)
 *
 * SYSTEM DEPENDENT FUNCTIONS:  
 *
 * AUTHOR: T. Habermann, NGDC, (303) 497 - 6472, haber@ngdc.noaa.gov
 *
 * COMMENTS:    
 *
 * KEYWORDS:    
 *
 */

#undef ROUTINE_NAME
#define ROUTINE_NAME "db_find_format"

FORMAT_PTR db_find_format(FORMAT_LIST f_list, ...)
{
	va_list args;
	FF_TYPES_t search_key = 0;
	char *key_name = NULL;
	FORMAT_PTR format = NULL;

	/* Error checking on NULL parameter */

	assert(f_list);

	va_start(args, f_list);

	if (set_keys(&args, &search_key, &key_name))
		return(NULL);

	/* Search the list */
	f_list = dll_first(f_list);
	format = FF_FORMAT(f_list);
	while (format)
	{
		if (test_keys(format, search_key, key_name))
			break;
			
		f_list = dll_next(f_list);
		format = FF_FORMAT(f_list);
	}

	return(format);
}

#if 0
/*
 * NAME: ap_find_format_data
 *              
 * PURPOSE: To search an array-conduit list for a format whose attribute matches a key.
 *
 * USAGE: FORMAT_DATA_PTR fd = ap_find_format_data(FORMAT_DATA_LIST, attrib, key, ... NULL)
 *
 * RETURNS: A pointer to the format-data or NULL if no format is found.
 *
 * DESCRIPTION: ap_find_format_data searches an array-conduit list looking for a format-data's
 * format with an attribute matching a key. The attributes are
 * specified in a list of arguments followed by the
 * key. The format type identifiers are given in freeform.h:
 *  
 * The attributes which db_find_format identifies are
 * FFF_GROUP and FFF_NAME
 *
 * form = ap_find_format_data(list, FFF_GROUP, FFF_INPUT | FFF_HEADER)
 *
 * SYSTEM DEPENDENT FUNCTIONS:  
 *
 * AUTHOR: T. Habermann, NGDC, (303) 497 - 6472, haber@ngdc.noaa.gov
 *
 * COMMENTS:    
 *
 * KEYWORDS:    
 *
 */

#undef ROUTINE_NAME
#define ROUTINE_NAME "ap_find_format_data"

FORMAT_DATA_PTR ap_find_format_data(FF_ARRAY_CONDUIT_LIST ac_list, ...)
{
	va_list args;
	FF_TYPES_t search_key = 0;
	char *key_name = NULL;
	FF_ARRAY_CONDUIT_PTR conduit = NULL;
	FORMAT_DATA_PTR format_data = NULL;

	/* Error checking on NULL parameter */

	assert(ac_list);

	va_start(args, ac_list);

	if (set_keys(&args, &search_key, &key_name))
		return(NULL);

	/* Search the list */
	ac_list = dll_first(ac_list);
	conduit = dll_data(ac_list);
	while (conduit)
	{
		format_data = AC_INFDATA(conduit);
		if (format_data)
		{
			if (test_keys(FD_FORMAT(format_data), search_key, key_name))
				break;
		}

		format_data = AC_OUTFDATA(conduit);
		if (format_data)
		{
			if (test_keys(FD_FORMAT(format_data), search_key, key_name))
				break;
		}

		format_data = NULL;
		ac_list = dll_next(ac_list);
		conduit = dll_data(ac_list);
	}

	return(format_data);
}
#endif /* 0 */

/*
 * NAME: fd_find_format_data
 *              
 * PURPOSE: To search a format-data list for a format whose attribute matches a key.
 *
 * USAGE: FORMAT_DATA_PTR fd = fd_find_format_data(FORMAT_DATA_LIST, attrib, key, ... NULL)
 *
 * RETURNS: A pointer to the format-data or NULL if no format is found.
 *
 * DESCRIPTION: fd_find_format_data searches a format-data list looking for a format-data's
 * format with an attribute matching a key. The attributes are
 * specified in a list of arguments followed by the
 * key. The format type identifiers are given in freeform.h:
 *  
 * The attributes which db_find_format identifies are
 * FFF_GROUP and FFF_NAME
 *
 * form = fd_find_format_data(list, FFF_GROUP, FFF_INPUT | FFF_HEADER)
 *
 * SYSTEM DEPENDENT FUNCTIONS:  
 *
 * AUTHOR: T. Habermann, NGDC, (303) 497 - 6472, haber@ngdc.noaa.gov
 *
 * COMMENTS:    
 *
 * KEYWORDS:    
 *
 */

#undef ROUTINE_NAME
#define ROUTINE_NAME "fd_find_format_data"
#define FDL_FORMAT(fdl)         ((FORMAT_PTR)(FD_FORMAT_DATA(fdl) ? FD_FORMAT_DATA(fdl)->format : NULL))

FORMAT_DATA_PTR fd_find_format_data(FORMAT_DATA_LIST fd_list, ...)
{
	va_list args;
	FF_TYPES_t search_key = 0;
	char *key_name = NULL;
	FORMAT_PTR format = NULL;

	/* Error checking on NULL parameter */

	if (!fd_list)
		return(NULL);

	va_start(args, fd_list);

	if (set_keys(&args, &search_key, &key_name))
		return(NULL);

	/* Search the list */
	fd_list = dll_first(fd_list);
	format = FDL_FORMAT(fd_list);
	while (format)
	{
		if (test_keys(format, search_key, key_name))
			break;

		fd_list = dll_next(fd_list);
		format = FDL_FORMAT(fd_list);
	}

	return(FD_FORMAT_DATA(fd_list));
}

/*
 * NAME:  db_format_list_mark_io
 *              
 * PURPOSE: To mark ambiguous formats in a format list as being either input
 * or output formats.
 *
 * USAGE: db_format_list_mark_io(FORMAT_LIST form_list, FF_TYPES_t io_type, char *input_file_name, char *output_file_name)
 *
 * RETURNS: void
 *
 * DESCRIPTION: Looks for and marks ambiguous data format descriptions to be
 * either input or output formats.  This means that format types in .fmt files
 * that were not given the input or output format descriptor (e.g.,
 * "ASCII_input_data") are defined by this function to be either input or
 * output according to the current context.  The extension of the data file
 * being processed is the sole determinant of this context.
 *
 * The rules for marking ambiguous formats based on data file extension
 * are detailed below:
 *
 * input_file_name ends in ".dat":
 * Mark all ambiguous ASCII formats as FFF_INPUT.
 *
 * input_file_name ends in ".dab":
 * Mark all ambiguous FLAT formats as FFF_INPUT.
 *
 * input_file_name ends in anything else:
 * Mark all ambiguous binary formats as FFF_INPUT.
 *
 * output_file_name ends in ".dat":
 * Mark all ambiguous ASCII formats as FFF_OUTPUT.
 *
 * output_file_name ends in ".dab":
 * Mark all ambiguous FLAT formats as FFF_OUTPUT.
 *
 * output_file_name ends in anything else:
 * Mark all ambiguous binary formats as FFF_OUTPUT.
 *
 * No output_file_name, input_file_name ends in ".dat":
 * Mark all ambiguous binary formats as FFF_OUTPUT.
 *
 * No output_file_name, input_file_name ends in anything else:
 * Mark all ambiguous ASCII formats as FFF_OUTPUT.
 *
 * SYSTEM DEPENDENT FUNCTIONS:  
 *
 * AUTHOR:  Mark Ohrenschall, NGDC, (303) 497-6124, mao@ngdc.noaa.gov
 *
 * COMMENTS: 
 *
 * KEYWORDS:  MAGIC WAND WAVING
 *
 */

#undef ROUTINE_NAME
#define ROUTINE_NAME "db_format_list_mark_io"

void db_format_list_mark_io
	(
	 FORMAT_LIST f_list,
	 FF_TYPES_t io_type,
	 char *input_file_name,
	 char *output_file_name
	)
{
	FORMAT_PTR format = NULL;

	FF_TYPES_t search_format_type = FFF_ASCII;
	
	char *in_point = input_file_name ? os_path_return_ext(input_file_name) : NULL;
	char *out_point = output_file_name ? os_path_return_ext(output_file_name) : NULL;

	if (!f_list)
		return;
	
	if (IS_INPUT_TYPE(io_type))
	{
		if (in_point && !osf_strcmp(in_point, "dat"))
			search_format_type = FFF_ASCII;
		else if (in_point && !osf_strcmp(in_point, "dab"))
			search_format_type = FFF_FLAT;
		else
			search_format_type = FFF_BINARY;
	}
	else if (IS_OUTPUT_TYPE(io_type))
	{
		if (out_point && !osf_strcmp(out_point, "dat"))
			search_format_type = FFF_ASCII;
		else if (out_point && !osf_strcmp(out_point, "dab"))
			search_format_type = FFF_FLAT;
		else if (output_file_name)
			search_format_type = FFF_BINARY;
		else
		{
			if (in_point && !osf_strcmp(in_point, "dat"))
				search_format_type = FFF_BINARY;
			else
				search_format_type = FFF_ASCII;
		}
	}

	/* Mark data formats */
	format = db_find_format_is_isnot(f_list,
	                                 FFF_GROUP,
	                                 FFF_DATA | search_format_type,
	                                 FFF_INPUT | FFF_OUTPUT
	                                );
	while (format)
	{
		FF_VALIDATE(format);
		
		format->type |= (io_type & FFF_IO);

		format = db_find_format_is_isnot(f_list,
		                                 FFF_GROUP,
		                                 FFF_DATA | search_format_type,
		                                 FFF_INPUT | FFF_OUTPUT
		                                );
	}
	
	/* Mark header formats */
	format = db_find_format_is_isnot(f_list,
	                                 FFF_GROUP,
	                                 FFF_HEADER | search_format_type,
	                                 FFF_INPUT | FFF_OUTPUT
	                                );
	while (format)
	{
		FF_VALIDATE(format);
		
		format->type |= (io_type & FFF_IO);

		format = db_find_format_is_isnot(f_list,
		                                 FFF_GROUP,
		                                 FFF_HEADER | search_format_type,
		                                 FFF_INPUT | FFF_OUTPUT
		                                );
	}
	
	return;
}

/*
 * NAME:	ff_find_variable
 *		
 * PURPOSE:	This function does a case sensitive search of a format for a
 *						variable with a given name.
 *  		
 * AUTHOR:	T. Habermann, NGDC, (303) 497 - 6472, haber@ngdc.noaa.gov
 * 		
 * USAGE:	ff_find_variable(char *name, FORMAT_PTR format)
 *			
 * ERRORS:	
 *		Pointer not defined,"Variable Name"
 *		Pointer not defined,"format"
 *		Possible memory corruption, format->name
 * 	
 * COMMENTS:
 * 	
 * RETURNS:	A pointer to the variable if it exists, NULL if it doesn't
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *  	   
 */ 

#ifdef ROUTINE_NAME
#undef ROUTINE_NAME
#endif
#define ROUTINE_NAME "ff_find_variable"

VARIABLE_PTR ff_find_variable(char *name, FORMAT_PTR format)
{
	VARIABLE_LIST v_list = NULL;

	/* Error checking on Pointers */
	assert(name);
	FF_VALIDATE(format);

	v_list = FFV_FIRST_VARIABLE(format);
	while (FF_VARIABLE(v_list))
	{
		if (memStrcmp(name, FF_VARIABLE(v_list)->name,NO_TAG) == 0)
			return(FF_VARIABLE(v_list));	/* name in list */

		v_list = dll_next(v_list);
	}
	
	return(NULL);		/* Not in list */
}

/*
 * NAME:	ff_lookup_string
 *		
 * PURPOSE: Lookup string for a given number in a FREEFORM lookup list
 *
 * AUTHOR:	T. Habermann, NGDC, (303) 497 - 6472, haber@mail.ngdc.noaa.gov
 *
 * USAGE:	char *ff_lookup_string(FFF_LOOKUP_PTR, FF_TYPES_t);
 *
 * RETURNS:	A pointer to the string, or NULL
 *
 * DESCRIPTION:	The FREEFORM lookup structure includes strings and numbers:
 * 				struct {
 *				char			*string;
 *				unsigned int	number;
 *				}FFF_LOOKUP, *FFF_LOOKUP_PTR;
 * 
 *				This function returns the string associated with the number
 *				given as the argument.
 *
 * AUTHOR:	T. Habermann, NGDC, (303) 497 - 6472, haber@ngdc.noaa.gov
 *
 * COMMENTS:	
 *
 * ERRORS:	
 *
 * SYSTEM DEPENDENT FUNCTIONS: 
 *
 * KEYWORDS:
 *
 */

char *ff_lookup_string(FFF_LOOKUP_PTR lookup, FF_TYPES_t int_key)
{
	while(lookup->string)
	{
		if (int_key == lookup->number)
			return(lookup->string);

		++lookup;
	}

	return(NULL);
}

/*
 * NAME:	ff_lookup_number
 *		
 * PURPOSE: Lookup number for a given string in a FREEFORM lookup list
 *
 * AUTHOR:	T. Habermann, NGDC, (303) 497 - 6472, haber@mail.ngdc.noaa.gov
 *
 * USAGE:	FF_TYPES_t ff_lookup_number(FFF_LOOKUP_PTR, char *);
 *
 * RETURNS:	An unsigned int  if the string is found or FF_VAR_TYPE_FLAG if not
 *
 * DESCRIPTION:	The FREEFORM lookup structure includes strings and numbers:
 * 				struct {
 *				char			*string;
 *				unsigned int	number;
 *				}FFF_LOOKUP, *FFF_LOOKUP_PTR;
 * 
 *				This function returns the number associated with the string
 *				given as the argument. The string comparisons are independent
 *				of case.
 *
 * AUTHOR:	T. Habermann, NGDC, (303) 497 - 6472, haber@ngdc.noaa.gov
 *
 * COMMENTS:	
 *
 * ERRORS:	
 *
 * SYSTEM DEPENDENT FUNCTIONS: 
 *
 * KEYWORDS:
 *
 */

FF_TYPES_t ff_lookup_number(FFF_LOOKUP_PTR lookup, char *str_key)
{
	while (lookup->string)
	{
		if(os_strcmpi(str_key, lookup->string) == 0)
			return(lookup->number);

		++lookup;
	}

	return(FF_VAR_TYPE_FLAG);
}

