/* FILENAME:  proclist.c
 *
 * CONTAINS:
 * Public functions:
 *
 * ff_create_format_data_mapping
 * ff_destroy_format_data_mapping
 * ff_process_format_data_mapping
 *
 * Private functions:
 *
 * bin2bin
 * cv_find_convert_var
 * initialize_middle_data
 * make_middle_format
 * lookup_convert_function
 * right_justify_conversion_output
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


#define WANT_NCSA_TYPES
#include <freeform.h>

const char *fft_cnv_flags[FFNT_ENOTE + 1] =
{
"%d",  /* int8 */
"%u",  /* uint8 */
"%hd", /* int16 */
"%hu", /* uint16 */
#ifdef LONGS_ARE_32
"%ld", /* int32 */
"%lu", /* uint32 */
"?",   /* int64 */
"?",   /* uint64 */
#elif defined(LONGS_ARE_64)
"%d",  /* int32 */
"%u",  /* uint32 */
"%ld", /* int64 */
"%lu", /* uint64 */
#endif
"%f",  /* float32 */
"%f",  /* float64 */
"%E"   /* E-notation */
};

const char *fft_cnv_flags_width[FFNT_ENOTE + 1] =
{
"%*d",  /* int8 */
"%*u",  /* uint8 */
"%*hd", /* int16 */
"%*hu", /* uint16 */
#ifdef LONGS_ARE_32
"%*ld", /* int32 */
"%*lu", /* uint32 */
"?",    /* int64 */
"?",    /* uint64 */
#elif defined(LONGS_ARE_64)
"%*d",  /* int32 */
"%*u",  /* uint32 */
"%*ld", /* int64 */
"%*lu", /* uint64 */
#endif
"%*f",  /* float32 */
"%*f",  /* float64 */
"%*E"   /* E-notation */
};

const char *fft_cnv_flags_prec[FFNT_ENOTE + 1] =
{
"%.*d",  /* int8 */
"%.*u",  /* uint8 */
"%.*hd", /* int16 */
"%.*hu", /* uint16 */
#ifdef LONGS_ARE_32
"%.*ld", /* int32 */
"%.*lu", /* uint32 */
"?",     /* int64 */
"?",     /* uint64 */
#elif defined(LONGS_ARE_64)
"%.*d",  /* int32 */
"%.*u",  /* uint32 */
"%.*ld", /* int64 */
"%.*lu", /* uint64 */
#endif
"%.*f",  /* float32 */
"%.*f",  /* float64 */
"%.*E"   /* E-notation */
};

const char *fft_cnv_flags_width_prec[FFNT_ENOTE + 1] =
{
"%*.*d",  /* int8 */
"%*.*u",  /* uint8 */
"%*.*hd", /* int16 */
"%*.*hu", /* uint16 */
#ifdef LONGS_ARE_32
"%*.*ld", /* int32 */
"%*.*lu", /* uint32 */
"?",      /* int64 */
"?",      /* uint64 */
#elif defined(LONGS_ARE_64)
"%*.*d",  /* int32 */
"%*.*u",  /* uint32 */
"%*.*ld", /* int64 */
"%*.*lu", /* uint64 */
#endif
"%*.*f",  /* float32 */
"%*.*f",  /* float64 */
"%*.*E",  /* E-notation */
};

/* REH: The NUMBER_OF_CONVERT_FUNCTIONS must be changed if
	convert functions are added or deleted!! */
#define NUMBER_OF_CONVERT_FUNCTIONS 105

typedef struct {
	char *name;
	FF_CVF *convert_func;
} CONVERT_VARIABLE;

static const CONVERT_VARIABLE convert_functions[NUMBER_OF_CONVERT_FUNCTIONS + 1] = 
{
	{NULL,                  (FF_CVF *)NULL},
	{"*",                   cv_units},
	{"longitude",           cv_deg},
	{"longitude",           cv_deg_nsew},
	{"longitude",           cv_lon_east},
	{"longitude",           cv_noaa_eq},
	{"longitude_deg",       cv_degabs_nsew},
	{"longitude_deg",       cv_dms},
	{"longitude_min",       cv_dms},
	{"longitude_sec",       cv_dms},
	{"longitude_abs",       cv_abs},

	{"longitude_abs",       cv_deg_abs},
	{"longitude_deg_abs",   cv_abs},
	{"longitude_deg_abs",   cv_degabs},
	{"longitude_min_abs",   cv_degabs},
	{"longitude_sec_abs",   cv_degabs},
	{"longitude_ew",        cv_nsew},
	{"longitude_ew",        cv_geog_sign},
	{"longitude_sign",      cv_abs},
	{"longitude_sign",      cv_geog_sign},
	{"longitude_east",      cv_lon_east},

	{"latitude",            cv_deg},
	{"latitude",            cv_deg_nsew},
	{"latitude",            cv_noaa_eq},
	{"latitude_abs",        cv_abs},
	{"latitude_abs",        cv_deg_abs},
	{"latitude_deg_abs",    cv_abs},
	{"latitude_deg_abs",    cv_degabs},
	{"latitude_min_abs",    cv_degabs},
	{"latitude_sec_abs",    cv_degabs},
	{"latitude_ns",         cv_geog_sign},

	{"latitude_ns",         cv_nsew},
	{"latitude_deg",        cv_degabs_nsew},
	{"latitude_deg",        cv_dms},
	{"latitude_min",        cv_dms},
	{"latitude_sec",        cv_dms},
	{"latitude_sign",       cv_abs},
	{"latitude_sign",       cv_geog_sign},
	{"mb",                  cv_long2mag},
	{"ms1",                 cv_long2mag},
	{"ms2",                 cv_long2mag},

	{"mb-maxlike",          cv_long2mag},
	{"magnitude_mb",        cv_long2mag},
	{"magnitude_mb",        cv_noaa_eq},
	{"magnitude_ms",        cv_long2mag},
	{"magnitude_ms1",       cv_long2mag},
	{"magnitude_ms2",       cv_long2mag},
	{"magnitude_ms",        cv_noaa_eq},
	{"magnitude_mo",        cv_noaa_eq},
	{"magnitude_ml",        cv_noaa_eq},
	{"magnitude_ml",        cv_long2mag},

	{"magnitude_local",     cv_long2mag},
	{"serial",              cv_ymd2ser},
	{"serial_day_1980",     cv_ymd2ser},
	{"serial",              cv_ipe2ser},
	{"serial_day_1980",     cv_ipe2ser},
	{"ipe_date",            cv_ymd2ipe},
	{"ipe_date",            cv_ser2ipe},
	{"century",             cv_ser2ymd},
	{"century",             cv_ydec2ymd},
	{"century_and_year",	  cv_ser2ymd},

	{"century_and_year",	  cv_ydec2ymd},
	{"year",                cv_ser2ymd},
	{"year",                cv_ydec2ymd},
	{"year",                cv_noaa_eq},
	{"year_decimal",  		cv_ymd2ser},
	{"month",               cv_ser2ymd},
	{"month",               cv_ydec2ymd},
	{"month",               cv_noaa_eq},
	{"day",                 cv_ser2ymd},
	{"day",                 cv_ydec2ymd},

	{"day",                 cv_noaa_eq},
	{"hour",                cv_ser2ymd},
	{"hour",                cv_ydec2ymd},
	{"hour",                cv_noaa_eq},
	{"minute",              cv_ser2ymd},
	{"minute",              cv_ydec2ymd},
	{"minute",              cv_noaa_eq},
	{"second",              cv_ser2ymd},
	{"second",              cv_ydec2ymd},
	{"second",              cv_noaa_eq},

	{"date_yymmdd",         cv_date_string},
	{"date_mm/dd/yy",       cv_date_string}, /* date_mm/dd/yy replaces date_m/d/y */
	{"date_dd/mm/yy",       cv_date_string},
	{"time_hhmmss",         cv_time_string},
	{"time_h:m:s",          cv_time_string},
	{"longmag",             cv_mag2long},
	{"cultural",            cv_sea_flags},
	{"cultural",            cv_noaa_eq},
	{"ngdc_flags",          cv_sea_flags},
	{"depth_control",       cv_sea_flags},

	{"depth",               cv_noaa_eq},
	{"WMO_quad_code",       cv_geog_quad},  
	{"geog_quad_code",      cv_geog_quad},
	{"time_offset",         cv_geo44tim},
	{"cultural",            cv_slu_flags},
	{"non_tectonic",        cv_slu_flags},
	{"magnitude_ml",        cv_slu_flags},
	{"scale",               cv_slu_flags},
	{"ml_authority",        cv_slu_flags},
	{"intensity",           cv_slu_flags},

	{"intensity",           cv_noaa_eq},
	{"fe_region",           cv_noaa_eq},
	{"no_station",          cv_noaa_eq},
	{"source_code",         cv_noaa_eq},
	{"zh_component",        cv_noaa_eq},
};

#if ff_dbg
static CONVERT_VARIABLE *lookup_convert_function(int cv_var_num)
{
	return((CONVERT_VARIABLE *)&convert_functions[cv_var_num]);
}
#else
#define lookup_convert_function(i) ((CONVERT_VARIABLE *)&convert_functions[i])
#endif

/*
 * NAME:	cv_find_convert_var
 *
 * PURPOSE:	This function checks the array of known conversion functions for one
 *			which makes a particular variable. If a conversion is possible,
 *			the variable is added to the input format with type FFV_CONVERT
 *			and the start position set to the index of the function to call.
 *
 * AUTHOR:	T. Habermann (303) 497-6472, haber@mail.ngdc.noaa.gov
 *
 * USAGE:	Used by Proclist (ff_process_format_data_mapping)
 *
 *			cv_find_convert_var(
 *				VARIABLE_LIST var,		Description of desired variable 
 *				FORMAT_PTR input_format,	The Input Format Description 	
 *				char *input_buffer)	The input buffer 			  
 *  													 
 * COMMENTS:
 *
 * RETURNS:	An error code on memory failure, zero otherwise, even if convert
 * function cannot be found.
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */

#ifdef ROUTINE_NAME
#undef ROUTINE_NAME
#endif
#define ROUTINE_NAME "cv_find_convert_var"

static int cv_find_convert_var
	(
	 VARIABLE_PTR var, 
	 FORMAT_DATA_PTR format_data,
	 int *conv_var_num
	)
{
	double double_value = 0;

	/* Loop Through the Possible Conversion Functions:
		var_look is a pointer to a CONVERT_VARIABLE, it contains the name
		of a variable and a pointer to a function which can construct that
		variable. convert_functions is an array of CONVERT_VARIABLES which
		is initialized in freeform.h. This array is searched here.
		
		If var_look->name == NULL, the end of the array has been reached. */

	/*	If var->name (the name of the variable we are trying to construct)
		matches convert_functions[i]->name, call the function to see if it can find
		the variables needed to make the conversion. If it can, it returns
		a 1, if it can't it returns 0.
			
		The last element of the convert_functions array has * as the
		variable name and cv_units as the convert function. This function
		handles the default unit conversions. */

	*conv_var_num = NUMBER_OF_CONVERT_FUNCTIONS;
	while (*conv_var_num)
	{
		if (!strcmp(var->name, convert_functions[*conv_var_num].name) ||
		    convert_functions[*conv_var_num].name[0] == '*'
		   )
		{
			if ((*(convert_functions[*conv_var_num].convert_func))(var,
			                                               &double_value,
			                                               format_data->format,
			                                               format_data->data->buffer
			                                              )
			   )
			{
				/* Successful Conversion Found */
				break;
			}
		}
		
		--(*conv_var_num);
	}

	return(0);
}

#if 0
/*
 * NAME:	ff_bin2bin
 *		
 * PURPOSE:	Converts binary representation between record fields
 *
 * USAGE:	int ff_bin2bin(	VARIABLE_PTR	in_var,
 *				char *input_buffer,
 *				VARIABLE_PTR	out_var,
 *				char *output_buffer)
 *
 * RETURNS:	0 if successful, error ID if not
 *
 * DESCRIPTION:	This function takes pointers to two data records and two
 * variables and converts from the input to the output.
 *
 * If the data type changes from integer to floating point then the floating
 * point value is divided by 10 raised to the power of the input precision,
 * otherwise if the data type changes from floating point to integer then the
 * integer value is multiplied by 10 raised to the power of the output
 * precision.
 *
 * AUTHOR:	T. Habermann, NGDC, (303) 497 - 6472, haber@ngdc.noaa.gov
 *
 * COMMENTS:	If the input variable is a character type, only copies to another
 *							character are supported.
 *
 * ERRORS:
 *		Problem in conversion,"out_var->type"
 *		Unknown variable type,"out_var->type"
 *		Unknown variable type,"Input Variable"
 *		Unknown variable type,"out_var->type"
 * 
 * SYSTEM DEPENDENT FUNCTIONS:	Alignment is taken care of using doubles.
 *
 * KEYWORDS:
 *
 */

#undef ROUTINE_NAME
#define ROUTINE_NAME "bin2bin"

static int bin2bin(VARIABLE_PTR in_var, char *input_buffer,
          VARIABLE_PTR out_var, char *output_buffer)
{
	int error;
	double dbl_var;

	/* Error checking on NULL parameters and corrupt memory */
	FF_VALIDATE(in_var);
	assert(input_buffer);
	FF_VALIDATE(out_var);
	assert(output_buffer);

	/* The only support presently available for FFV_TEXT variables is a
	copy from the input to the output */
	if (IS_TEXT(in_var))
	{ 
		if (IS_TEXT(out_var))
		{
			memcpy((void *)(output_buffer + max(1, out_var->start_pos) - 1),
			       (void *)(input_buffer + max(1, in_var->start_pos) - 1),
			       FF_VAR_LENGTH(in_var)
			      );
		}
		else
			return(err_push(ERR_CONVERT, "convert to var type"));
	}
	else
		if ((error = btype_to_btype(input_buffer + max(1, in_var->start_pos) - 1,
		 FFV_DATA_TYPE(in_var), output_buffer + max(1, out_var->start_pos) - 1,
		 FFV_DATA_TYPE(out_var))) != 0)
			return(err_push(error, "conversion to variable type"));
	
	/* Next check for precision conversion */
	if ((in_var->precision || out_var->precision) &&
	    ( (IS_INTEGER(in_var) && IS_REAL(out_var)) ||
	      (IS_REAL(in_var) && IS_INTEGER(out_var))) )
	{
		(void)btype_to_btype(output_buffer + max(1, out_var->start_pos) - 1,
		 FFV_DATA_TYPE(out_var), &dbl_var, FFV_DOUBLE);

		if (IS_INTEGER(in_var) && IS_REAL(out_var))
			dbl_var /= pow(10, in_var->precision);
		else
			dbl_var *= pow(10, out_var->precision);
		
		if ((error = btype_to_btype(&dbl_var, FFV_DOUBLE,
		 output_buffer + max(1, out_var->start_pos) - 1, FFV_DATA_TYPE(out_var))) != 0)
			return(err_push(error, "implicit precision conversion"));
	}
	
	return(0);
}

#endif /* 0 */

static void right_justify_conversion_output
	(
	 char *variable_str,
	 double *data_ptr,
	 size_t out_var_length
	)
{
	int i;
	size_t varstr_length;

#ifdef FF_DBG	
	char *cp;
#endif
	
	/* Seek past leading digits (and leading spaces, if any) */
	memset(variable_str, ' ', out_var_length);
	memcpy(variable_str, (void *)data_ptr, sizeof(double));
	variable_str[out_var_length] = STR_END;

	varstr_length = strlen(variable_str);
	i = varstr_length - 1;
	while (variable_str[i] == ' ' && i)
		--i;
	++i;

#ifdef FF_DBG	    
	for (cp = variable_str + strlen(variable_str) - 1;
	     *cp == ' ' && (char HUGE *)cp > (char HUGE *)variable_str;
	     cp--
	    )
	{
		;
	}
	cp++;
	assert(varstr_length - i == strlen(cp));
	assert(i == (char HUGE *)cp - (char HUGE *)variable_str);
#endif

	if (variable_str[i])
	{
		/* Flush leading digits (and any leading spaces) right */
		memmove(variable_str + (varstr_length - i),
		        variable_str,
		        i
		       );
		/* Fill vacated positions with spaces */
		memset(variable_str, ' ', (varstr_length - i));
	}
}

#undef ROUTINE_NAME
#define ROUTINE_NAME "ff_binary_to_string"

/*****************************************************************************
 * NAME:  ff_binary_to_string()
 *
 * PURPOSE:  Write a binary representation of a number as a string
 *
 * USAGE:  error = ff_binary_to_string(binary_data, data_type, text_string);
 *
 * RETURNS:	0 if all goes well, otherwise an error code defined in err.h
 *
 * DESCRIPTION:  Depending on data_type, binary_data is cast as a pointer to
 * the given type, and sprintf()'d into string.  If binary_data contains a
 * string (data_type is FFV_CHAR) then the string is copied into text_string.
 * Doubles and floats are converted using default precision.
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

int ff_binary_to_string
	(
	 void *binary_data,
	 FF_TYPES_t data_type,
	 int precision,
	 char *text_string
	)
{
	int error = 0;
	align_var_type align_var;

	assert(binary_data && text_string);
	
	if (!IS_TEXT_TYPE(data_type))
		memcpy((void *)&align_var, binary_data, ffv_type_size(data_type));

	switch (FFV_DATA_TYPE_TYPE(data_type))
	{
		case FFV_TEXT:
			strcpy(text_string, (char *)binary_data);
		break;
		
		case FFV_INT8:
			sprintf(text_string, fft_cnv_flags[FFNT_INT8], *(int8 *)&align_var);
		break;
		
		case FFV_UINT8:
			sprintf(text_string, fft_cnv_flags[FFNT_UINT8], *(uint8 *)&align_var);
		break;
		
		case FFV_INT16:
			sprintf(text_string, fft_cnv_flags[FFNT_INT16], *(int16 *)&align_var);
		break;
		
		case FFV_UINT16:
			sprintf(text_string, fft_cnv_flags[FFNT_UINT16], *(uint16 *)&align_var);
		break;
		
		case FFV_INT32:
			sprintf(text_string, fft_cnv_flags[FFNT_INT32], *(int32 *)&align_var);
		break;
		
		case FFV_UINT32:
			sprintf(text_string, fft_cnv_flags[FFNT_UINT32], *(uint32 *)&align_var);
		break;
		
		case FFV_INT64:
			sprintf(text_string, fft_cnv_flags[FFNT_INT64], *(int64 *)&align_var);
		break;
		
		case FFV_UINT64:
			sprintf(text_string, fft_cnv_flags[FFNT_UINT64], *(uint64 *)&align_var);
		break;
		
		case FFV_FLOAT32:
			sprintf(text_string, fft_cnv_flags_prec[FFNT_FLOAT32], precision, *(float32 *)&align_var);
		break;
		
		case FFV_FLOAT64:
			sprintf(text_string, fft_cnv_flags_prec[FFNT_FLOAT64], precision, *(float64 *)&align_var);
		break;
		
		case FFV_ENOTE:
			sprintf(text_string, fft_cnv_flags_prec[FFNT_ENOTE], precision, *(ff_enote *)&align_var);
		break;
		
		default:
			assert(!ERR_SWITCH_DEFAULT);
			error = err_push(ERR_SWITCH_DEFAULT, "%d, %s:%d", (int)FFV_DATA_TYPE_TYPE(data_type), os_path_return_name(__FILE__), __LINE__);
		break;
	}

	return(error);
}

/*****************************************************************************
 * NAME:  ff_put_binary_data
 *
 * PURPOSE:  Put a double-type value to any type value in ASCII or binary
 *
 * USAGE:
 *				error = ff_put_binary_data(o_work_space,
 *				                           FFV_DATA_TYPE(in_var),
 *				                           FF_VAR_LENGTH(out_var),
 *				                           output_ptr + out_var->start_pos - 1,
 *				                           FFV_DATA_TYPE(out_var),
 *				                           out_var->precision,
 *				                           output_format->type
 *				                          );
 *
 * RETURNS:  Zero on success, an error code on failure.
 *
 * DESCRIPTION:  In a sense this function is the opposite of ff_get_value
 * and ff_get_double.  It takes either a double or a C-string and converts
 * the double value to the given data type and format type.
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
#define ROUTINE_NAME "ff_put_binary_data"

int ff_put_binary_data
	(
	 VARIABLE_PTR var,
	 void *in_data_ptr,
	 size_t in_var_length,
	 FF_TYPES_t in_data_type,
	 void *out_data_ptr,
	 FF_TYPES_t out_format_type
	)
{
	BOOLEAN found_data_flag = FALSE;
	double dbl_var = DBL_MAX;
	char work_string[260]; /* must be large enough for any printf conversion */
	size_t byte_offset = 0;

	int error = 0;

	size_t out_var_length = FF_VAR_LENGTH(var);
	FF_TYPES_t out_data_type = FFV_DATA_TYPE(var);
	int out_var_prec = var->precision;
			                           
	FF_VALIDATE(var);
	
	if (IS_TEXT_TYPE(in_data_type))
	{
		size_t bytes_to_copy = 0;

		/* character type and numeric type are not compatible */
		if (!IS_TEXT_TYPE(out_data_type))
			return(err_push(ERR_CONVERT, "converting between text and numeric types"));
		
		bytes_to_copy = min(in_var_length, out_var_length);

		byte_offset = out_var_length > in_var_length ? out_var_length - in_var_length : 0;

		if (bytes_to_copy > 0)
			memcpy((char *)out_data_ptr + byte_offset, (char *)in_data_ptr, bytes_to_copy);

		if (!IS_TRANSLATOR(var) && !IS_CONVERT(var) && var->misc.mm)
			mm_set(var, MM_MAX_MIN, (char *)out_data_ptr + byte_offset, &found_data_flag);
	
		return(0);
	}
	else
		error = btype_to_btype(in_data_ptr, FFV_DOUBLE, &dbl_var, out_data_type);

	if (error)
		return(error);

	if (!IS_TRANSLATOR(var) && !IS_CONVERT(var) && var->misc.mm)
		mm_set(var, MM_MAX_MIN, (char *)&dbl_var, &found_data_flag);
	
	if (IS_ASCII_TYPE(out_format_type) || IS_FLAT_TYPE(out_format_type))
	{
		size_t bytes_to_copy;

		error = ff_binary_to_string(IS_FLOAT32_TYPE(out_data_type) ? in_data_ptr : &dbl_var, IS_FLOAT32_TYPE(out_data_type) ? FFV_DOUBLE : out_data_type, out_var_prec, work_string);
		if (error)
			return(error);

		/* Check size of output string */

		bytes_to_copy = strlen(work_string);
		
		assert(bytes_to_copy < sizeof(work_string)); /* might help a little */
		
		if (bytes_to_copy > (size_t)out_var_length)
		{
			memset(work_string, '*', out_var_length);
			bytes_to_copy = out_var_length;
		}
	
		/* Copy string to output buffer */
		memcpy((char *)out_data_ptr + (int)(out_var_length - bytes_to_copy),
		       work_string,
		       bytes_to_copy
		      );
	} /* if (IS_ASCII(out_format_type) || IS_FLAT(out_format_type)) */
	else if (IS_BINARY_TYPE(out_format_type))
	{
		memcpy(out_data_ptr,
		       (void *)&dbl_var,
		       out_var_length
		      );
	} /* else if (IS_BINARY(out_format_type)) */
	
	return(0);
}

#ifdef ROUTINE_NAME
#undef ROUTINE_NAME
#endif
#define ROUTINE_NAME "ff_varstring_to_double"

static int ff_varstring_to_double(char *varstring, double *dblvalue)
{
	char *endptr = NULL;
	int i;
	size_t ld_spc;
	
	/* ignore leading whitespace */
	ld_spc = strspn(varstring, LINESPACE);

#ifdef FF_DBG	
	{
		size_t ld_spc2;
		
		ld_spc2 = 0;
		while (isspace(varstring[ld_spc2]))
			++ld_spc2;

		assert(ld_spc == ld_spc2);
	}
#endif
  
  /* Is the field space-filled? */
	if (ld_spc == strlen(varstring))
	{
		*dblvalue = 0;
		return(0);
	}

	/* replace remaining spaces with zeros */
	for (i = strlen(varstring); i > (int)ld_spc; i--)
	{
		if (varstring[i - 1] == ' ')
			varstring[i - 1] = '0';
	}

	errno = 0;
	*dblvalue = strtod(varstring, &endptr);
	if (errno || ok_strlen(endptr))
	{
		return(err_push(errno == ERANGE ? errno : ERR_CONVERT, "Numeric conversion of \"%s\" stopped at \"%s\"", varstring, endptr));
	}
	else
		return(0);
}

/*
 * NAME:	ff_get_double
 *
 * PURPOSE:	This is the double version of ff_get_value.
 *			IT ADJUSTS THE PRECISION OF BINARY VARIABLES AS DOUBLES.
 *			This is the fastest technique.
 *		
 *			This is similar to ff_get_value, but the destination is cast
 *			a double.
 *			
 * AUTHOR:	Written by Ted Habermann, National Geophysical Data Center, Oct. 1990.
 *
 * USAGE:	ff_get_double(
 *				VARIABLE_PTR var, The variable description 
 *  			void *data_src, The address of the variable 
 *				double *dbl_dest, The destination for the variable 
 *				FF_TYPES_t format_type) The Format Type 
 *
 * ERRORS:
 *		Unknown variable type,"FLAT or ASCII var->type"
 *		Unknown variable type,"Binary var->type"
 *		Unknown format type, NULL
 *
 * COMMENTS:
 *
 * RETURNS:	0 if successful, an error code on failure
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */
 
#undef ROUTINE_NAME
#define ROUTINE_NAME "ff_get_double"

int ff_get_double
	(
	 VARIABLE_PTR var,	/* The variable description */
	 void *data_src,				/* The address of the variable */
	 double *dbl_dest,				/* The destination for the variable */
	 FF_TYPES_t format_type		/* The Format Type */
	)
{
	char scratch_buffer[256]; /* no ASCII number field could be larger! */
	int error;

	FF_VALIDATE(var);
	assert(data_src);
	assert(dbl_dest);

	format_type &= FFF_FILE_TYPES;
	switch (format_type)
	{
		case FFF_ASCII:
		case FFF_FLAT:
		
			if (IS_TEXT(var))
			{
				size_t bytes_to_copy = min(FF_VAR_LENGTH(var), sizeof(*dbl_dest) - 1);
				
				assert(FF_VAR_LENGTH(var) <= sizeof(*dbl_dest));

				memcpy((void *)dbl_dest, data_src, bytes_to_copy);
#if 0
				// I think this code is an error. dbl_dest is a double*, so
				// dbl_dest[i] will access the ith element in a double array.
				// Probably what's intended here is to tack a null byte onto
				// the end of the _bytes_ written into the 8-byte double
				// referenced by the pointer. But there's no need to do that
				// since this is a binary type. jhrg 1/9/12
				dbl_dest[bytes_to_copy] = STR_END;
#endif
			}
			else
			{
				int scratch_used;

				assert(FF_VAR_LENGTH(var) < sizeof(scratch_buffer));
				
				scratch_used = min(sizeof(scratch_buffer) - 1, FF_VAR_LENGTH(var));
				
				memcpy(scratch_buffer, data_src, scratch_used);
				scratch_buffer[scratch_used] = STR_END;
				
				error = ff_varstring_to_double(scratch_buffer, dbl_dest);
				if (error)
					return(err_push(error, "Problem with \"%s\"", var->name));
			}
			
			if (IS_INTEGER(var) && var->precision)
				*dbl_dest /= pow(10, var->precision);
	
		break; /* End of ASCII and FLAT cases */
			
		case FFF_BINARY:

			if (IS_TEXT(var))
			{
				FF_TYPES_t old_type = var->type;
				
				var->type = FFV_FLOAT64;
				
				error = ff_get_double(var, data_src, dbl_dest, FFF_ASCII);
				var->type = old_type;
				if (error)
					return(err_push(error, "Problem with \"%s\"", var->name));
			}
			else
			{
				error = btype_to_btype(data_src, FFV_DATA_TYPE(var), dbl_dest, FFV_DOUBLE);
				if (error)
					return(err_push(error, "Problem with \"%s\"", var->name));

				if (IS_INTEGER(var) && var->precision)
					*dbl_dest /= pow(10, var->precision);
			}
			
			break; 	/* end FFF_BINARY case */
				
		default:
			assert(!ERR_SWITCH_DEFAULT);
			return(err_push(ERR_SWITCH_DEFAULT, "%d, %s:%d", (int)format_type, os_path_return_name(__FILE__), __LINE__));
	}/* switch format_type */
					
	return(0);
}

int calculate_variable
	(
	 VARIABLE_PTR var, 
	 FORMAT_PTR format,
	 char *input_ptr,
	 double *d
	)
{
	int error = 0;

	FF_VALIDATE(var);
	FF_VALIDATE(format);

	if (ee_check_vars_exist(var->eqn_info, format))
		return(err_push(ERR_EE_VAR_NFOUND, "In format (%s)", format->name));

	if (ee_set_var_values(var->eqn_info, input_ptr, format))
		return(err_push(ERR_GEN_QUERY, "Seting equation variables in format (%s)", format->name));

	*d = ee_evaluate_equation(var->eqn_info, &error);

	return(error);
} 

/*
 * NAME:        ff_process_format_data_mapping() 
 *                      
 * PURPOSE:     Convert a buffer from one format to another.
 *
 * USAGE:       ff_process_format_data_mapping(
 *                      char   *input_buffer,
 *                      char   *output_buffer,
 *                      FORMAT *input_format,
 *                      FORMAT *output_format
 *                      long   *num_bytes)
 *                      
 * RETURNS: Zero on success, error code on failure
 *           
 * DESCRIPTION: This function takes input and output format specifications,
 * reads the input data buffer according to the input list
 * and writes the output buffer according to the output list.
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *              
 * AUTHOR:      T. Habermann, NGDC, (303) 497 - 6472, haber@mail.ngdc.noaa.gov
 * 
 * ERRORS:
 *              Pointer not defined,((!input_format->variables) ? "Input_format->variable
 *              Record Length or CR Problem, "In Input Data Format"
 *              Record Length or CR Problem, variable_str
 *              Out of memory, "Output Initialization String"
 *              System File Error,"Initialization String"
 *              System File Error,"initial_file"
 *              Unknown variable type,"binary var->type"
 *              Problem in conversion,variable_str
 *              Problem in conversion,(strcat("bin2bin: ",out_var->name))
 *
 * COMMENTS:    
 * This function now processes a certain number of bytes until done
 * EOL's are skipped while processing the input and added to output
 * It is assumed:
 * input_buffer begins with the start of a format
 * num_bytes in the cache has been calculated correctly
 * the pointers to input and output buffers are set correctly
 *      
 * A negative num_bytes indicates that processing is to start at end
 * of input_buffer. This use to be handled in CONVERT_CACHE so that
 * the cache can process onto itself. THIS NEEDS TESTING.
 *
 * KEYWORDS:
 *      
 */

#ifdef ROUTINE_NAME
#undef ROUTINE_NAME
#endif
#define ROUTINE_NAME "ff_process_format_data_mapping"

int ff_process_format_data_mapping
	(
	 FORMAT_DATA_MAPPING_PTR format_data_map
	)
{
	VARIABLE_PTR mid_var = NULL;
	VARIABLE_PTR out_var = NULL;
	
	VARIABLE_LIST mid_v_list = NULL;
	VARIABLE_LIST out_v_list = NULL;

	CONVERT_VARIABLE *conv_func = NULL;

	size_t input_increment  = 0;
	size_t output_increment = 0;

	double  double_value = 0;
	double  cv_double    = 0; /* convert variable, double will hold 8 char's */

	void *data_ptr = NULL; /* for the conversion/input_buffer */
	char *input_ptr  = NULL; /* increments along input_buffer */
	char *output_ptr = NULL; /* increments along output_buffer */
	
	void *o_work_space = NULL;
	
	long bytes_left = 0;

	int error;
	
	FF_VALIDATE(format_data_map);

	FF_VALIDATE(format_data_map->input);
	FF_VALIDATE(format_data_map->input->format);
	
	FF_VALIDATE(format_data_map->middle);
	FF_VALIDATE(format_data_map->middle->format);
	
	FF_VALIDATE(format_data_map->output);
	FF_VALIDATE(format_data_map->output->format);

	assert(format_data_map->input->data->bytes_used < FF_MAX_CACHE_SIZE);
	
	bytes_left = format_data_map->input->data->bytes_used;

	/* EOL's will be added to output and skipped in input */
	output_increment = FORMAT_LENGTH(format_data_map->output->format);
	input_increment  = (size_t)format_data_map->input->format->length;

	input_ptr  = format_data_map->input->data->buffer;
	output_ptr = format_data_map->output->data->buffer;
	format_data_map->output->data->bytes_used = 0;

	/********** MAIN LOOP ON INPUT DATA  - process bytes_left ***************/
	while (bytes_left > 0)
	{
		/* Initialize variables for this loop */
		mid_v_list = FFV_FIRST_VARIABLE(format_data_map->middle->format);
		out_v_list = FFV_FIRST_VARIABLE(format_data_map->output->format);
	
		/* initialize the record */
		assert(output_increment < format_data_map->output->data->total_bytes - format_data_map->output->data->bytes_used);
		memcpy(output_ptr, format_data_map->middle->data->buffer, output_increment);
		format_data_map->output->data->bytes_used += output_increment;
	
		/***** MAIN LOOP ON OUTPUT VARIABLE LIST *****/
		mid_var = FF_VARIABLE(mid_v_list);
		out_var = FF_VARIABLE(out_v_list);
		while (out_var)
		{    
			char variable_str[256];

			/* Check for possible corrupt memory */
			FF_VALIDATE(out_var);
			FF_VALIDATE(mid_var);

			if (IS_CONSTANT(out_var) || IS_INITIAL(out_var) || IS_ORPHAN_VAR(mid_var))
			{
				mid_v_list = dll_next(mid_v_list);
				out_v_list = dll_next(out_v_list);
				mid_var = FF_VARIABLE(mid_v_list);
				out_var = FF_VARIABLE(out_v_list);
				continue;
			}

			/* Convert variables are processed by creating a double in double_value
			and setting mid_var to the generic convert variable. */
			if (IS_CONVERT(mid_var))
			{
				conv_func = lookup_convert_function(mid_var->misc.cv_var_num);
				(*(conv_func->convert_func))(out_var, &cv_double, format_data_map->input->format, input_ptr);
	
				data_ptr = &cv_double;
			} /* if convert variable */
			else
				data_ptr = input_ptr + max(1, mid_var->start_pos) - 1;

			if (IS_TEXT(mid_var))
			{
				if (IS_CONVERT(mid_var))
				{
					assert(FF_VAR_LENGTH(out_var) < sizeof(variable_str));
					
					right_justify_conversion_output(variable_str, data_ptr, FF_VAR_LENGTH(out_var));
					o_work_space = &variable_str;
				}
				else
					o_work_space = data_ptr;
				
			} /* if is text variable */
			else
			{
				if (IS_EQN(out_var))
				{
					error = calculate_variable(out_var, format_data_map->input->format, input_ptr, &double_value);
					if (error)
						return(error);
				}
				else
				{
					/* Get a double with the value of the input variable */
					error = ff_get_double(mid_var,
					                      data_ptr,
					                      &double_value,
					                      IS_CONVERT(mid_var) ? FFF_BINARY
					                                          : format_data_map->input->format->type
					                     );
					if (error)
						return(err_push(error, "Problem with \"%s\"", mid_var->name));
				}

				/* adjust the value to the output precision */
				if (IS_INTEGER(out_var))
				{
					if (out_var->precision)
						double_value *= pow(10, out_var->precision);

					double_value = (double)ROUND(double_value);
				}
				
				o_work_space = &double_value;
			} /* (else) if is text variable */

			error = ff_put_binary_data(out_var,
			                           o_work_space,
			                           IS_CONVERT(mid_var) ? FF_VAR_LENGTH(out_var) : FF_VAR_LENGTH(mid_var),
									   IS_TEXT(mid_var) ? FFV_TEXT : FFV_DOUBLE,
			                           output_ptr + max(1, out_var->start_pos) - 1,
			                           format_data_map->output->format->type
			                          );
			if (error)
				return(err_push(error, "Problem with \"%s\"", out_var->name));

			mid_v_list = dll_next(mid_v_list);
			out_v_list = dll_next(out_v_list);

			mid_var = FF_VARIABLE(mid_v_list);
			out_var = FF_VARIABLE(out_v_list);
		} /***** End of Main loop on variable list *****/

		/* increment pointers and decrement bytes_left for more processing */
	
		output_ptr += output_increment;
		input_ptr  += input_increment;
		bytes_left -= input_increment;
	}/********** End Main loop processing bytes_left of data **********/

	/* Calculate how many bytes have been processed and assign to bytes,
	return the number of bytes not processed
	*/

	assert(format_data_map->output->data->bytes_used == (FF_BSS_t)labs(((char HUGE *)output_ptr -
	                                               (char HUGE *)format_data_map->output->data->buffer
	                                              )
	                                             ));
	if (bytes_left)
		return(err_push(ERR_CONVERT, "%d bytes not processed", (int)bytes_left));
	else
	{
		format_data_map->output->state.new_record = 1;
		return(0);
	}
}

/*
 * NAME:	btype_to_btype
 *              
 * PURPOSE:	Convert data from one binary representation to another, handling
 * alignment
 *
 * USAGE:	int btype_to_btype(void *src_value, unsigned int src_type, void *dest_value, unsigned int dest_type)
 *
 * RETURNS:	On success, return zero, and dest_value is set. Otherwise, return
 * an error value defined in err.h
 *
 * DESCRIPTION: If types are the same, then src_value is memcpy()'d into
 * dest_value, and if both are FFV_CHAR then src_value is strcpy()'d; src_value
 * must be NULL-terminated if it is a string.  If neither type is FFV_CHAR
 * src_value is memcpy()'d into a local double (for alignment) and then
 * dereferenced according to src_type and converted into a double which is
 * ranged checked according to dest_type.  Next, the double is converted into
 * another local double which is cast and dereferenced according to dest_type
 * (again for alignment) and then is memcpy()'d into dest_value.
 * 
 * SYSTEM DEPENDENT FUNCTIONS:  none
 *
 * GLOBALS:     none
 *
 * AUTHOR:  Mark Ohrenschall, NGDC, (303) 497-6124, mao@ngdc.noaa.gov
 *
 * COMMENTS:    
 *
 * KEYWORDS:    
 *
 */

#undef ROUTINE_NAME
#define ROUTINE_NAME "btype_to_btype"

int btype_to_btype(void *src_value, FF_TYPES_t src_type, void *dest_value, FF_TYPES_t dest_type)
{
	big_var_type   big_var = 0;
	align_var_type align_var = 0;

	int error = 0;
	
	size_t  src_type_size;
	size_t dest_type_size;
		
	src_type  = FFV_DATA_TYPE_TYPE(src_type);
	dest_type = FFV_DATA_TYPE_TYPE(dest_type);
	
	src_type_size  = ffv_type_size(src_type);
	dest_type_size = ffv_type_size(dest_type);
	
	if (src_type_size == 0 || dest_type_size == 0)
		return(ERR_UNKNOWN_VAR_TYPE);
	
	/* if the data types are the same, just do memory copy */
	if (src_type == dest_type)
	{
		switch (FFV_DATA_TYPE_TYPE(src_type))
		{
			case FFV_TEXT:
				strcpy((char *)dest_value, (char *)src_value);
			break;
			
			default:
					memcpy(dest_value, src_value, src_type_size);
			break;
		}
		
		return(0);
	}
	else if (IS_TEXT_TYPE(src_type) || IS_TEXT_TYPE(dest_type))
	{
		/* character type and numeric type are not compatible */
		return(err_push(ERR_CONVERT, "converting between text and numeric types"));
	}

	/* treat src_value alignment and convert to double */
	assert(sizeof(align_var) >= src_type_size); /* Added jhrg */
	memcpy((void *)&align_var, src_value, src_type_size);
	
	switch (FFV_DATA_TYPE_TYPE(src_type))
	{
		case FFV_INT8:
			big_var = *((int8 *)&align_var);
		break;
	
		case FFV_UINT8:
			big_var = *((uint8 *)&align_var);
		break;
	
		case FFV_INT16:
			big_var = *((int16 *)&align_var);
		break;
	
		case FFV_UINT16:
			big_var = *((uint16 *)&align_var);
		break;
	
		case FFV_INT32:
			big_var = *((int32 *)&align_var);
		break;
	
		case FFV_UINT32:
			big_var = *((uint32 *)&align_var);
		break;
	
		case FFV_INT64:
			big_var = *((int64 *)&align_var);
		break;
	
		case FFV_UINT64:
			big_var = *((uint64 *)&align_var);
		break;
	
		case FFV_FLOAT32:
			big_var = *((float32 *)&align_var);
		break;
	
		case FFV_FLOAT64:
			big_var = *((float64 *)&align_var);
		break;
	
		case FFV_ENOTE:
			big_var = *((ff_enote *)&align_var);
		break;
	
		default:
			assert(!ERR_SWITCH_DEFAULT);
			error = err_push(ERR_SWITCH_DEFAULT, "%d, %s:%d", (int)src_type, os_path_return_name(__FILE__), __LINE__);
		break;
	} 

	/* convert to dest_type and treat dest_value alignment */ 
	if (!error)
	{
		switch (FFV_DATA_TYPE_TYPE(dest_type))
		{
			case FFV_INT8:
				if (big_var < FFV_INT8_MIN || big_var > FFV_INT8_MAX)
				{
					error = ERR_OUT_OF_RANGE;

					*(int8 *)&align_var = (int8)(big_var < 0 ? FFV_INT8_MIN : FFV_INT8_MAX);
				}
				else
					*(int8 *)&align_var = (int8)ROUND(big_var);
			break;
			
			case FFV_UINT8:
				if (big_var < FFV_UINT8_MIN || big_var > FFV_UINT8_MAX)
				{
					error = ERR_OUT_OF_RANGE;

					*(uint8 *)&align_var = (uint8)(big_var < 0 ? FFV_UINT8_MIN : FFV_UINT8_MAX);
				}
				else
					*(uint8 *)&align_var = (uint8)ROUND(big_var);
			break;
			
			case FFV_INT16:
				if (big_var < FFV_INT16_MIN || big_var > FFV_INT16_MAX)
				{
					error = ERR_OUT_OF_RANGE;

					*(int16 *)&align_var = (int16)(big_var < 0 ? FFV_INT16_MIN : FFV_INT16_MAX);
				}
				else
					*(int16 *)&align_var = (int16)ROUND(big_var);
			break;
			
			case FFV_UINT16:
				if (big_var < FFV_UINT16_MIN || big_var > FFV_UINT16_MAX)
				{
					error = ERR_OUT_OF_RANGE;

					*(uint16 *)&align_var = (uint16)(big_var < 0 ? FFV_UINT16_MIN : FFV_UINT16_MAX);
				}
				else
					*(uint16 *)&align_var = (uint16)ROUND(big_var);
			break;
			
			case FFV_INT32:
				if (big_var < FFV_INT32_MIN || big_var > FFV_INT32_MAX)
				{
					error = ERR_OUT_OF_RANGE;

					*(int32 *)&align_var = big_var < 0 ? FFV_INT32_MIN : FFV_INT32_MAX;
				}
				else
					*(int32 *)&align_var = (int32)ROUND(big_var);
			break;
			
			case FFV_UINT32:
				if (big_var < FFV_UINT32_MIN || big_var > FFV_UINT32_MAX)
				{
					error = ERR_OUT_OF_RANGE;

					*(uint32 *)&align_var = big_var < 0 ? FFV_UINT32_MIN : FFV_UINT32_MAX;
				}
				else
					*(uint32 *)&align_var = (uint32)ROUND(big_var);
			break;
			
			case FFV_INT64:
				if (big_var < FFV_INT64_MIN || big_var > FFV_INT64_MAX)
				{
					error = ERR_OUT_OF_RANGE;

					*(int64 *)&align_var = (int64)(big_var < 0 ? FFV_INT64_MIN : FFV_INT64_MAX);
				}
				else
					*(int64 *)&align_var = (int64)ROUND(big_var);
			break;
			
			case FFV_UINT64:
				if (big_var < FFV_UINT64_MIN || big_var > FFV_UINT64_MAX)
				{
					error = ERR_OUT_OF_RANGE;

					*(uint64 *)&align_var = (uint64)(big_var < 0 ? FFV_UINT64_MIN : FFV_UINT64_MAX);
				}
				else
					*(uint64 *)&align_var = (uint64)ROUND(big_var);
			break;
			
			case FFV_FLOAT32:         
				if (big_var > FFV_FLOAT32_MAX || big_var < FFV_FLOAT32_MIN)
				{
					error = ERR_OUT_OF_RANGE;

					*(float32 *)&align_var = big_var < 0 ? FFV_FLOAT32_MIN : FFV_FLOAT32_MAX;
				}
				else
					*(float32 *)&align_var = (float32)big_var;
			break;
		
			case FFV_FLOAT64:
				if (big_var > FFV_FLOAT64_MAX || big_var < FFV_FLOAT64_MIN)
				{
					error = ERR_OUT_OF_RANGE;

					*(float64 *)&align_var = big_var < 0 ? FFV_FLOAT64_MIN : FFV_FLOAT64_MAX;
				}
				else
					*(float64 *)&align_var = (float64)big_var;
			break;
		
			case FFV_ENOTE:
				if (big_var > FFV_ENOTE_MAX || big_var < FFV_ENOTE_MIN)
				{
					error = ERR_OUT_OF_RANGE;

					*(ff_enote *)&align_var = big_var < 0 ? FFV_ENOTE_MIN : FFV_ENOTE_MAX;
				}
				else
					*(ff_enote *)&align_var = (ff_enote)big_var;
			break;
		
			default:
				assert(!ERR_SWITCH_DEFAULT);
				error = err_push(ERR_SWITCH_DEFAULT, "%d, %s:%d", (int)dest_type, os_path_return_name(__FILE__), __LINE__);
			break;
		} /* switch (FFV_DATA_TYPE_TYPE(dest_type)) */
	} /* if not error */

	memcpy(dest_value, (void *)&align_var, dest_type_size);

	if (error)
		return(err_push(error, "Type %s cannot hold %f", ff_lookup_string(variable_types, FFV_DATA_TYPE_TYPE(dest_type)), big_var));
	else
		return(0);
}

BOOLEAN type_cmp(FF_TYPES_t type, void *value0, void *value1)
/*****************************************************************************
 * NAME:  type_cmp()
 *
 * PURPOSE:  Compare two variables depending on FreeForm type
 *
 * USAGE:  is_equal = type_cmp(data_type, (void *)&var1, (void *)&var2);
 *
 * RETURNS:  1 if variables are equal, zero if unequal, or unknown data_type
 *
 * DESCRIPTION:  Strings are strcmp()'d, integers are memcmp()'d and floats
 * and doubles are differenced and compared to FLT/DBL_EPSILON, after attending
 * to alignment.
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

{
	size_t type_size = ffv_type_size(FFV_DATA_TYPE_TYPE(type));
	BOOLEAN cmp_result = FALSE;

	switch (FFV_DATA_TYPE_TYPE(type))
	{
		big_var_type big_var0;
		big_var_type big_var1;
		
		case FFV_TEXT:
			cmp_result = (BOOLEAN)!strcmp((char *)value0, (char *)value1);
		break;
					
		case FFV_INT8:
		case FFV_UINT8:
		case FFV_INT16:
		case FFV_UINT16:
		case FFV_INT32:
		case FFV_UINT32:
		case FFV_INT64:
		case FFV_UINT64:
			cmp_result = (BOOLEAN)!memcmp(value0, value1, type_size);
		break;
		
		case FFV_FLOAT32:
			memcpy((void *)&big_var0, value0, type_size);
			memcpy((void *)&big_var1, value1, type_size);
			cmp_result = (BOOLEAN)(fabs(*(float32 *)&big_var0 - *(float32 *)&big_var1) < FFV_FLOAT32_EPSILON);
		break;
	
		case FFV_FLOAT64:
			memcpy((void *)&big_var0, value0, type_size);
			memcpy((void *)&big_var1, value1, type_size);
			cmp_result = (BOOLEAN)(fabs(*(float64 *)&big_var0 - *(float64 *)&big_var1) < FFV_FLOAT64_EPSILON);
		break;
				
		case FFV_ENOTE:
			memcpy((void *)&big_var0, value0, type_size);
			memcpy((void *)&big_var1, value1, type_size);
			cmp_result = (BOOLEAN)(fabs(*(ff_enote *)&big_var0 - *(ff_enote *)&big_var1) < FFV_ENOTE_EPSILON);
		break;
				
		default:
			assert(!ERR_SWITCH_DEFAULT);
			err_push(ERR_SWITCH_DEFAULT, "%d, %s:%d", (int)type, os_path_return_name(__FILE__), __LINE__);
			cmp_result = FALSE;
		break;
	}

	return(cmp_result);
}

/*
 * NAME:        make_middle_format
 *              
 * PURPOSE:     To create a temporary format which matches an output format
 *
 * USAGE:       FORMAT_PTR make_middle_format(FORMAT_PTR input_format,
 *                      FORMAT_PTR output_format,
 *                      char *input_buffer)
 *
 * RETURNS:     Pointer to new format or NULL on error
 *
 * DESCRIPTION:
 * make_middle_format loops through the variables in the output
 * format and creates a new format with each of the variables
 * in it. If the variables exist in the input format, their
 * descriptions are simply copied to the middle format. If
 * the variables do not exist in the input format, but can be
 * created with a conversion function, a FFV_CONVERT variable
 * is set up. If the output variables are FFV_CONSTANTS's,
 * or FFV_INITIAL's they are copied to the middle format.
 *
 * SYSTEM DEPENDENT FUNCTIONS:  
 *
 * AUTHOR:      T. Habermann, NGDC, (303) 497 - 6472, haber@ngdc.noaa.gov
 *
 * ERRORS:
 *
 * COMMENTS:    
 *
 * KEYWORDS:    
 *
 */

#undef ROUTINE_NAME
#define ROUTINE_NAME "make_middle_format"

static int make_middle_format
	(
	 FORMAT_DATA_PTR input,
	 FORMAT_PTR output_format,
	 FORMAT_PTR middle_format
	)
{
	int cv_var_num = -1;
	int error = 0;
	int error_return = 0;
	
	VARIABLE_PTR      in_var     = NULL;
	VARIABLE_PTR      out_var    = NULL;
	VARIABLE_PTR      mid_var    = NULL;
	VARIABLE_LIST out_v_list = NULL;
	VARIABLE_LIST mid_v_list = NULL;

	char *copy_buffer = NULL;

	FF_VALIDATE(middle_format);
	FF_VALIDATE(output_format);
	if (input)
	{
		FF_VALIDATE(input);
		FF_VALIDATE(input->format);
	}

	middle_format->variables = dll_init();
	if (!middle_format->variables)
		return(err_push(ERR_MEM_LACK,"Temporary Format Variables"));

	if (input)
	{
		copy_buffer = (char *)memMalloc((size_t)input->format->length, "copy_buffer");
		if (!copy_buffer)
			return(err_push(ERR_MEM_LACK,"copy buffer"));
	}
	else
		copy_buffer = NULL;

	/* variable string is a character buffer used as temporary string
	manipulation area. It must be as long as the longer of the input record
	and the output record plus 1 byte for a null character added by sprintf
	when writing ASCII formats. */

	out_v_list = FFV_FIRST_VARIABLE(output_format);
	out_var = FF_VARIABLE(out_v_list);
	while (out_var)
	{ /* MAIN LOOP ON OUTPUT VARIABLE LIST */
		FF_VALIDATE(out_var);

		mid_var = ff_create_variable("");
		if (!mid_var)
		{
			if (copy_buffer)
				memFree(copy_buffer, "copy_buffer");

			return(err_push(ERR_MEM_LACK,"temporary variable"));
		}

		if (input)
		{
			in_var = ff_find_variable(out_var->name, input->format);
			if (!in_var)
			{
				/* Might be a calculated input variable */

				char try_name[MAX_PV_LENGTH];
				
				/* Added first assert to prevent int overflow. jhrg */
				assert(strlen(out_var->name) < UINT_MAX - strlen("_eqn"));
				assert(strlen(out_var->name) + strlen("_eqn") < sizeof(try_name) - 1);

				strncpy(try_name, out_var->name, sizeof(try_name) - 1);
				try_name[sizeof(try_name) - 1] = '\0'; // Added jhrg 3/18/11
				strncat(try_name, "_eqn", sizeof(try_name) - strlen(try_name) - 1);
				try_name[sizeof(try_name) - 1] = '\0'; // jhrg 3/18/11
				in_var = ff_find_variable(try_name, input->format);
			}
		}
		else
			in_var = NULL;

		if (in_var)
		{
			FF_VALIDATE(in_var);
	
			error = ff_copy_variable(in_var, mid_var);
			if (error)
			{
				ff_destroy_variable(mid_var);

				if (copy_buffer)
					memFree(copy_buffer, "copy_buffer");

				return(error);
			}
		}
		else if (input && input->format->length <= input->data->bytes_used)
		{
			memMemcpy(copy_buffer, input->data->buffer, (size_t)input->format->length, "copy_buffer");
			error = cv_find_convert_var(out_var, input, &cv_var_num);
			memcpy(input->data->buffer, copy_buffer, (size_t)input->format->length);
				
			if (error)
			{
				ff_destroy_variable(mid_var);

				if (copy_buffer)
					memFree(copy_buffer, "copy_buffer");

				return(error);
			}
			
			if (cv_var_num != 0)
			{
				mid_var->misc.cv_var_num = cv_var_num;
	
				error = new_name_string__(out_var->name, &mid_var->name);
				if (error)
				{
					ff_destroy_variable(mid_var);

					if (copy_buffer)
						memFree(copy_buffer, "copy_buffer");

					return(error);
				}
	
				mid_var->start_pos = 1;
				mid_var->end_pos = (FF_NDX_t)ffv_type_size(FFV_FLOAT64);
	
				/* We must deal with the special case of a character type conversion
				variable which is buried in a double value. This can happen when one
				is doing a conversion to a character type variable. */
				if (IS_TEXT(out_var))
				{
					mid_var->type = FFV_CONVERT | FFV_TEXT;
					mid_var->precision = 0;
				}
				else
				{
					mid_var->type = FFV_CONVERT | FFV_FLOAT64;
					mid_var->precision = out_var->precision;
				}
			}
			else
			{
				error = ff_copy_variable(out_var, mid_var);
				if (error)
				{
					ff_destroy_variable(mid_var);

					if (copy_buffer)
						memFree(copy_buffer, "copy_buffer");

					return(error);
				}

				if (!IS_INITIAL(out_var) && !IS_CONSTANT(out_var) && !IS_EQN(out_var))
				{
					mid_var->type |= FFV_ORPHAN;
					error_return = err_push(ERR_ORPHAN_VAR + ERR_WARNING_ONLY, out_var->name);
				}
			}
		} /* else if input->format->max_length <= input->data->bytes_used (if in_var) */
		else /* figure out a better way to do this */
		{
			error = ff_copy_variable(out_var, mid_var);
			if (error)
			{
				ff_destroy_variable(mid_var);

				if (copy_buffer)
					memFree(copy_buffer, "copy_buffer");

				return(error);
			}

			if (!IS_INITIAL(out_var) && !IS_CONSTANT(out_var))
			{
				mid_var->type |= FFV_ORPHAN;
				error_return = err_push(ERR_ORPHAN_VAR + ERR_WARNING_ONLY, out_var->name);
			}
		}

		mid_v_list = dll_add(middle_format->variables);
		if (!mid_v_list)
		{
			ff_destroy_variable(mid_var);

			if (copy_buffer)
				memFree(copy_buffer, "copy_buffer");

			return(err_push(ERR_MEM_LACK,"temporary variable"));
		}

		dll_assign(mid_var, DLL_VAR, mid_v_list);

		++middle_format->num_vars;

		out_v_list = dll_next(out_v_list);
		out_var = FF_VARIABLE(out_v_list);
	} /* while out_var */


	if (copy_buffer)
		memFree(copy_buffer, "copy_buffer");

	assert(output_format->num_vars == middle_format->num_vars);

	return(error_return);
}

#ifdef ROUTINE_NAME
#undef ROUTINE_NAME
#endif
#define ROUTINE_NAME "initialize_middle_data"

int initialize_middle_data
	(
	 FORMAT_DATA_PTR input,
	 FORMAT_DATA_PTR output,
	 FORMAT_DATA_PTR middle
	)
{
	VARIABLE_LIST out_v_list = NULL;
	VARIABLE_PTR out_var = NULL;
	
	FILE *initial_file = NULL;
	
	int error = 0;
	int error_return = 0;

	/* This format has the variables needed for the output format and the
	   convert variables added. It is created to avoid searching the input
		 format for each variable. */

	error = make_middle_format(input, output->format, middle->format);
	if (error && error < ERR_WARNING_ONLY)
		return(error);
	else if (error)
		error_return = error;

	assert(FORMAT_LENGTH(output->format) <= middle->data->total_bytes);
	if (IS_BINARY(output->format))
		memMemset(middle->data->buffer, STR_END, FORMAT_LENGTH(output->format),"middle->data->buffer, STR_END, FORMAT_LENGTH(output->format)" );
	else
		memMemset(middle->data->buffer, ' ', FORMAT_LENGTH(output->format),"middle->data->buffer, ' ', FORMAT_LENGTH(output->format)");
	
	/* If an FFV_INITIAL type variable exists in the output format, a file
	   with the initialization string is copied to the initialization buffer.
	*/

	out_v_list = FFV_FIRST_VARIABLE(output->format);
	out_var = FF_VARIABLE(out_v_list);
	while (out_var)
	{
		if (IS_INITIAL(out_var))
		{
			initial_file = fopen(out_var->name, "rb");
			if (initial_file == NULL)
				return(err_push(ERR_OPEN_FILE,"Unable to open file given by INITIAL variable %s", out_var->name));

			if (FF_VAR_LENGTH(out_var) > middle->data->total_bytes - out_var->start_pos) {
			    fclose(initial_file); 	// jhrg 3/18/11
			    return(err_push(ERR_GENERAL, "Length of \"%s\" exceeds internal buffer", out_var->name));
			}

			if (fread(middle->data->buffer + max(1, out_var->start_pos) - 1,
				      sizeof(char),
			          FF_VAR_LENGTH(out_var),
			          initial_file
			        ) != (size_t)FF_VAR_LENGTH(out_var)
			   ) {

			    fclose(initial_file);	// jhrg 3/18/11
			    return(err_push(ERR_READ_FILE,"Unable to load file given by INITIAL variable %s", out_var->name));
			}
			fclose(initial_file);
		}
		else if (IS_CONSTANT(out_var))
		{
			char *targ = middle->data->buffer;
			size_t bytes_to_copy = min(strlen(out_var->name), (size_t)FF_VAR_LENGTH(out_var));

			targ += (max(1, out_var->start_pos) - 1);
			targ += FF_VAR_LENGTH(out_var) - bytes_to_copy;

			memcpy(targ, out_var->name, bytes_to_copy);
		}
		else if (IS_TEXT(out_var))
			memMemset(middle->data->buffer + max(1, out_var->start_pos) - 1, ' ', FF_VAR_LENGTH(out_var),"middle->data->buffer,'\\0',output_increment" );

		middle->data->bytes_used = max(middle->data->bytes_used, out_var->end_pos);
		
		out_v_list = dll_next(out_v_list);
		out_var = FF_VARIABLE(out_v_list);
	}/* end initial variable search */
	
	if (output->data->total_bytes < middle->data->total_bytes)
	{
		error = ff_resize_bufsize(middle->data->total_bytes, &output->data);
		if (error)
			return(error);
	}

	return(error_return);
}

