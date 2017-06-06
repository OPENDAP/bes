/*
 * FILENAME:  cv_units.c
 * CONTAINS:  Conversion functions:
 * Unit Conversions:
 *
 * cv_meters_to_feet
 *	cv_feet_to_meters
 *	cv_abs_sign_to_value
 *	cv_units are the defaults:
 *	v_name to v_name_scaled
 *	v_name_abs and v_name_sign to v_name
 *	v_name to v_name_abs and v_name_sign
 *	v_name_ft to v_name_m
 *	v_name_m to v_name_ft
 *	v_name_base and v_name_exp to value
 *	v_name to v_name_base and v_name_exp
 *
 * Latitude and Longitude Conversions:
 *	FUNCTION	CONVERT FROM			TO
 *
 *	cv_abs		v_name or				v_name_abs or
 *										v_name_sign
 *
 *	cv_deg		v_name_deg				v_name
 *				v_name_min
 *				v_name_sec
 *							-- OR --
 *				v_name_deg_abs			v_name
 *				v_name_min
 *				v_name_sec
 *				v_name_ns
 *				v_name_ew
 *				v_name_sign
 *				geo_quad_code
 *
 *  cv_lon_east	longitude_east			longitude
 *							-- OR --
 *				longitude				longitude_east
 *
 *	cv_deg_nsew	v_name_abs				v_name
 *				v_name_ns
 *				v_name_ew
 *
 *	cv_deg_abs	v_name_deg				v_name_abs
 *				v_name_min
 *				v_name_sec
 *							-- OR --
 *				v_name_deg_abs			v_name_abs
 *				v_name_min
 *				v_name_sec
 *
 *	cv_nsew		v_name					v_name_ns
 *										v_name_ew
 *					  					geog_quad_code
 *							-- OR --
 *				v_name_deg				v_name_ns
 *										v_name_ew
 *					  					geog_quad_code
 *
 *
 *	cv_dms		v_name					v_name_deg
 *							-- OR --
 *				v_name_abs				v_name_min
 *				v_name_ns				v_name_sec
 *				v_name_ew
 *
 *  cv_degabs_nsew
 *				v_name_deg_abs			(v-name)_deg
 *				v_name_ns
 *				v_name_ew
 *
 *	cv_degabs	v_name 					v_name_deg_abs
 *										v_name_min_abs
 *										v_name_sec_abs
 *						-- OR --
 *				v_name_abs				v_name_deg_abs, _min_abs, sec_abs
 *
 *
 *
 *	cv_geog_quad	latitude				geog_quad_code
 *				and longitude
 *							-- OR --
 *				geog_quad_code			latitude_ns
 *										longitude_ew
 *										latitude_sign
 *										longitude_sign
 *
 *  cv_geog_sign	v_name_ns or			v_name_sign
 *				v_name_ew
 *							-- OR --
 *				v_name_sign				v_name_ns
 *										v_name_ew
 *					  					geog_quad_code
 *
 * Seismicity Conversions:
 *
 *	FUNCTION	CONVERT FROM:			TO:
 *
 *	cv_long2mag	longmag					magnitude_mb
 *										magnitude_ms
 *										magnitude_ml
 *										mb-max_like
 *
 *	cv_mag2long	magnitude_mb or			longmag
 *				magnitude_ms or
 *				magnitude_ml
 *
 *	cv_noaa_eq	NOAA binary				earthquake variables
 *
 *  cv_sea_flags	Seattle Catalog		Special Variables
 *
 *  cv_slu_flags	slu_line2 into			non_tectonic
 *											cultural
 *											intensity
 *											magnitude_ml
 *											scale
 *											ml_authority
 *
 * Time Conversions:
 *
 *	FUNCTION			Convert From:			to:
 *
 *	cv_ymd2ser		year, month, day		serial_day_1980 (1980 = 0)
 *						hour, minute, second    OR year_decimal
 *
 *	cv_ser2ymd		serial_day_1980				year, month, day,
 *	                                                                hour, minute, second
 *
 * cv_ydec2ymd    year_decimal                   year, month, day,
 *	                                                                hour, minute, second
 *
 *	cv_ipe2ser		ipe_date				serial_day_1980 (1980 = 0)
 *
 *	cv_ser2ipe		serial_day_1980 (1980 = 0)	ipe_date
 *
 *	cv_date_string	day, month, year OR		date_m/d/y	OR
 *						date_m/d/y 		 OR		date_yymmdd
 *						date_yymmdd
 *
 *
 *	cv_time_string	hour, minute, second OR	time_h:m:s
 *						time_h:m:s			 OR	time_hhmmss
 *						time_hhmmss
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

#define FF_TIME_VAR_YEAR 0
#define FF_TIME_VAR_MONTH 1
#define FF_TIME_VAR_DAY 2
#define FF_TIME_VAR_HOUR 3
#define FF_TIME_VAR_MINUTE 4
#define FF_TIME_VAR_SECOND 5
#define FF_TIME_VAR_CENTURY_AND_YEAR 6
#define FF_TIME_VAR_CENTURY 7


/*
 * NAME:	cv_multiply_value
 *
 * PURPOSE: Multiply the variable by a constant
 *
 * AUTHOR:	T. Habermann (303) 497-6472, haber@ngdc.noaa.gov
 *
 * USAGE:	cv_multiply_value(
 *				VARIABLE_PTR var,			Description of Desired variable
 *				double *converted_value,	Pointer to the Converted value
 *				double conversion_factor,	Factor to multiply to value
 *				char   *var_extension,		Look for input var with extension
 *				FORMAT_PTR input_format,		Input Format Description
 *				FF_DATA_BUFFER input_buffer)	Input Buffer
 *
 * COMMENTS:
 *
 * RETURNS:	Conversion functions return 0 if unsuccessful, 1 if successful
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "cv_multiply_value"

static int cv_multiply_value(
	VARIABLE_PTR 	var,				/* Description of Desired variable */
	double 				*converted_value,	/* Pointer to the Converted value */
	double 				conversion_factor,	/* Factor to multiply to value */
	char 				*var_extension,		/* Look for input var with extension */
	FORMAT_PTR 			input_format,		/* Input Format Description */
	FF_DATA_BUFFER 		input_buffer)		/* Input Buffer */

{
	char 	v_name[MAX_NAME_LENGTH + 24];

	double 	double_value = 0;
	double 	*double_ptr	 = NULL;

	VARIABLE_PTR in_var = NULL;
	int error;
	char *last_underscore = NULL;

	/* Initialize the return value to zero */
	double_ptr = &double_value;
	*converted_value = 0;

	assert(strlen(var->name) < sizeof(v_name));
	strncpy(v_name, var->name, sizeof(v_name) - 1);
	v_name[sizeof(v_name) - 1] = STR_END;

	last_underscore = strrchr(v_name,'_');
	if (last_underscore)
		*last_underscore = STR_END;

    assert(strlen(v_name) < sizeof(v_name) -1);
	assert(sizeof(v_name) - strlen(v_name) > strlen(var_extension));
	strncat(v_name, var_extension, sizeof(v_name) - strlen(v_name) - 1);
	v_name[sizeof(v_name) - 1] = STR_END;

	in_var = ff_find_variable(v_name, input_format);
	if (in_var)
	{
		error = ff_get_double(in_var, input_buffer + (int)(in_var->start_pos - 1), double_ptr, FFF_TYPE(input_format));
	  if (error)
	  	return(0);

		*converted_value = double_value * conversion_factor;
	}

	if (in_var)
		return(1);
	else
		return(0);
}

/*
 * NAME:	cv_meters_to_feet
 *
 * PURPOSE:	Convert v_name_m to v_name_ft
 *
 * AUTHOR:	T. Habermann (303) 497-6472, haber@ngdc.noaa.gov
 *
 * USAGE:	cv_meters_to_feet(
 *				VARIABLE_PTR var,		Description of Desired variable
 *				double *converted_value,Pointer to the Converted value
 *				FORMAT_PTR input_format,	Input Format Description
 *				FF_DATA_BUFFER input_buffer)	Input Buffer
 *
 * COMMENTS:
 *
 * RETURNS:	Conversion functions return 0 if unsuccessful, 1 if successful
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "cv_meters_to_feet"

int cv_meters_to_feet(
	VARIABLE_PTR 	var,				/* Description of Desired variable */
	double 				*converted_value,	/* Pointer to the Converted value */
	FORMAT_PTR 			input_format,		/* Input Format Description */
	FF_DATA_BUFFER 		input_buffer)		/* Input Buffer */
{
	if (cv_multiply_value(var, converted_value, 3.28084, "_m",
	                      input_format, input_buffer)
	   )
		return(1);
	else
		return(0);
}

/*
 * NAME:	cv_feet_to_meters
 *
 * PURPOSE: Convert v_name_ft to v_name_m
 *
 * AUTHOR:	T. Habermann (303) 497-6472, haber@ngdc.noaa.gov
 *
 * USAGE:	cv_feet_to_meters(
 *				VARIABLE_PTR var,		Description of Desired variable
 *				double *converted_value,Pointer to the Converted value
 *				FORMAT_PTR input_format,	Input Format Description
 *				FF_DATA_BUFFER input_buffer)	Input Buffer
 *
 * COMMENTS:
 *
 * RETURNS:	Conversion functions return 0 if unsuccessful, 1 if successful
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "cv_feet_to_meters"

int cv_feet_to_meters(
	VARIABLE_PTR 	var,				/* Description of Desired variable */
	double 				*converted_value,	/* Pointer to the Converted value */
	FORMAT_PTR 			input_format,		/* Input Format Description */
	FF_DATA_BUFFER 		input_buffer)		/* Input Buffer */
{
	if (cv_multiply_value(var, converted_value, 0.3048, "_ft",
	                      input_format, input_buffer)
	   )
		return(1);
	else
		return(0);
}

#ifdef CV_ABS_SIGN_TO_VALUE_NEVER_CALLED
/*
 * NAME:	cv_abs_sign_to_value
 *
 * PURPOSE: convert v_name_abs and v_name_sign  to  v_name
 *
 * AUTHOR:	T. Habermann (303) 497-6472, haber@ngdc.noaa.gov
 *
 * USAGE:	cv_abs_sign_to_value(
 *				VARIABLE_PTR var,		Description of Desired variable
 *				double *converted_value,Pointer to the Converted value
 *				FORMAT_PTR input_format,	Input Format Description
 *				FF_DATA_BUFFER input_buffer)	Input Buffer
 *
 * COMMENTS:
 *
 * RETURNS:	Conversion functions return 0 if unsuccessful, 1 if successful
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "cv_abs_sign_to_value"

int cv_abs_sign_to_value(VARIABLE_PTR var,	/* Description of Desired variable */
	double *converted_value,					/* Pointer to the Converted value */
	FORMAT_PTR input_format,					/* Input Format Description */
	FF_DATA_BUFFER input_buffer)				/* Input Buffer */
{
	char		v_name[MAX_NAME_LENGTH + 24];	/* Variable Name */

	VARIABLE_PTR var_source;

	name_length = strlen(var->name);

	/* find the value */
	memcpy(v_name, var->name, name_length);
	*(v_name + name_length) = STR_END;

	var_source = ff_find_variable(strcat(v_name,"_abs"), input_format);
	if (!var_source)
		return(0);

	memcpy(align, input_buffer + (int)(var_source->start_pos - 1), FF_VAR_LENGTH(var_source));
	error = ff_get_double(var_source, align, converted_value, FFF_TYPE(input_format));
  if (error)
  	return(0);

	/* Now Get The Sign */
	*(v_name + name_length) = STR_END;

	var_source = ff_find_variable(strcat(v_name,"_sign"), input_format);
	if (!var_source)
	{
		if (*converted_value <= 0.0)
			*converted_value *= -1.0;
		return(1);
	}

	ch = input_buffer + var_source->start_pos - 1;
	if (*ch == '-')
		*converted_value *= -1.0;

	return(1);
}
#endif /* CV_ABS_SIGN_TO_VALUE_NEVER_CALLED */

/*
 * NAME:	ff_get_string
 *
 * PURPOSE: Reads data to an ASCII string using info from var.
 *			The output string has the precision of the input variable.
 *
 * AUTHOR:	S. D. Davis, National Geophysical Center, September 1990
 *
 * USAGE:	ff_get_string(
 *				VARIABLE *var, 		The input variable description
 *				void *data_ptr,				Address of the variable
 *				char *variable_str,			Destination (null-termined sring)
 *				FF_TYPES_t format_type)		Input format type
 *
 * ERRORS:
 *		Pointer not defined,"var"
 *		Possible memory corruption, "var"
 *		Pointer not defined,"data_ptr"
 *		Pointer not defined,"variable_str"
 *		Out of memory,"tmp_str"
 *		Unknown variable type,"binary var->type"
 *		Unknown format type,NULL
 *
 * COMMENTS:
 *
 * RETURNS:	if processed correctly 0, else -1 if error occurs
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */

#undef ROUTINE_NAME
#define ROUTINE_NAME "ff_get_string"
static int ff_get_string
	(
	 VARIABLE_PTR var,       /* The variable description */
	 FF_DATA_PTR data_ptr,   /* Address of the variable */
	 char *variable_str,     /* Destination (null-terminated sring) */
	 FF_TYPES_t format_type	 /* Format type */
	)
{
	size_t var_length;
	unsigned int decimal_shift;

	unsigned short num_chars = 0;

	char *ch = NULL;
	char *last_char = NULL;
	char *tmp_str	= NULL;

	/*error checking on undefined parameter pointers*/
	assert(data_ptr && variable_str);
	FF_VALIDATE(var);

	/* initialize the pointer to the beginning of the variable: */
	ch = (char *)data_ptr;
	var_length = FF_VAR_LENGTH(var);

	/* Character variables are handled the same way in either case*/
	if (IS_TEXT(var))
	{
		memcpy(variable_str, ch, var_length);
		variable_str[var_length] = STR_END;
		return(0);
	}

	/* so that switch will include the composite Format types */
	format_type &= FFF_FILE_TYPES;
	switch (format_type)
	{
		case FFF_ASCII:
		case FFF_FLAT:

			/* ASCII Input:
				Numeric variables must be left justified in the variable
				string for the precision determinations to work:

				Move the pointer to the first non-blank character in the
				input variable string and decrease the number of characters
				to copy.

				Example:
					Before: ___123
					            |
							  ch      var_length = 6

					After:  ___123
					                 |
							        ch   var_length = 3
			*/

			while (*ch == ' ' && var_length)
			{
				--var_length;
				++ch;
			}

			/* The string is blank so fill from left with precision
			number of 0's */
			if (var_length == 0)
			{
				assert(var->precision >= 0);

				var_length = num_chars = (unsigned short)(var->precision + 1);
				ch = (char *)data_ptr;
				for ( ; num_chars > 0; num_chars--, ch++)
					*ch = '0';
				ch = data_ptr;
			}
			else
			{
				/* ASCII Input:
					Numeric variables cannot have blanks on the end for the
					precision determinations to work:

					Pad the right side of the string with blanks.

					Example:	var_length = 6
						Before: 123___
						             |
								     tmp_str

						After:  123000
						          |
								  tmp_str
				*/

				tmp_str = ch + var_length - 1;
				while (*tmp_str == ' ')
				{
					*tmp_str = '0';
					tmp_str--;
				}
			}

			/* Copy the variable into variable_str */
			memcpy(variable_str, ch, var_length);
			variable_str[var_length] = STR_END;
		break;

		case FFF_BINARY:

			/* copy data to variable and put it into a string */
			/* tmp_str is a string created to avoid allignment errors on the SUN */
			tmp_str = (char *)memMalloc(var_length + 1, "ff_get_string: tmp_str");
			if (!tmp_str)
			{
				err_push(ERR_MEM_LACK,"tmp_str");
				return(-1);
			}

			memMemcpy(tmp_str, (char *)data_ptr, var_length,NO_TAG);
			tmp_str[var_length] = STR_END;

			switch (FFV_DATA_TYPE(var))
			{
				case FFV_INT8:
					sprintf(variable_str, fft_cnv_flags[FFNT_INT8], *(int8 *)tmp_str);
				break;

				case FFV_UINT8:
					sprintf(variable_str, fft_cnv_flags[FFNT_UINT8], *(uint8 *)tmp_str);
				break;

				case FFV_INT16:
					sprintf(variable_str, fft_cnv_flags[FFNT_INT16], *(int16 *)tmp_str);
				break;

				case FFV_UINT16:
					sprintf(variable_str, fft_cnv_flags[FFNT_UINT16], *(uint16 *)tmp_str);
				break;

				case FFV_INT32:
					sprintf(variable_str, fft_cnv_flags[FFNT_INT32], *(int32 *)tmp_str);
				break;

				case FFV_UINT32:
					sprintf(variable_str, fft_cnv_flags[FFNT_UINT32], *(uint32 *)tmp_str);
				break;

				case FFV_INT64:
					sprintf(variable_str, fft_cnv_flags[FFNT_INT64], *(int64 *)tmp_str);
				break;

				case FFV_UINT64:
					sprintf(variable_str, fft_cnv_flags[FFNT_UINT64], *(uint64 *)tmp_str);
				break;



				case FFV_FLOAT32:	/* Binary to ASCII float */
					sprintf(variable_str, fft_cnv_flags_prec[FFNT_FLOAT32], var->precision, *((float32 *)tmp_str));
				break;

				case FFV_FLOAT64:	/* Binary to ASCII double */
					sprintf(variable_str, fft_cnv_flags_prec[FFNT_FLOAT64], var->precision, *((float64 *)tmp_str));
				break;

				case FFV_ENOTE:	/* Binary to ASCII double */
						sprintf(variable_str, fft_cnv_flags_prec[FFNT_ENOTE], var->precision, *((float64 *)tmp_str));
				break;

				default:			/* Unknown variable type */
					assert(!ERR_SWITCH_DEFAULT);
					err_push(ERR_SWITCH_DEFAULT, "%d, %s:%d", (int)FFV_DATA_TYPE(var), os_path_return_name(__FILE__), __LINE__);
					memFree(tmp_str, "tmp_str");
					return(-1);
			}

			memFree(tmp_str, "ff_get_string: tmp_str");
		break;

		default:
			assert(!ERR_SWITCH_DEFAULT);
			err_push(ERR_SWITCH_DEFAULT, "%d, %s:%d", (int)format_type, os_path_return_name(__FILE__), __LINE__);
			return(-1);
	}

	/*	Variable strings which are shorter than the precision
		can cause some problems:

		Example:	precision  = 6 String: 123 Number = 0.000123
					var_length = 3
					decimal_shift = 3

                        Before: 123____
                                |  |
                                |  last_char
                                |  destination
                                |  source

                        After:  123123
                                |
                                ch

					After:  000123
	*/
	var_length = strlen(variable_str);
	if (strchr(variable_str, '-'))
		--var_length;

	assert(var->precision >= 0);
	if ((short)var_length <= var->precision)
	{
		decimal_shift = var->precision - var_length + 1;

		/* Left pad with zeroes */
		last_char = memStrrchr(variable_str, STR_END,NO_TAG);
		memmove(last_char + (int)(decimal_shift - var_length),
		        last_char - var_length,
		        var_length);

		ch = last_char - var_length;
		while (decimal_shift--)
		{
			*ch = '0';
			ch++;
		}

		ch[var_length] = STR_END;
	}

	return(0);
}

/*
 * NAME:	cv_ser2ymd
 *
 * PURPOSE:	This function converts a time given as either a serial_day_1980 date
 *			with January 1, 1980 as 0 (serial_day_1980) or an IPE date in minutes AD
 *			into year, month, day, hour, minute, second.
 *
 * AUTHOR:	The code was modified from that given by Michael Covington,
 *			PC Tech Journal, December, 1985, PP. 136-142.
 *			Ted Habermann, NGDC, (303)497-6472, haber@mail.ngdc.noaa.gov
 *
 * USAGE:	cv_ser2ymd(
 *				VARIABLE_PTR out_var,
 *				double        *conv_var,
 *				FORMAT        *input_format,
 *				char          *input_buffer)
 *
 * COMMENTS:
 *
 * RETURNS:	1 if successful, else 0
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */

#undef ROUTINE_NAME
#define ROUTINE_NAME "cv_ser2ymd"

int cv_ser2ymd(VARIABLE_PTR out_var, double *conv_var, FORMAT_PTR input_format, FF_DATA_BUFFER input_buffer)
{
	char scratch_buffer[256];
	VARIABLE_PTR in_var = NULL;
	char *output_names[] = {
		"year",
		"month",
		"day",
		"hour",
		"minute",
		"second",
		"century_and_year",
		"century",

		/* DON'T EVEN THINK OF ADDING SOMETHING HERE WITHOUT INCREASING
		THE number_of_outputs. */
	};

	unsigned char number_of_outputs = 8;

	unsigned char ipe = 0;
	unsigned char leap = 0;
	unsigned char output_variable = 0;

	int		centuries = 0;
	int		real_leap_yrs = 0;
	int		correct = 0;

	double serial_day_1980;
	double decimal;
	double fprior_days;
	double raw_mon;
	double year, month, day, hour, minute, second;

	short prior_days, extra_days, mon_const;
	long int_year, int_month, int_hour, int_minute, century;

	long adj_serial_day_1980, base_days, whole_serial_day_1980;
	int error;
	int variable_length;
/*	int int_day; */

	/* Define the Variables */

	/* Check input format to see if variables are present */

	in_var = ff_find_variable("serial_day_1980", input_format);	/* Get the serial date */

	if(!in_var){
		/* Check for old variable name */
		in_var = ff_find_variable("serial", input_format);
		if(!in_var)	return (0);
	}

	if(!in_var){
		/* Check for IPE date */
		in_var = ff_find_variable("ipe_date", input_format);
		if(!in_var)	return (0);
		ipe = 1;
	}

	assert(FF_VAR_LENGTH(in_var) < sizeof(scratch_buffer));

	variable_length = min(sizeof(scratch_buffer) - 1, FF_VAR_LENGTH(in_var));

	memcpy(scratch_buffer, input_buffer + in_var->start_pos - 1, variable_length);
	scratch_buffer[FF_VAR_LENGTH(in_var)] = STR_END;
	error = ff_get_double(in_var, scratch_buffer, &serial_day_1980, input_format->type);
  if (error)
  	return(0);

	if(ipe){		/* Convert ipe date to serial_day_1980 date
					Define time: 1,040,874,840 = number of minutes
					to end of 1979 1440 minutes / day */
		serial_day_1980 = (serial_day_1980 - 1040874840) / 1440.0;
	}

	whole_serial_day_1980 = (long)(floor(serial_day_1980));

	/* Now the serial_day_1980 number is known, define the output variable */
	for(output_variable = 0; output_variable < number_of_outputs; ++output_variable){
		if (!strcmp(out_var->name, output_names[output_variable]))
			break;
	}

	/* Now begin the extraction of the output variables and exit
	when appropriate. There are two major branches: one deals with
	portions of a day and one with units larger than one day */

	switch(output_variable)
	{
	case FF_TIME_VAR_YEAR:
	case FF_TIME_VAR_CENTURY_AND_YEAR:
	case FF_TIME_VAR_MONTH:
	case FF_TIME_VAR_DAY:
	case FF_TIME_VAR_CENTURY:

        /* 28855 days in this century before 1980 */
		adj_serial_day_1980 = whole_serial_day_1980 + 28855;

		year = 1901+ adj_serial_day_1980 / 365.25;

		/* Now year is known, apply Gregorian correction */
		centuries = (int)floor((1980 - year) / 100.0); /* The number of centuries counted (including the year 0)*/
		real_leap_yrs = (int)floor(centuries / 4.0); /* The number of centuries that were leap years (divisible by 400) */
		correct = centuries - real_leap_yrs;
		adj_serial_day_1980 -= correct;

		int_year = (short) (year + DOUBLE_UP);
		base_days = (long)((int_year - 1901) * 365.25 + DOUBLE_UP);
		extra_days = (short) (adj_serial_day_1980 - base_days);	/* days since beginning of year */
		if ((int_year % 4 == 0 && int_year % 100 != 0) || int_year % 400 == 0)
			leap = 1;
		if ((int)extra_days > 59 + (int)leap){ 	/*  month is not Jan or Feb */
			mon_const = 1;
			extra_days = leap;
#if 0
			// This looks like a bug to me... Not sure what was
			// meant. 05/14/03 jhrg
			extra_days = extra_days = leap;
#endif
		}
		else{					/* if month is Jan or Feb, add 12 to month so
								that Feb 29 will be last day of year */
			extra_days += 365;
			mon_const = 13;
		}

		raw_mon = (extra_days + 63) / 30.6001;
		month = raw_mon - mon_const;
		int_month = (short)(month + DOUBLE_UP);

		fprior_days = (float)((int_month + mon_const) * 30.6001 - 63);
		prior_days = (short)fprior_days; /* days between beg. of yr & beg. of current mon.*/
		day = extra_days - prior_days;

		if(day == 0 && int_month == 13){	/* Dec 31 of leap years is treated specially */
			day = 31;
			month = 12.0;
			int_month = 12;
		}

		/* Times during the last minute of the year sometimes have problems */
		if(day == 31 && int_month == 0){
			month = 12.0;
			int_month = 12;
			year -= 1.0;
			int_year -= 1;
		}

		/* The time variables are special conversions.
		If they are integer variables with a precision of 0,
		truncate at this point.  */

		switch(output_variable){
		case FF_TIME_VAR_CENTURY:
			if(IS_INTEGER(out_var) && out_var->precision == 0)
				*conv_var =  (short)(year / 100.0 + DOUBLE_UP);
			else
				*conv_var = year/100.0;
			return(1);

		case FF_TIME_VAR_YEAR:
			century =  (short)(year / 100.0) * (short)100;
			if(IS_INTEGER(out_var) && out_var->precision == 0)
				*conv_var = int_year - century;
			else
				*conv_var = year - century;
			return(1);

		case FF_TIME_VAR_CENTURY_AND_YEAR:
			if(IS_INTEGER(out_var) && out_var->precision == 0)
				*conv_var = int_year;
			else
				*conv_var = year;
			return(1);

		case FF_TIME_VAR_MONTH:
			if(IS_INTEGER(out_var) && out_var->precision == 0)
				*conv_var = int_month;
			else
				*conv_var = month;
			return(1);

		case FF_TIME_VAR_DAY:
			if(IS_INTEGER(out_var) && out_var->precision == 0)
				*conv_var =  (short)(day + DOUBLE_UP);
			else
				*conv_var = day;
			return(1);
		}

	case FF_TIME_VAR_HOUR:
	case FF_TIME_VAR_MINUTE:
	case FF_TIME_VAR_SECOND:

		decimal = serial_day_1980 - whole_serial_day_1980;

		hour = decimal * 24.0;
		int_hour = (short)(hour + DOUBLE_UP);
		if(int_hour == 24){
			hour = 0.0;
			int_hour = 0;
		}

        decimal = hour - int_hour;
		minute = decimal * 60.0;
		int_minute = (short)(minute + DOUBLE_UP);

        decimal = minute - int_minute;
        second = decimal * 60.0;
        if(second < 0.0) second = 0.0;

		switch(output_variable){
		case FF_TIME_VAR_HOUR:	/* hour */
			if(IS_INTEGER(out_var) && out_var->precision == 0)
				*conv_var =  int_hour;
			else
				*conv_var = hour;
			break;

		case FF_TIME_VAR_MINUTE: /* minute */
			if(IS_INTEGER(out_var) && out_var->precision == 0)
				*conv_var = int_minute;
			else
				*conv_var = minute;
			break;

		case FF_TIME_VAR_SECOND: /* second */
			if(IS_INTEGER(out_var) && out_var->precision == 0)
				*conv_var =  (short)(second + DOUBLE_UP);
			else
				*conv_var = second;
			break;
		}
		return(1);
	}
	return(0);
}

/*
 * NAME:	cv_ydec2ymd
 *
 * PURPOSE:	This function converts a time given as decimal years
 *			into year, month, day, hour, minute, second.
 *
 * AUTHOR:	Ted Habermann, NGDC, (303)497-6472, haber@mail.ngdc.noaa.gov
 *
 * USAGE:	cv_ydec2ymd(
 *				VARIABLE_PTR out_var,
 *				double        *conv_var,
 *				FORMAT        *input_format,
 *				char          *input_buffer)
 *
 * COMMENTS:
 *
 * RETURNS:	1 if successful, else 0
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */

#undef ROUTINE_NAME
#define ROUTINE_NAME "cv_ydec2ymd"

int cv_ydec2ymd(VARIABLE_PTR out_var, double *conv_var, FORMAT_PTR input_format, FF_DATA_BUFFER input_buffer)
{
	short days_per_month[13] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};
	short days_per_leap_month[13] = {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366};

	static double last_input_value = -989898.989898;
	char scratch_buffer[256];
	VARIABLE_PTR in_var = NULL;
	char *output_names[] = {
		"year",
		"month",
		"day",
		"hour",
		"minute",
		"second",
		"century_and_year",
		"century",

		/* DON'T EVEN THINK OF ADDING SOMETHING HERE WITHOUT INCREASING
		THE number_of_outputs. */
	};

	unsigned char number_of_outputs = 8;
	unsigned char output_variable = 0;

	double decimal, year_decimal;
/*
	double day_per_year = 1.0 / 365.0;
	double day_per_leap_year = 1.0 / 366.0;
*/

	static double century_and_year, century, year, month, day, hour, minute, second;
	static long int_century_and_year, int_century, int_year, int_month, int_day, int_hour, int_minute, int_second;
	int error;
	int variable_length;

	/* Define the Variables */
	/* Check input format to see if variables are present */
	in_var = ff_find_variable("year_decimal", input_format);	/* Get the serial date */
	if(!in_var)	return (0);

	assert(FF_VAR_LENGTH(in_var) < sizeof(scratch_buffer));

	variable_length = min(sizeof(scratch_buffer) - 1, FF_VAR_LENGTH(in_var));

	memcpy(scratch_buffer, input_buffer + in_var->start_pos - 1, variable_length);
	scratch_buffer[FF_VAR_LENGTH(in_var)] = STR_END;
	error = ff_get_double(in_var, scratch_buffer, &year_decimal, input_format->type);
  if (error)
  	return(0);

	/* These functions keep track of the last value that they dealt with and only compute
	conversion variables if they do not match. */
	if(year_decimal != last_input_value)
	{	/* Extract output variables. */

		last_input_value = year_decimal;

		int_century_and_year = (long)(year_decimal + DOUBLE_UP);
		century_and_year = (double)int_century_and_year;

		int_century = (long)century_and_year / 100;
		century =  (double) int_century;

		int_year = int_century_and_year - int_century * (short)100;
		if(century < 0) int_year *= -1;
		year =  (double) int_year;

		decimal = fabs(year_decimal - century_and_year);
		int_month = 0;

		/* determine if this is a leap year */
	 	if((int_century_and_year % 4 == 0 && int_century_and_year % 100 != 0) || int_century_and_year % 400 == 0)
		{
			decimal *= 366.0;	/* Convert Decimal to days */
			while(decimal > days_per_leap_month[int_month])
				int_month++;
			if(int_month > 1)
				decimal -= days_per_leap_month[int_month - 1];
		} else {
			decimal *= 365.0;	/* Convert Decimal to days */
			while(decimal > days_per_month[int_month])
				int_month++;
			if(int_month > 1)
				decimal -= days_per_month[int_month - 1];
		}
		month = (double)int_month;

		/* 1 must be added to the day because there is no day 0 */
		day = decimal + 1;
		int_day = (short)(day + DOUBLE_UP);
		decimal = (day - int_day > 0.0) ? (day - int_day) * 24.0 : 0.0;	/* Convert decimal to days */

		hour = decimal;
		int_hour = (short) (hour + DOUBLE_UP);
		decimal = (hour - int_hour > 0.0) ? (hour - int_hour) * 60.0 : 0.0; /* Convert decimal to minutes */

		minute = decimal;
		int_minute = (short) (minute +DOUBLE_UP) ;
		decimal = (minute - int_minute > 0.0) ? (minute - int_minute) * 60.0 : 0.0; /* Convert decimal to seconds */

		second = decimal;
		int_second = (short) second;
	} /* Done calculating times */

	/* Define the output variable */
	for(output_variable = 0; output_variable < number_of_outputs; ++output_variable){
		if(!memStrcmp(out_var->name, output_names[output_variable],NO_TAG))
			break;
	}
	switch(output_variable){
		case FF_TIME_VAR_CENTURY_AND_YEAR:
			if(IS_INTEGER(out_var) && out_var->precision == 0)
				*conv_var = int_century_and_year;
			else
				*conv_var = century_and_year;
			break;

		case FF_TIME_VAR_CENTURY:
			if(IS_INTEGER(out_var) && out_var->precision == 0)
				*conv_var =  int_century;
			else
				*conv_var = century;
			break;

		case FF_TIME_VAR_YEAR:
			if(IS_INTEGER(out_var) && out_var->precision == 0)
				*conv_var = int_year;
			else
				*conv_var = year;
			break;

		case FF_TIME_VAR_MONTH:
			if(IS_INTEGER(out_var) && out_var->precision == 0)
				*conv_var = int_month;
			else
				*conv_var = month;
			break;

		case FF_TIME_VAR_DAY:
			if(IS_INTEGER(out_var) && out_var->precision == 0)
				*conv_var =  (short)(day + DOUBLE_UP);
			else
				*conv_var = day;
			break;

		case FF_TIME_VAR_HOUR:	/* hour */
			if(IS_INTEGER(out_var) && out_var->precision == 0)
				*conv_var =  int_hour;
			else
				*conv_var = hour;
			break;

		case FF_TIME_VAR_MINUTE: /* minute */
			if(IS_INTEGER(out_var) && out_var->precision == 0)
				*conv_var = int_minute;
			else
				*conv_var = minute;
			break;

		case FF_TIME_VAR_SECOND: /* second */
			if(IS_INTEGER(out_var) && out_var->precision == 0)
				*conv_var =  (short)(second + DOUBLE_UP);
			else
				*conv_var = second;
			break;

		default:
			return(0);
	}
	return(1);
}

/*
 * NAME:  	cv_date_string
 *
 * PURPOSE:	This function converts to various string representations
 *			of dates. The position of the date and time elements in the
 *			string are controlled by the variable name:
 *				cc = century
 *				yy = year
 *				mm = month
 *				dd = day
 *				hh = hour
 *				mi = minute
 *				ss = second
 *			The presently supported strings are:
 *				day/month/year	(date_d/m/y)
 *				mmddyy			(date_ddmmyy)
 *
 *
 * AUTHOR:	Ted Habermann, NGDC, (303)497-6472, haber@mail.ngdc.noaa.gov
 *
 * USAGE:	cv_date_string(
 *				VARIABLE_PTR out_var,
 *				double        *serial_day_1980,
 *				FORMAT        *input_format,
 *				char          *input_buffer)
 *
 * COMMENTS:
 *
 * RETURNS:	1 if successful, else 0
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */

#undef ROUTINE_NAME
#define ROUTINE_NAME "cv_date_string"

static int helper_1
	(
	 char *name,
	 double *yy_mm_dd,
	 FORMAT_PTR input_format,
	 FF_DATA_BUFFER input_buffer
	)
{
	VARIABLE_PTR tmp_var = NULL;

	tmp_var = ff_create_variable(name);
	if (!tmp_var)
		return(0);

	return(cv_ser2ymd(tmp_var, yy_mm_dd, input_format, input_buffer));
}

static int helper_2
	(
	 char *name,
	 double *yy_mm_dd,
	 FORMAT_PTR input_format,
	 FF_DATA_BUFFER input_buffer
	)
{
/*
	char *last_underscore = NULL;
*/
	VARIABLE_PTR tmp_var = NULL;

	tmp_var = ff_create_variable(name);
	if (!tmp_var)
		return(0);

	return(cv_ydec2ymd(tmp_var, yy_mm_dd, input_format, input_buffer));
}

int cv_date_string(VARIABLE_PTR out_var, double *output, FORMAT_PTR input_format, FF_DATA_BUFFER input_buffer)
{
	char scratch_buffer[256];
	char *last_underscore = NULL;
	VARIABLE_PTR in_var = NULL;
	enum {DATE_NAMES_COUNT = 3};

	/* Must match an input variable name with one of the following
	*/
	char *date_names[DATE_NAMES_COUNT + 1] =
	{
		"date_mm/dd/yy",
		"date_dd/mm/yy",
		"date_yymmdd",

		/* DON'T EVEN THINK OF ADDING SOMETHING HERE WITHOUT INCREASING
		THE number_of_names. */

		NULL,
	};

	char *ch_ptr;
	char *target;

	int variable_length;
	unsigned int string_number;
	int i;

	double yy_mm_dd;

	char sec_str[4]= {STR_END};
	char min_str[4]= {STR_END};
	char hour_str[4]= {STR_END};
	char day_str[4]= {STR_END};
	char month_str[4]= {STR_END};
	char year_str[5]= {STR_END};
	char century_str[5]= {STR_END};

	FF_VALIDATE(input_format);

	assert(sizeof(*output) == sizeof(double));
	memset((void *)output, ' ', sizeof(*output));

	in_var = NULL;

	/* Search the Input Format to Find the Date String */
	/* The first time this function is called it is an experimental call, to
	see if the conversion can be done. If this call succeeds, the desired
	conversion variable is added to the input format. The next time it is
	called, to do the actual conversions, this variable is there as a FFV_CONVERT
	variable. We are not interested in finding the convert variable here, we
	need the input variable. */

	string_number = DATE_NAMES_COUNT;
	for (i = DATE_NAMES_COUNT; i; i--)
	{
		in_var = ff_find_variable(date_names[i - 1], input_format);
		if (in_var && in_var->type != FFV_CONVERT)
		{
			string_number = i - 1;
			break;
		}
	}

	if (string_number == DATE_NAMES_COUNT)
	{
		err_push(ERR_CONVERT_VAR, out_var->name);
		return(0);
	}

	if(string_number < DATE_NAMES_COUNT)
	{		/* Input variable is data string */
		/* Copy The String to buffer */
		assert(FF_VAR_LENGTH(in_var) < sizeof(scratch_buffer));

		variable_length = min(sizeof(scratch_buffer) - 1, FF_VAR_LENGTH(in_var));

		memcpy(scratch_buffer, input_buffer + in_var->start_pos - 1, variable_length);
		scratch_buffer[variable_length] = STR_END;

		/* Skip leading blanks */
		ch_ptr = scratch_buffer;
		while (*ch_ptr == ' ')
			++ch_ptr;

		switch(string_number)
		{
		case 0:		/* Date String is mm/dd/yy */
		 	snprintf(month_str, 4, "%02d", atoi(strtok(ch_ptr, "/:, ")));
		 	snprintf(day_str, 4, "%02d", atoi(strtok(NULL, "/:, ")));
		 	snprintf(year_str, 5, "%02d", atoi(strtok(NULL, "/:, ")));
		break;

		case 1:		/* Date String is dd/mm/yy */
		 	snprintf(day_str, 4, "%02d", atoi(strtok(ch_ptr, "/:, ")));
		 	snprintf(month_str, 4, "%02d", atoi(strtok(NULL, "/:, ")));
		 	snprintf(year_str, 5, "%02d", atoi(strtok(NULL, "/:, ")));
		break;

		case 2:		/* Date String is yymmdd */
			if (strlen(ch_ptr) == 5)
			{
				memMemmove(ch_ptr + 1, ch_ptr, 6,NO_TAG);
				*ch_ptr = '0';
			}
		 	memMemmove(year_str ,ch_ptr, 2,NO_TAG);
		 	memMemmove(month_str,ch_ptr + 2, 2,NO_TAG);
		 	memMemmove(day_str  ,ch_ptr + 4, 2,NO_TAG);
			*(year_str + 2) = STR_END;
			*(month_str + 2) = STR_END;
			*(day_str + 2) = STR_END;
		break;

		default:
			assert(!ERR_SWITCH_DEFAULT);
			err_push(ERR_SWITCH_DEFAULT, "%s, %s:%d", ROUTINE_NAME, os_path_return_name(__FILE__), __LINE__);
			return(0);
		}
	}
	else
	{		/* Input is data, not a string */
		in_var = ff_find_variable("second", input_format);
		if(in_var)ff_get_string(in_var, input_buffer + in_var->start_pos - 1, sec_str, input_format->type);

		in_var = ff_find_variable("minute", input_format);
		if(in_var)ff_get_string(in_var, input_buffer + in_var->start_pos - 1, min_str, input_format->type);

		in_var = ff_find_variable("hour", input_format);
		if(in_var)ff_get_string(in_var, input_buffer + in_var->start_pos - 1, hour_str, input_format->type);

		in_var = ff_find_variable("day", input_format);
		if(in_var)ff_get_string(in_var, input_buffer + in_var->start_pos - 1, day_str, input_format->type);

		in_var = ff_find_variable("month", input_format);
		if(in_var)ff_get_string(in_var, input_buffer + in_var->start_pos - 1, month_str, input_format->type);

		in_var = ff_find_variable("year", input_format);
		if(in_var)ff_get_string(in_var, input_buffer + in_var->start_pos - 1, year_str, input_format->type);

		in_var = ff_find_variable("century", input_format);
		if(in_var)ff_get_string(in_var, input_buffer + in_var->start_pos - 1, century_str, input_format->type);

		in_var = ff_find_variable("serial_day_1980", input_format);
		if(in_var)
		{	/* Convert serial_day_1980 to year, month, day */

			/* In converting these real time values to integers I try to
			have correct truncation for times down to 100th of a second.
			In other words, if a time is within 100th of a second of the
			next major unit, it may be incorrectly rounded up.

			0.01 second = 0.001666 minute
						= 0.000003 hour
						= 0.00000012 day
			*/

			if (!helper_1("second", &yy_mm_dd, input_format, input_buffer))
				return(0);
			sprintf(sec_str, "%02d", (short)(yy_mm_dd + 0.5));	/* Round Seconds */

			if (!helper_1("minute", &yy_mm_dd, input_format, input_buffer))
				return(0);
			sprintf(min_str, "%02d", (short)(yy_mm_dd + DOUBLE_UP));

			if (!helper_1("hour", &yy_mm_dd, input_format, input_buffer))
				return(0);
			sprintf(hour_str, "%02d", (short)(yy_mm_dd + DOUBLE_UP));

			if (!helper_1("day", &yy_mm_dd, input_format, input_buffer))
				return(0);
			sprintf(day_str, "%02d", (short)(yy_mm_dd + DOUBLE_UP));

			if (!helper_1("month", &yy_mm_dd, input_format, input_buffer))
				return(0);
			sprintf(month_str, "%02d", (short)(yy_mm_dd + DOUBLE_UP));

			if (!helper_1("year", &yy_mm_dd, input_format, input_buffer))
				return(0);
			sprintf(year_str, "%02d", (short)yy_mm_dd);

			if (!helper_1("century", &yy_mm_dd, input_format, input_buffer))
				return(0);
			sprintf(century_str, "%02d", (short)yy_mm_dd);
		}

		/* Unsuccessful so far, try year_decimal for input */
		in_var = ff_find_variable("year_decimal", input_format);
		if(in_var)
		{	/* Convert year_decimal to year, month, day */
			/* In converting these real time values to integers I try to
			have correct truncation for times down to 100th of a second.
			In other words, if a time is within 100th of a second of the
			next major unit, it may be incorrectly rounded up.

			0.01 second = 0.001666 minute
						= 0.000003 hour
						= 0.00000012 day
			*/
			if (!helper_2("second", &yy_mm_dd, input_format, input_buffer))
				return(0);
			sprintf(sec_str, "%02d", (short)(yy_mm_dd + 0.5));	/* Round Seconds */

			if (!helper_2("minute", &yy_mm_dd, input_format, input_buffer))
				return(0);
			sprintf(min_str, "%02d", (short)(yy_mm_dd + DOUBLE_UP));

			if (!helper_2("hour", &yy_mm_dd, input_format, input_buffer))
				return(0);
			sprintf(hour_str, "%02d", (short)(yy_mm_dd + DOUBLE_UP));

			if (!helper_2("day", &yy_mm_dd, input_format, input_buffer))
				return(0);
			sprintf(day_str, "%02d", (short)(yy_mm_dd + DOUBLE_UP));

			if (!helper_2("month", &yy_mm_dd, input_format, input_buffer))
				return(0);
			sprintf(month_str, "%02d", (short)(yy_mm_dd + DOUBLE_UP));

			if (!helper_2("year", &yy_mm_dd, input_format, input_buffer))
				return(0);
			sprintf(year_str, "%02d", (short)yy_mm_dd);

			if (!helper_2("century", &yy_mm_dd, input_format, input_buffer))
				return(0);
			sprintf(century_str, "%02d", (short)yy_mm_dd);
		}
	}
	if (!in_var)
		return(0);

	/* Now the input data is known, Determine the string positions in the
		output string */

/*	string_number = 0;
	while(memStrcmp(out_var->name, date_names[string_number],NO_TAG))++string_number;
*/
	last_underscore = memStrrchr(out_var->name,'_', "out_var->name,'_'");
	if (!last_underscore)return(0);	/* Invalid output variable name */
	++last_underscore;
	if (strlen(last_underscore) > sizeof(double))
		return(0); /* Output string too long */

	/* Search for tokens in the output variable name */
	ch_ptr = memStrstr(last_underscore, "ss", "ss");
	if(ch_ptr){
		target = (char *)((char *)output + (ch_ptr - last_underscore));
		memMemcpy(target, sec_str, 2, "second");
	}

	ch_ptr = memStrstr(last_underscore, "mi", "mi");
	if(ch_ptr){
		target = (char *)((char *)output + (ch_ptr - last_underscore));
		memMemcpy(target, min_str, 2, "minute");
	}

	ch_ptr = memStrstr(last_underscore, "hh", "hh");
	if(ch_ptr){
		target = (char *)((char *)output + (ch_ptr - last_underscore));
		memMemcpy(target, hour_str, 2, "hour");
	}

	ch_ptr = memStrstr(last_underscore, "dd", "dd");
	if(ch_ptr){
		target = (char *)((char *)output + (ch_ptr - last_underscore));
		memMemcpy(target, day_str, 2, "day");
	}

	ch_ptr = memStrstr(last_underscore, "mm", "mm");
	if(ch_ptr){
		target = (char *)((char *)output + (ch_ptr - last_underscore));
		memMemcpy(target, month_str, 2, "month");
	}

	ch_ptr = memStrstr(last_underscore, "yy", "yy");
	if(ch_ptr){
		target = (char *)((char *)output + (ch_ptr - last_underscore));
		memMemcpy(target, year_str, 2, "year");
	}

	ch_ptr = memStrstr(last_underscore, "cc", "cc");
	if(ch_ptr){
		target = (char *)((char *)output + (ch_ptr - last_underscore));
		memMemcpy(target, century_str, 2, "century");
		if(*target == ' ')*target = '0';
	}

	/* Search for /'s in the output variable name */
	ch_ptr = last_underscore;
	while( (ch_ptr = strchr(++ch_ptr, '/')) != NULL)
	{
		target = (char *)((char *)output + (ch_ptr - last_underscore));
		memMemcpy(target, "/", 1, "Slash");
	}

	/* convert leading zero's to spaces */

	for (target = (char *)output; *target == '0' /* zero */; target++)
		*target = ' ';

	/* Should right justify strings in field */

	return(1);
}

static void setup_vname
	(
	 char *orig,
	 char *new_name,
	 char **last_underscore
	)
{
	unsigned name_length = 0;

	assert(strlen(orig) < MAX_NAME_LENGTH + 24); /* (size of new_name) */

	name_length = min(strlen(orig), MAX_NAME_LENGTH + 24 - 1);
	memcpy(new_name, orig, name_length);
	new_name[name_length] = STR_END;

	*last_underscore = strrchr(new_name, '_');
}

/*
 * NAME:	cv_units
 *
 * PURPOSE: miscellaneous unit conversions, called as last resort
 *
 * AUTHOR:	T. Habermann (303) 497-6472, haber@ngdc.noaa.gov
 *
 * USAGE:	cv_units(
 *				VARIABLE_PTR var,		Description of Desired variable
 *				double *converted_value,Pointer to the Converted value
 *				FORMAT_PTR input_format,	Input Format Description
 *				FF_DATA_BUFFER input_buffer)	Input Buffer
 *
 * COMMENTS:
 *
 * RETURNS:	Conversion functions return 0 if unsuccessful, 1 if successful
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "cv_units"

int cv_units(
	VARIABLE_PTR 	var,				/* Description of Desired variable */
	double 				*converted_value,	/* Pointer to the Converted value */
	FORMAT_PTR 			input_format,		/* Input Format Description */
	FF_DATA_BUFFER 		input_buffer)		/* Input Buffer */

{
	char 	v_name[MAX_NAME_LENGTH + 24];	/* Variable Name */

	double 	*double_ptr	 = NULL;
	double 	double_value = 0.0;
/*
	double	scale		= 0.0;
	double	offset		= 0.0;

	int count = 0;
	int i = 0;
	unsigned variable_length = 0;

	VARIABLE_PTR scale_var = NULL;
*/
	VARIABLE_PTR var_source = NULL;
	int error;
	FF_DATA_BUFFER ch = NULL;
	char *last_underscore = NULL;

	setup_vname(var->name, v_name, &last_underscore);

#ifdef SCALED_CONVERSION_NOT_WORKING
/*	CONVERT FROM:			TO:
 *
 *	v_name				v_name_scaled
 *
*/
	if (last_underscore && !strcmp(last_underscore, "_scaled"))
	{
		*last_underscore = STR_END;

		var_source = ff_find_variable(v_name, input_format);
		if (var_source)
		{
    	/* get the scale and offset */
/*			scale_var=var_source->next; */
			double_ptr=(double *)scale_var->name;
			scale=*double_ptr;
			offset=*(double_ptr+1);

			/* initialize the return value to 0 */
			double_ptr = &double_value;
			*converted_value=0;

			memcpy(align, input_buffer + var_source->start_pos - 1, FF_VAR_LENGTH(var_source));
			ff_get_double(var_source, align, double_ptr, FFF_TYPE(input_format));
			*converted_value = double_value*scale + offset;
			return(1);
		}
		else
			setup_vname(var->name, v_name, &last_underscore);
	}
#endif /* SCALED_CONVERSION_NOT_WORKING */

/*	CONVERT FROM:			TO:
 *
 *	v_name_m				v_name_ft
 *
*/
	if (last_underscore && !strcmp(last_underscore, "_ft"))
	{
		*last_underscore = STR_END;

		assert(sizeof(v_name) - strlen(v_name) > 2);
		strncat(v_name, "_m", sizeof(v_name) - strlen(v_name) - 1);
		v_name[sizeof(v_name) - 1] = STR_END;

		var_source = ff_find_variable(v_name, input_format);
		if (var_source)
		{		/*  value in meters found */

			/* Initialize the return value to zero */
			double_ptr = &double_value;
			*converted_value = 0;

			error = ff_get_double( var_source, input_buffer + var_source->start_pos - 1, double_ptr, FFF_TYPE(input_format));
		  if (error)
		  	return(0);

			*converted_value = double_value * 3.28084;
			return(1);
		}
		else
			setup_vname(var->name, v_name, &last_underscore);
	}

/*	CONVERT FROM:			TO:
 *
 *	v_name_ft				v_name_m
 *
*/

	if (last_underscore && !strcmp(last_underscore, "_m"))
	{
		*last_underscore = STR_END;

		assert(sizeof(v_name) - strlen(v_name) > 3);
		strncat(v_name, "_ft", sizeof(v_name) - strlen(v_name) - 1);
		v_name[sizeof(v_name) - 1] = STR_END;

		var_source = ff_find_variable(v_name, input_format);
		if (var_source)
		{		/*  value in feet found */
			/* Initialize the return value to zero */
			double_ptr = &double_value;
   			*converted_value = 0;

			error = ff_get_double( var_source, input_buffer + var_source->start_pos - 1, double_ptr, FFF_TYPE(input_format));
		  if (error)
		  	return(0);

			*converted_value = double_value * 0.3048;
			return(1);
		}
		else
			setup_vname(var->name, v_name, &last_underscore);
	}


/*	CONVERT FROM:			TO:
 *
 *	v_name					v_name_abs and
 *							v_name_sign
*/

	if (last_underscore && !strcmp(last_underscore + 1, "abs"))
	{
		*last_underscore = STR_END;

		var_source = ff_find_variable(v_name, input_format);
		if (var_source)
		{
			/* source has been found, determine value */
			error = ff_get_double(var_source, input_buffer + var_source->start_pos - 1, &double_value, FFF_TYPE(input_format));
		  if (error)
		  	return(0);

			*converted_value = fabs(double_value);
			return(1);
		}
		else
			setup_vname(var->name, v_name, &last_underscore);
	}

	if (last_underscore && !strcmp(last_underscore + 1, "sign"))
	{
		*last_underscore = STR_END;

		var_source = ff_find_variable(v_name, input_format);
		if (var_source)
		{
			/* source has been found, determine value */
			error = ff_get_double(var_source, input_buffer + var_source->start_pos - 1, &double_value, FFF_TYPE(input_format));
		  if (error)
		  	return(0);

			ch = (FF_DATA_BUFFER)converted_value;
			*ch = (char)((double_value >= 0.0) ? '+' : '-');
			return(1);
		}
		else
			setup_vname(var->name, v_name, &last_underscore);
	}

/*	CONVERT FROM:			TO:
 *
 *	v_name					v_name_negate (negate input value)
 */

	if (last_underscore && !strncmp(last_underscore + 1, "negate", 6))
	{
		*last_underscore = STR_END;

		var_source = ff_find_variable(v_name, input_format);
		if (var_source)
		{
			error = ff_get_double(var_source, input_buffer + var_source->start_pos - 1, &double_value, FFF_TYPE(input_format));
		  if (error)
		  	return(0);

			*converted_value = -double_value;
			return(1);
		}
		else
			setup_vname(var->name, v_name, &last_underscore);
	}

/*	CONVERT FROM:			TO:
 *
 *	v_name_abs and			v_name
 *	v_name_sign
 *
*/

	setup_vname(var->name, v_name, &last_underscore);

	assert(sizeof(v_name) - strlen(v_name) > 4);
	strncat(v_name, "_abs", sizeof(v_name) - strlen(v_name) - 1);
	v_name[sizeof(v_name) - 1] = STR_END;

	var_source = ff_find_variable(v_name, input_format);
	if (var_source)
	{		/* absolute value found */
		error = ff_get_double(var_source, input_buffer + (int)(var_source->start_pos - 1), &double_value, FFF_TYPE(input_format));
	  if (error)
	  	return(0);

		*converted_value = double_value;

		/* Now Get The Sign */
		setup_vname(var->name, v_name, &last_underscore);

		assert(sizeof(v_name) - strlen(v_name) > 5);
		strncat(v_name, "_sign", sizeof(v_name) - strlen(v_name) - 1);
		v_name[sizeof(v_name) - 1] = STR_END;

		var_source = ff_find_variable(v_name, input_format);
		if (!var_source)
		{
			if (*converted_value <= 0.0)
				*converted_value *= -1.0;
			return(1);
		}

		ch = input_buffer + var_source->start_pos - 1;
		if (*ch == '-')
			*converted_value *= -1.0;
		return(1);
	}

/*	CONVERT FROM:			TO:
 *
 *  v_name_negate   v_name
 */

	setup_vname(var->name, v_name, &last_underscore);

	assert(sizeof(v_name) - strlen(v_name) > 7);
	strncat(v_name, "_negate", sizeof(v_name) - strlen(v_name) - 1);
	v_name[sizeof(v_name) - 1] = STR_END;

	var_source = ff_find_variable(v_name, input_format);
	if (var_source)
	{
		error = ff_get_double(var_source, input_buffer + (int)(var_source->start_pos - 1), &double_value, FFF_TYPE(input_format));
	  if (error)
	  	return(0);

		*converted_value = -double_value;
		return(1);
	}

	/* Check for generic date string conversion */

	setup_vname(var->name, v_name, &last_underscore);

	if (!strncmp(v_name,"date", 4))
	{
		return(cv_date_string(var, converted_value, input_format,input_buffer));
	}

	/* else no conversion found */
	return(0);
}  /* End cv_units() */

/*
 * NAME:	cv_abs
 *
 * PURPOSE:	convert from v_name  to  v_name_abs OR v_name_sign
 *
 * AUTHOR: T. Habermann, NGDC, (303) 497-6472, haber@ngdc.noaa.gov
 *
 * USAGE: 	cv_abs(
 *				VARIABLE_PTR var,		Description of Desired variable
 *				double *converted_value,Pointer to the Converted value
 *				FORMAT_PTR input_format,	Input Format Description
 *				char   *input_buffer)	Input Buffer
 *
 * COMMENTS: This function deals with degree / minute / second and NSEW
 *				conversions.
 *
 * RETURNS:	0 if unsuccessful, 1 if successful
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "cv_abs"

int cv_abs(VARIABLE_PTR var,			/* Description of Desired variable */
	double 				*converted_value,	/* Pointer to the Converted value */
	FORMAT_PTR 			input_format,		/* Input Format Description */
	FF_DATA_BUFFER 		input_buffer)		/* Input Buffer */

{
	VARIABLE_PTR var_source = NULL;
/*
	double double_value = 0.0;
*/
	char *last_underscore = NULL;
	char v_name[MAX_NAME_LENGTH + 24];	/* Variable Name */

	int error;
	/* Initialize the return value to zero */
	*converted_value = 0.0;

	/*	See if v_name exists	*/
	assert(sizeof(v_name) > strlen(var->name));
	strncpy(v_name, var->name, sizeof(v_name) - 1);
	v_name[sizeof(v_name) - 1] = STR_END;

	last_underscore  = strrchr(v_name, '_');
	if(last_underscore) *last_underscore = STR_END;

	var_source = ff_find_variable(v_name, input_format);

	if (var_source) {
		/* source has been found, determine value */
		double double_value = 0.0;

		error = ff_get_double(var_source, input_buffer + (int)(var_source->start_pos - 1), &double_value,input_format->type);
		if (error)
			return(0);

		if(memStrcmp(last_underscore + 1, "abs", "last_underscore+1,\"abs\"") == 0)
			*converted_value = fabs(double_value);

		if(memStrcmp(last_underscore + 1, "sign", "last_underscore+1, \"sign\"") == 0)
		{
			FF_DATA_BUFFER ch = NULL;

			ch = (char HUGE *)converted_value;
			*ch = (char)((double_value >= 0.0) ? '+' : '-');
		}

		return(1);
	}
	return(0);
}


/*
 * NAME:	cv_deg
 *
 * PURPOSE:	CONVERT FROM			TO
 *
 *			v_name_deg				v_name
 *			v_name_min
 *			v_name_sec
 *						-- OR --
 *			v_name_deg_abs			v_name
 *			v_name_min
 *			v_name_sec
 *			v_name_ns
 *			v_name_ew
 *			geo_quad_code
 *			WMO_quad
 *
 * AUTHOR: T. Habermann, NGDC, (303) 497-6472, haber@ngdc.noaa.gov
 *
 * USAGE:  	cv_deg(
 *				VARIABLE_PTR var,		Description of Desired variable
 *				double *converted_value,Pointer to the Converted value
 *				FORMAT_PTR input_format,	Input Format Description
 *				char   *input_buffer)	Input Buffer
 *
 * COMMENTS: This function deals with degree / minute / second and NSEW
 *				conversions.
 *
 * RETURNS:	0 if unsuccessful, 1 if successful
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "cv_deg"

int cv_deg(VARIABLE_PTR var,		/* Description of Desired variable */
	double *converted_value,		/* Pointer to the Converted value */
	FORMAT_PTR input_format,			/* Input Format Description */
	FF_DATA_BUFFER input_buffer)				/* Input Buffer */

{
	VARIABLE_PTR var_sec = NULL;
	VARIABLE_PTR var_min = NULL;
	VARIABLE_PTR var_deg = NULL;
	unsigned name_length = 0;

	double double_value = 0.0;
	char *first_underscore = NULL;
	char v_name[MAX_NAME_LENGTH + 24];	/* Variable Name */

	VARIABLE_PTR	var_nsew 	= NULL;
	char 				sign 		= 1;
	int error;

	/* Initialize return value to zero */
	*converted_value = 0;

	/* For each extension, see if exists in input_format.
	   If it does, compute the appropriate value */

	assert(strlen(var->name) < sizeof(v_name));
	name_length = min(strlen(var->name), sizeof(v_name) - 1);
	v_name[name_length] = STR_END;

	/* find the degrees */
	memcpy(v_name, var->name, name_length);
	first_underscore = strchr(v_name, '_');
	if(!first_underscore)
		first_underscore = v_name + name_length;

	assert(sizeof(v_name) - strlen(v_name) > 4);
	strncat(v_name, "_deg", sizeof(v_name) - strlen(v_name) - 1);
	v_name[sizeof(v_name) - 1] = STR_END;

	var_deg = ff_find_variable(v_name, input_format);
	if (!var_deg)
	{
		assert(sizeof(v_name) - strlen(v_name) > 4);
		strncat(v_name, "_abs", sizeof(v_name) - strlen(v_name) - 1);
		v_name[sizeof(v_name) - 1] = STR_END;

		var_deg = ff_find_variable(v_name, input_format);
	}

	if (var_deg)
	{
		error = ff_get_double(var_deg, input_buffer + (int)(var_deg->start_pos - 1), &double_value,input_format->type);
		if (error)
			return(0);

		*converted_value += double_value;
		if(*converted_value < 0.0)
			sign = -1;
	}

	/* Converted value is now in degrees, now find the minutes */
	memcpy(v_name, var->name, name_length);
	v_name[name_length] = STR_END;


	assert(sizeof(v_name) - strlen(v_name) > 4);
	strncat(v_name, "_min", sizeof(v_name) - strlen(v_name) - 1);
	v_name[sizeof(v_name) - 1] = STR_END;

	var_min = ff_find_variable(v_name, input_format);
	if (!var_min)
	{
		assert(sizeof(v_name) - strlen(v_name) > 4);
		strncat(v_name, "_abs", sizeof(v_name) - strlen(v_name) - 1);
		v_name[sizeof(v_name) - 1] = STR_END;

		var_min = ff_find_variable(v_name, input_format);
	}

	if (var_min) {
		error = ff_get_double(var_min, input_buffer + (int)(var_min->start_pos - 1), &double_value,input_format->type);
	  if (error)
	  	return(0);

		*converted_value += double_value / (sign * 60.);
		if(*converted_value < 0.0) sign = -1;
	}


	/* Converted value is now in degrees and minutes, now find the seconds */
	memcpy(v_name, var->name, name_length);
	v_name[name_length] = STR_END;


	assert(sizeof(v_name) - strlen(v_name) > 4);
	strncat(v_name, "_sec", sizeof(v_name) - strlen(v_name) - 1);
	v_name[sizeof(v_name) - 1] = STR_END;

	var_sec = ff_find_variable(v_name, input_format);
	if(!var_sec)
	{
		assert(sizeof(v_name) - strlen(v_name) > 4);
		strncat(v_name, "_abs", sizeof(v_name) - strlen(v_name) - 1);
		v_name[sizeof(v_name) - 1] = STR_END;

		var_sec = ff_find_variable(v_name, input_format);
	}

	if (var_sec) {
		error = ff_get_double(var_sec, input_buffer + (int)(var_sec->start_pos - 1), &double_value,input_format->type);
	  if (error)
	  	return(0);

		*converted_value += double_value / (sign * 3600.);
	}


	/* Check the sign of the value if var_nsew is found */

	if (var_deg) {
		memcpy(v_name, var->name, name_length);
		v_name[name_length] = STR_END;

		assert(sizeof(v_name) - strlen(v_name) > 3);
		strncat(v_name, "_ns", sizeof(v_name) - strlen(v_name) - 1);
		v_name[sizeof(v_name) - 1] = STR_END;

		var_nsew = ff_find_variable(v_name, input_format);

		if (!var_nsew) {
			v_name[name_length] = STR_END;

			assert(sizeof(v_name) - strlen(v_name) > 3);
			strncat(v_name, "_ew", sizeof(v_name) - strlen(v_name) - 1);
			v_name[sizeof(v_name) - 1] = STR_END;

			var_nsew = ff_find_variable(v_name, input_format);
		}

		if (!var_nsew) {
			v_name[name_length] = STR_END;

			assert(sizeof(v_name) - strlen(v_name) > 5);
			strncat(v_name, "_sign", sizeof(v_name) - strlen(v_name) - 1);
			v_name[sizeof(v_name) - 1] = STR_END;

			var_nsew = ff_find_variable(v_name, input_format);
		}

		if (!var_nsew) {
			var_nsew = ff_find_variable("geog_quad_code", input_format);
		}

		if (!var_nsew) {
			var_nsew = ff_find_variable("WMO_quad_code", input_format);
		}

		if (var_nsew)
		{
			FF_DATA_BUFFER ch = NULL;

			ch = input_buffer + var_nsew->start_pos - 1;

			/* Take care of the nsew case: Southern and Western
			Hemispheres are negative */
			if(*ch == 'S' || *ch == 's' || *ch == 'W' || *ch == 'w'){
				*converted_value = -*converted_value;
				return(1);
			}

			/* The geographic quad codes of DMA indicate:
				1 = Northeast
				2 = Northwest
				3 = Southeast
				4 = Southwest
			*/

			if(memStrcmp(var_nsew->name, "geog_quad_code", "var_nsew->name,\"geog_quad_code\"") == 0){
				*(first_underscore) = STR_END;
				if(strcmp(v_name, "latitude") == 0 && ((*ch == '3' || *ch == '4') || *ch == '-'))
					*converted_value = -*converted_value;

				if(strcmp(v_name, "longitude") == 0 && ((*ch == '2' || *ch == '4') || *ch == '-'))
					*converted_value = -*converted_value;

			}

			/* The geographic quad codes of WMO indicate:
				1 = Northeast
				7 = Northwest
				3 = Southeast
				5 = Southwest
			*/

			if(memStrcmp(var_nsew->name, "WMO_quad_code", "var_nsew->name,\"WMO_quad_code\"") == 0){
				*(first_underscore) = STR_END;
				if(strcmp(v_name, "latitude") == 0 && (*ch == '3' || *ch == '5'))
					*converted_value = -*converted_value;

				if(strcmp(v_name, "longitude") == 0 && (*ch == '5' || *ch == '7'))
					*converted_value = -*converted_value;

			}

		}

	}

	/* Has a conversion been found? */

	if (var_deg || var_min || var_sec ) return(1);
	else return(0);

}
/*
 * NAME:	cv_lon_east
 *
 * PURPOSE:	CONVERT FROM			TO
 *
 *			longitude_east 			longitude
 *						-- OR --
 *			longitude 				longitude_east
 *			v_name_min
 *			v_name_sec
 *			v_name_ns
 *			v_name_ew
 *			geo_quad_code
 *
 * AUTHOR: T. Habermann, NGDC, (303) 497-6472, haber@ngdc.noaa.gov
 *
 * USAGE:  	cv_lon_east(
 *				VARIABLE_PTR var,		Description of Desired variable
 *				double *converted_value,Pointer to the Converted value
 *				FORMAT_PTR input_format,	Input Format Description
 *				char   *input_buffer)	Input Buffer
 *
 * COMMENTS: This function deals with longitude_east
 *
 * RETURNS:	0 if unsuccessful, 1 if successful
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "cv_lon_east"

int cv_lon_east(VARIABLE_PTR var,		/* Description of Desired variable */
	double *converted_value,		/* Pointer to the Converted value */
	FORMAT_PTR input_format,			/* Input Format Description */
	FF_DATA_BUFFER input_buffer)				/* Input Buffer */

{
	VARIABLE_PTR var_source = NULL;
	double double_value = 0.0;
	int error;
	*converted_value = 0;  	/* Initialize the return value to zero */

	/* Check to see which way the conversion is going */

	if(memStrcmp(var->name, "longitude", "var->name,\"longitude\"") == 0){

		/* Conversion of longitude_east to longitude */
		var_source = ff_find_variable("longitude_east", input_format);
		if(!var_source) return(0);

		error = ff_get_double(var_source, input_buffer + (int)(var_source->start_pos - 1), &double_value,input_format->type);
		if (error)
			return(0);

		if(double_value < 180.0)	*converted_value += double_value;
		else *converted_value = double_value - 360.0;
		return(1);
	}

	/* Conversion of longitude to longitude_east */
	var_source = ff_find_variable("longitude", input_format);
	if(!var_source) return(0);

	error = ff_get_double(var_source, input_buffer + (int)(var_source->start_pos - 1), &double_value,input_format->type);
	if (error)
		return(0);

	if(double_value > .000000000000001)	*converted_value += double_value;
	else *converted_value = 360 + double_value;
	return(1);
}

/*
 * NAME:	cv_deg_nsew
 *
 * PURPOSE:	CONVERT FROM			TO
 *
 *			v_name_abs				v_name
 *			v_name_ns
 *			v_name_ew
 *
 * AUTHOR: T. Habermann, NGDC, (303) 497-6472, haber@ngdc.noaa.gov
 *
 * USAGE:	cv_deg_nsew(
 *				VARIABLE_PTR var,		Description of Desired variable
 *				double *converted_value,Pointer to the Converted value
 *				FORMAT_PTR input_format,	Input Format Description
 *				char   *input_buffer)	Input Buffer
 *
 * COMMENTS: This function deals with degree / minute / second and NSEW
 *				conversions.
 *
 * RETURNS:	0 if unsuccessful, 1 if successful
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "cv_deg_nsew"

int cv_deg_nsew(VARIABLE_PTR var,		/* Description of Desired variable */
	double *converted_value,			/* Pointer to the Converted value */
	FORMAT_PTR input_format,				/* Input Format Description */
	FF_DATA_BUFFER input_buffer)					/* Input Buffer */

{
	unsigned name_length = 0;
	char v_name[MAX_NAME_LENGTH + 24];	/* Variable Name */

	VARIABLE_PTR var_abs;
	VARIABLE_PTR var_nsew;
	int error;

	/* Initialize the return value to zero */

	*converted_value = 0;

	/* See if v_name_abs and v_name_ns (or ew) exist in the input format */

	assert(strlen(var->name) < sizeof(v_name));
	name_length = min(strlen(var->name), sizeof(v_name) - 1);

	strncpy(v_name, var->name, name_length);
	v_name[name_length] = STR_END;

	assert(sizeof(v_name) - strlen(v_name) > 4);
	strncat(v_name, "_abs", sizeof(v_name) - strlen(v_name) - 1);
	v_name[sizeof(v_name) - 1] = STR_END;

	var_abs = ff_find_variable(v_name, input_format);

	strncpy(v_name, var->name, name_length);
	v_name[name_length] = STR_END;

	assert(sizeof(v_name) - strlen(v_name) > 3);
	strncat(v_name, "_ns", sizeof(v_name) - strlen(v_name) - 1);
	v_name[sizeof(v_name) - 1] = STR_END;

	var_nsew = ff_find_variable(v_name, input_format);
	if (!var_nsew) {
		strncpy(v_name, var->name, name_length);
		v_name[name_length] = STR_END;

		assert(sizeof(v_name) - strlen(v_name) > 3);
		strncat(v_name, "_ew", sizeof(v_name) - strlen(v_name) - 1);
		v_name[sizeof(v_name) - 1] = STR_END;

		var_nsew = ff_find_variable(v_name, input_format);
	}

	/* If both var_abs and var_nsew exist, compute the converted value */


	if (var_abs && var_nsew)
	{
		double double_value = 0.0;
		int nsew_char = 0;

		FF_DATA_BUFFER ch = NULL;

		error = ff_get_double(var_abs, input_buffer + (int)(var_abs->start_pos - 1), &double_value,input_format->type);
		if (error)
			return(0);

		*converted_value = double_value;

		ch = input_buffer + var_nsew->start_pos - 1;

	/* Change the sign of converted value depending on
		the value of nsew_char */

		nsew_char = toupper( (int)((* (char HUGE *) ch ) % 128));
		if (nsew_char == 'N' || nsew_char == 'E')
			*converted_value = fabs(*converted_value);
		if (nsew_char == 'S' || nsew_char == 'W')
			*converted_value = -fabs(*converted_value);

	}

	if (var_abs && var_nsew ) return(1);
	else return(0);

}


/*
 * NAME:	cv_deg_abs
 *
 * PURPOSE:	CONVERT FROM			TO
 *
 *			v_name_deg				v_name_abs
 *			v_name_min
 *			v_name_sec
 *						-- OR --
 *			v_name_deg_abs			v_name_abs
 *			v_name_min
 *			v_name_sec
 *
 *
 * AUTHOR: T. Habermann, NGDC, (303) 497-6472, haber@ngdc.noaa.gov
 *
 * USAGE:	cv_deg_abs(
 *				VARIABLE_PTR var,		Description of Desired variable
 *				double *converted_value,Pointer to the Converted value
 *				FORMAT_PTR input_format,	Input Format Description
 *				char   *input_buffer)	Input Buffer
 *
 * COMMENTS: This function deals with degree / minute / second and NSEW
 *				conversions.
 *
 * RETURNS:	0 if unsuccessful, 1 if successful
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "cv_deg_abs"

int cv_deg_abs(VARIABLE_PTR var,
								/* Description of Desired variable */
	double *converted_value,		/* Pointer to the Converted value */
	FORMAT_PTR input_format,			/* Input Format Description */
	FF_DATA_BUFFER input_buffer)				/* Input Buffer */

{
	VARIABLE_PTR var_sec = NULL;
	VARIABLE_PTR var_min = NULL;
	VARIABLE_PTR var_deg = NULL;
	unsigned name_length = 0;
	double double_value = 0.0;
	char *underscore = NULL;

	char v_name[MAX_NAME_LENGTH + 24];	/* Variable Name */

	int error;
	/* Initialize the return value to zero */
	*converted_value = 0.0;

	/* For each extension, see if exists in input_format.
	If it does, compute the appropriate value, also check for _abs extensions */

	assert(strlen(var->name) < sizeof(v_name));
	name_length = min(strlen(var->name), sizeof(v_name) - 1);

	/* Look for v_name_deg and v_name_deg_abs */
	strncpy(v_name, var->name, name_length);
	v_name[name_length] = STR_END;

	underscore = strrchr(v_name, '_');
	*(underscore++) = STR_END;

	assert(sizeof(v_name) - strlen(v_name) > 4);
	strncat(v_name, "_deg", sizeof(v_name) - strlen(v_name) - 1);
	v_name[sizeof(v_name) - 1] = STR_END;
	var_deg = ff_find_variable(v_name, input_format);

	if (!var_deg)
	{
		assert(sizeof(v_name) - strlen(v_name) > 4);
		strncat(v_name, "_abs", sizeof(v_name) - strlen(v_name) - 1);
		v_name[sizeof(v_name) - 1] = STR_END;

		var_deg = ff_find_variable(v_name, input_format);
	}

	if (var_deg) {
		error = ff_get_double(var_deg, input_buffer + (int)(var_deg->start_pos - 1), &double_value,input_format->type);
		if (error)
			return(0);

		*converted_value = fabs(double_value);
	}

	/* Look for v_name_min and v_name_min_abs */
	strncpy(v_name, var->name, name_length);
	v_name[name_length] = STR_END;

	underscore = strrchr(v_name, '_');
	*(underscore++) = STR_END;


	assert(sizeof(v_name) - strlen(v_name) > 4);
	strncat(v_name, "_min", sizeof(v_name) - strlen(v_name) - 1);
	v_name[sizeof(v_name) - 1] = STR_END;

	var_min = ff_find_variable(v_name, input_format);
	if (!var_min)
	{
		assert(sizeof(v_name) - strlen(v_name) > 4);
		strncat(v_name, "_abs", sizeof(v_name) - strlen(v_name) - 1);
		v_name[sizeof(v_name) - 1] = STR_END;

		var_min = ff_find_variable(v_name, input_format);
	}

	if (var_min) {
		error = ff_get_double(var_min, input_buffer + (int)(var_min->start_pos - 1), &double_value,input_format->type);
	  if (error)
	  	return(0);

		*converted_value += fabs(double_value / 60.0);
	}

	/* Look for v_name_sec and v_name_sec_abs */
	strncpy(v_name, var->name, name_length);
	v_name[name_length] = STR_END;

	underscore = strrchr(v_name, '_');
	*(underscore++) = STR_END;


	assert(sizeof(v_name) - strlen(v_name) > 4);
	strncat(v_name, "_sec", sizeof(v_name) - strlen(v_name) - 1);
	v_name[sizeof(v_name) - 1] = STR_END;

	var_sec = ff_find_variable(v_name, input_format);
	if (!var_sec)
	{
		assert(sizeof(v_name) - strlen(v_name) > 4);
		strncat(v_name, "_abs", sizeof(v_name) - strlen(v_name) - 1);
		v_name[sizeof(v_name) - 1] = STR_END;

		var_sec = ff_find_variable(v_name, input_format);
	}

	if (var_sec) {
		error = ff_get_double(var_sec, input_buffer + (int)(var_sec->start_pos - 1), &double_value,input_format->type);
	  if (error)
	  	return(0);

		*converted_value += fabs(double_value / 3600.0);

	}

	/* Has a conversion been found? */

	if (var_deg || var_min || var_sec ) return(1);
	else return(0);

}


/*
 * NAME:	cv_nsew
 *
 * PURPOSE:	CONVERT FROM			TO
 *
 *				v_name					v_name_ns
 *										v_name_ew
 *							-- OR --
 *				v_name_deg				v_name_ns
 *										v_name_ew
 *							-- OR --
 *				v_name_sign				v_name_ns
 *										v_name_ew
 *
 *
 * AUTHOR: T. Habermann, NGDC, (303) 497-6472, haber@ngdc.noaa.gov
 *
 * USAGE:	cv_nsew(
 *				VARIABLE_PTR var,		Description of Desired variable
 *		 		double *converted_value,Pointer to the Converted value
 *				FORMAT_PTR input_format,	Input Format Description
 *				char   *input_buffer)	Input Buffer
 *
 * COMMENTS: This function deals with degree / minute / second and NSEW
 *				conversions.
 *
 * RETURNS:	0 if unsuccessful, 1 if successful
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "cv_nsew"

int cv_nsew(VARIABLE_PTR var,		/* Description of Desired variable */
	double *converted_value,   		/* Pointer to the Converted value */
	FORMAT_PTR input_format,			/* Input Format Description */
	FF_DATA_BUFFER input_buffer)				/* Input Buffer */

{
	VARIABLE_PTR var_source = NULL;
	unsigned name_length = 0;
	double double_value = 0.0;
	char *last_underscore = NULL;
	char *first_underscore = NULL;
	char v_name[MAX_NAME_LENGTH + 24];	/* Variable Name */

	char *nsew;
	int error;

	*converted_value = 0;	/* Initialize the return value to zero */
	nsew = (char *)converted_value;

	/*  See if v_name exists in the input format	*/

	assert(strlen(var->name) < sizeof(v_name));
	name_length = min(strlen(var->name), sizeof(v_name) - 1);

	/* define the first and last underscores */
	memcpy(v_name, var->name, name_length);
	v_name[name_length] = STR_END;

	first_underscore = strchr(v_name, '_');
	last_underscore  = strrchr(v_name, '_');

	*(first_underscore) = STR_END;

	var_source = ff_find_variable(v_name, input_format);

	/* If v_name does not exist, try v_name_deg	*/

	if (!var_source)
	{
		assert(sizeof(v_name) - strlen(v_name) > 4);
		strncat(v_name, "_deg", sizeof(v_name) - strlen(v_name) - 1);
		v_name[sizeof(v_name) - 1] = STR_END;

		var_source = ff_find_variable(v_name, input_format);
	}

	if (!var_source) return(0);

	memcpy(v_name, var->name, name_length);
	v_name[name_length] = STR_END;

	error = ff_get_double(var_source, input_buffer + (int)(var_source->start_pos - 1), &double_value,input_format->type);
	if (error)
		return(0);

	if (!memStrcmp(last_underscore,"_ns", NO_TAG))
		*nsew = (char)((double_value < 0.0) ? 'S' : 'N');
	if (!memStrcmp(last_underscore,"_ew", NO_TAG))
		*nsew = (char)((double_value < 0.0) ? 'W'  : 'E');

	return(1);
}

/*
 * NAME:	cv_dms
 *
 * PURPOSE:	CONVERT FROM			TO
 *
 *			v_name					v_name_deg
 *			v_name_abs				v_name_min
 *			v_name_ns				v_name_sec
 *			ew
 *
 * AUTHOR: T. Habermann, NGDC, (303) 497-6472, haber@ngdc.noaa.gov
 *
 * USAGE:	cv_dms(VARIABLE_PTR var,	Description of Desired variable
 *				double *converted_value,Pointer to the Converted value
 *				FORMAT_PTR input_format,	Input Format Description
 *				char   *input_buffer)	Input Buffer
 *
 * COMMENTS: This function deals with degree / minute / second and NSEW
 *				conversions.
 *
 *			If the degrees are between -1 and 0       minutes are negated
 *			If the minutes are also between -1 and 0  seconds are negated
 *
 *
 * RETURNS:	0 if unsuccessful, 1 if successful
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "cv_dms"

int cv_dms(VARIABLE_PTR var,	/* Description of Desired variable */
	double *converted_value,	/* Pointer to the Converted value */
	FORMAT_PTR input_format,		/* Input Format Description */
	FF_DATA_BUFFER input_buffer)			/* Input Buffer */

{
	unsigned name_length = 0;
	double double_value = 0.0;
	char *last_underscore = NULL;
	char *first_underscore = NULL;
	char v_name[MAX_NAME_LENGTH + 24];	/* Variable Name */

	VARIABLE_PTR var_source = NULL;
	VARIABLE_PTR var_sec = NULL;
	VARIABLE_PTR var_nsew = NULL;
	short abs_marker = 0;
	short negate_value = 0;
	int error;

 	*converted_value = 0;  	/* Initialize the return value to zero */

	assert(strlen(var->name) < sizeof(v_name));
	name_length = min(strlen(var->name), sizeof(v_name) - 1);

	/* define the first and last underscores */
	memcpy(v_name, var->name, name_length);
	v_name[name_length] = STR_END;

	first_underscore = strchr(v_name, '_');
	last_underscore  = strrchr(v_name, '_');

	/* Check to see if the base variable exists */
	*(first_underscore) = STR_END;
	var_source = ff_find_variable(v_name, input_format);

	/* If v_name does not exist, try v_name_abs	*/

	if (!var_source) {

		memcpy(v_name, var->name, name_length);

		*last_underscore = STR_END;

		assert(sizeof(v_name) - strlen(v_name) > 4);
		strncat(v_name, "_abs", sizeof(v_name) - strlen(v_name) - 1);
		v_name[sizeof(v_name) - 1] = STR_END;

		var_source = ff_find_variable(v_name, input_format);

		if (var_source){
			abs_marker++;
			memcpy(v_name, var->name, name_length);
		}

	}

	/* If neither the base name or abs can be found, return */
	if(!var_source) return(0);

	/* At this point either
		1) the variable base (abs_marker == 0) or
		2) _abs 			 (abs_marker == 1)
		is selected as var_source.
	*/

	/* Restore v_name */
	memcpy(v_name, var->name, name_length);

	/* Define the location of the input variable and get it's value */
	error = ff_get_double(var_source, input_buffer + (int)(var_source->start_pos - 1), &double_value,input_format->type);
	if (error)
		return(0);

	/* If the variable being constructed is _deg, check for sign conversion */
	if (!memStrcmp(last_underscore,"_deg", NO_TAG)) {

		if (abs_marker) {

			memcpy(v_name, var->name, name_length);
			*(first_underscore) = STR_END;

			assert(sizeof(v_name) - strlen(v_name) > 3);
			strncat(v_name, "_ns", sizeof(v_name) - strlen(v_name) - 1);
			v_name[sizeof(v_name) - 1] = STR_END;

			var_nsew = ff_find_variable(v_name, input_format);
			if (!var_nsew) {
				memcpy(v_name, var->name, name_length);
				*(first_underscore) = STR_END;

				assert(sizeof(v_name) - strlen(v_name) > 3);
				strncat(v_name, "_ew", sizeof(v_name) - strlen(v_name) - 1);
				v_name[sizeof(v_name) - 1] = STR_END;

				var_nsew = ff_find_variable(v_name, input_format);
			}

			if (var_nsew)
			{
				int nsew_char = 0;
				FF_DATA_BUFFER ch = NULL;

				ch = input_buffer + var_nsew->start_pos - 1;
				nsew_char = toupper( (int)((* (char HUGE *) ch ) % 128));

				if (nsew_char == 'N' || nsew_char == 'E')
						*converted_value = fabs(*converted_value);

				if (nsew_char == 'S' || nsew_char == 'W')
						*converted_value = -fabs(*converted_value);
			}
		}

		*converted_value = (double) ( (int) double_value);
		return(1);
	}

	if (!memStrcmp(last_underscore,"_min", NO_TAG)) {

		/* Check to see if the degrees are between -1 and 0,
		negate the minutes if they are */

		if (double_value < 0.0 && double_value > -1.0) negate_value = 1;
		*converted_value = fabs(double_value);

		*converted_value = 60.0 * fmod(*converted_value, 1.0);

		/* If output minutes needs precision, look for seconds
			and convert to minutes */

		if (var->precision) {

			memcpy(v_name, var->name, name_length);
			*(first_underscore) = STR_END;

			assert(sizeof(v_name) - strlen(v_name) > 4);
			strncat(v_name, "_sec", sizeof(v_name) - strlen(v_name) - 1);
			v_name[sizeof(v_name) - 1] = STR_END;

			var_sec = ff_find_variable(v_name, input_format);
			if (var_sec) {
				error = ff_get_double(var_sec, input_buffer + (int)(var_sec->start_pos - 1), &double_value,input_format->type);
			  if (error)
			  	return(0);

				*converted_value += fabs(double_value / 60.0);
			}

		}
		else *converted_value = (double) ( (int) *converted_value);

		if (*converted_value && negate_value) *converted_value *= -1.0;

		return(1);		/* Done with _min */
	}

	if (!memStrcmp(last_underscore,"_sec", NO_TAG)) {

		/* Check to see if the minutes are between -1 and 0,
		 negate the seconds if they are */

		if (double_value < 0.0 && double_value > -1.0) negate_value = 1;
		*converted_value = fabs(double_value);

		/* convert to minutes */
		*converted_value = 60.0 * (*converted_value -
			(double) ( (int) *converted_value));

		if (negate_value)
			if (*converted_value >= 1.0) negate_value = 0;


		/* convert to seconds */
		*converted_value = 60.0 * (*converted_value -
			(double) ( (int) *converted_value));
	}

	if (negate_value) *converted_value *= -1.0;
	return(1);
}


/*
 * NAME:	cv_degabs_nsew
 *
 * PURPOSE:	CONVERT FROM			TO
 *
 *			v_name_deg_abs			(v-name)_deg
 *			v_name_ns
 *			v_name_ew
 *
 * AUTHOR: T. Habermann, NGDC, (303) 497-6472, haber@ngdc.noaa.gov
 *
 * USAGE:	cv_degabs_nsew(
 *				VARIABLE_PTR var,		Description of Desired variable
 *				double *converted_value,Pointer to the Converted value
 *				FORMAT_PTR input_format,	Input Format Description
 *				char   *input_buffer)	Input Buffer
 *
 * COMMENTS: This function deals with degree / minute / second and NSEW
 *				conversions.
 *
 * RETURNS:	0 if unsuccessful, 1 if successful
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "cv_degabs_nsew"

int cv_degabs_nsew(VARIABLE_PTR var,
								/* Description of Desired variable */
	double *converted_value,			/* Pointer to the Converted value */
	FORMAT_PTR input_format,			/* Input Format Description */
	FF_DATA_BUFFER input_buffer)				/* Input Buffer */

{
	unsigned name_length = 0;
	char v_name[MAX_NAME_LENGTH + 24];	/* Variable Name */

	VARIABLE_PTR var_abs;
	VARIABLE_PTR var_nsew;
	char *extension;
	int error;

	*converted_value = 0;	/* Initialize the return value to zero */

	/* See if v_name_deg_abs and v_name_ns (or ew) exist in the input format */

	assert(strlen(var->name) < sizeof(v_name));
	name_length = min(strlen(var->name), sizeof(v_name) - 1);

	strncpy(v_name, var->name, name_length);
	v_name[name_length] = STR_END;

	extension = strchr(v_name, '_');
	*extension = STR_END;

	assert(sizeof(v_name) - strlen(v_name) > 8);
	strncat(v_name, "_deg_abs", sizeof(v_name) - strlen(v_name) - 1);
	v_name[sizeof(v_name) - 1] = STR_END;

	var_abs = ff_find_variable(v_name, input_format);

	strncpy(v_name, var->name, name_length);
	v_name[name_length] = STR_END;

	extension = strchr(v_name, '_');
	*extension = STR_END;

	assert(sizeof(v_name) - strlen(v_name) > 3);
	strncat(v_name, "_ns", sizeof(v_name) - strlen(v_name) - 1);
	v_name[sizeof(v_name) - 1] = STR_END;

	var_nsew = ff_find_variable(v_name, input_format);
	if (!var_nsew) {
		strncpy(v_name, var->name, name_length);
		v_name[name_length] = STR_END;

		extension = strchr(v_name, '_');
		*extension = STR_END;

		assert(sizeof(v_name) - strlen(v_name) > 3);
		strncat(v_name, "_ew", sizeof(v_name) - strlen(v_name) - 1);
		v_name[sizeof(v_name) - 1] = STR_END;

		var_nsew = ff_find_variable(v_name, input_format);
	}

	/* If both var_abs and var_nsew exist, compute the converted value */

	if (var_abs && var_nsew)
	{
		double double_value = 0.0;
		int nsew_char = 0;
		FF_DATA_BUFFER ch = NULL;

		error = ff_get_double(var_abs, input_buffer + (int)(var_abs->start_pos - 1), &double_value,input_format->type);
		if (error)
			return(0);

		*converted_value = double_value;

		ch = input_buffer + var_nsew->start_pos - 1;

		/* Change the sign of converted value depending on
		the value of nsew_char */

		nsew_char = toupper( (int)((* (char HUGE *) ch ) % 128));

		if (nsew_char == 'N' || nsew_char == 'E')
			*converted_value = fabs(*converted_value);

   		if (nsew_char == 'S' || nsew_char == 'W')
			*converted_value = -fabs(*converted_value);

	}

	if (var_abs && var_nsew ) return(1);
	else return(0);

}


/*
 * NAME:	cv_degabs
 *
 * PURPOSE:	CONVERT FROM			TO
 *
 *			v_name 					v_name_deg_abs, _min_abs, _sec_abs
 *						-- OR --
 *			v_name_abs				v_name_deg_abs, _min_abs, _sec_abs
 *
 *
 * AUTHOR: T. Habermann, NGDC, (303) 497-6472, haber@ngdc.noaa.gov
 *
 * USAGE:	cv_degabs(
 *				VARIABLE_PTR var,		Description of Desired variable
 *				double *converted_value,Pointer to the Converted value
 *				FORMAT_PTR input_format,	Input Format Description
 *				char   *input_buffer)	Input Buffer
 *
 * COMMENTS: This function deals with degree / minute / second and NSEW
 *				conversions.
 *
 * RETURNS:	0 if unsuccessful, 1 if successful
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "cv_degabs"

int cv_degabs(VARIABLE_PTR var,	/* Description of Desired variable */
	double *converted_value,		/* Pointer to the Converted value */
	FORMAT_PTR input_format,			/* Input Format Description */
	FF_DATA_BUFFER input_buffer)				/* Input Buffer */

{
	VARIABLE_PTR var_source = NULL;
	VARIABLE_PTR var_sec = NULL;
	unsigned name_length = 0;
	double double_value = 0.0;
	char *first_underscore = NULL;
	char v_name[MAX_NAME_LENGTH + 24];	/* Variable Name */

	int error;

	/* Initialize the return value to zero */
	*converted_value = 0.0;

	/*  See if v_name exists in the input format	*/

	assert(strlen(var->name) < sizeof(v_name));
	name_length = min(strlen(var->name), sizeof(v_name) - 1);

	strncpy(v_name, var->name, name_length);
	v_name[name_length] = STR_END;

	first_underscore = strchr(v_name, '_');
	*first_underscore = STR_END;

	var_source = ff_find_variable(v_name, input_format);
	if (!var_source) {		/* Check for v_name_abs */

		assert(sizeof(v_name) - strlen(v_name) > 4);
		strncat(v_name, "_abs", sizeof(v_name) - strlen(v_name) - 1);
		v_name[sizeof(v_name) - 1] = STR_END;

		var_source = ff_find_variable(v_name, input_format);
	}

	if(!var_source) return(0);	/* Give up */

	/* Restore v_name and get value of the input v_name */
	memcpy(v_name, var->name, name_length);

	error = ff_get_double(var_source, input_buffer + (int)(var_source->start_pos - 1), &double_value,input_format->type);
	if (error)
		return(0);

	/* construct _deg_abs */
	if (!memStrcmp(first_underscore, "_deg_abs", NO_TAG)){

		*converted_value = fabs( (double) ( (int) double_value) );

		return(1);
	}

	/* construct _min_abs */
	if (!memStrcmp(first_underscore, "_min_abs", NO_TAG)){

		*converted_value = fabs(double_value);

		*converted_value = 60.0 * fmod(*converted_value, 1.0);

		/* If output minutes needs precision, look for seconds
			and convert to minutes */

		if (var->precision) {

			memcpy(v_name, var->name, name_length);
			*(first_underscore) = STR_END;

			assert(sizeof(v_name) - strlen(v_name) > 4);
			strncat(v_name, "_sec", sizeof(v_name) - strlen(v_name) - 1);
			v_name[sizeof(v_name) - 1] = STR_END;

			var_sec = ff_find_variable(v_name,	input_format);
			if (!var_sec)
			{
				assert(sizeof(v_name) - strlen(v_name) > 8);
				strncat(v_name, "_sec_abs", sizeof(v_name) - strlen(v_name) - 1);
				v_name[sizeof(v_name) - 1] = STR_END;

				var_sec = ff_find_variable(v_name, input_format);
			}

			if (var_sec) {
				error = ff_get_double(var_sec, input_buffer + (int)(var_sec->start_pos - 1), &double_value,input_format->type);
			  if (error)
			  	return(0);

				*converted_value += fabs(double_value / 60.0);
			}

		}
		else *converted_value = (double) ( (int) *converted_value);


		return(1);		/* Done with _min */

	}

	/* construct _sec_abs */
	if (!memStrcmp(first_underscore, "_sec_abs", NO_TAG)){

		*converted_value = fabs(double_value);

		/* convert to minutes */
		*converted_value = 60.0 * (*converted_value -
			(double) ( (int) *converted_value));

		/* convert to seconds */
		*converted_value = 60.0 * (*converted_value -
			(double) ( (int) *converted_value));

	}

  	return(1);
} /* End cv_degabs() */


/*
 * NAME:	cv_geog_quad
 *
 * PURPOSE:	The geographic quad codes of DMA indicate:
 *		   		1 = Northeast
 *				2 = Northwest
 *				3 = Southeast
 *				4 = Southwest
 *
 *			The geographic quad codes of WMO indicate:
 *				1 = Northeast
 *				7 = Northwest
 *				3 = Southeast
 *				5 = Southwest
 *
 *			CONVERT FROM			TO
 *
 *			latitude and longitude	geog_quad_code
 *						WMO_quad_code
 *
 *						-- OR --
 *
 *			latitude_ns and
 *			longitude_ew			geog_quad_code
 *							WMO_quad_code
 *
 *						-- OR --
 *
 *			latitude_sign and
 *			longitude_sign			geog_quad_code
 *							WMO_quad_code
 *
 * AUTHOR: T. Habermann, NGDC, (303) 497-6472, haber@ngdc.noaa.gov
 *
 * USAGE:	cv_geog_quad(
 *				VARIABLE_PTR var,		Description of Desired variable
 *				double *converted_value,Pointer to the Converted value
 *				FORMAT_PTR input_format,	Input Format Description
 *				char   *input_buffer)	Input Buffer
 *
 * COMMENTS: This function creates a geographic quadrant code used by
 *			 DMA in their gravity data.
 *
 * RETURNS:	0 if unsuccessful, 1 if successful
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "cv_geog_quad"

int cv_geog_quad(VARIABLE_PTR var,	/* Description of Desired variable */
	double *converted_value,		/* Pointer to the Converted value */
	FORMAT_PTR input_format,			/* Input Format Description */
	FF_DATA_BUFFER input_buffer)				/* Input Buffer */
{
	double double_value = 0.0;
	FF_DATA_BUFFER ch = NULL;

	VARIABLE_PTR var_latlon = NULL;

	unsigned char ns = 0, ew = 0;
	char *nsew;
	int error;

	*converted_value = 0;	/* Initialize the return value to zero */
	nsew = (char *)converted_value;

	/* First search for latitude */
	var_latlon = ff_find_variable("latitude", input_format);

	if(var_latlon)
	{
		ch = input_buffer + var_latlon->start_pos -1;
		error = ff_get_double(var_latlon, ch, &double_value,input_format->type);
		if (error)
			return(0);

		if(double_value <= 0.0) ns = 1;
	}

	else {
		/* try for latitude_ns */
		var_latlon = ff_find_variable("latitude_ns", input_format);

		if(var_latlon){
			ch = input_buffer + var_latlon->start_pos - 1;
			if(*ch == 'S' || *ch == 's') ns = 1;
		}

		else {

			/* try for latitude_sign */
			var_latlon = ff_find_variable("latitude_sign", input_format);
			if(var_latlon){
				ch = input_buffer + var_latlon->start_pos - 1;
				if(*ch == '-') ns = 1;
			}
		}
	}

	if(!ch) return (0);
	ch = NULL;

	/* Next search for longitude */
	var_latlon = ff_find_variable("longitude", input_format);

	if(var_latlon){
		ch = input_buffer + var_latlon->start_pos -1;
		error = ff_get_double(var_latlon, ch, &double_value,input_format->type);
	  if (error)
	  	return(0);

		if(double_value <= 0.0) ew = 1;
	}

	else {	/* try for longitude_ew */
		var_latlon = ff_find_variable("longitude_ew", input_format);

		if(var_latlon){
			ch = input_buffer + var_latlon->start_pos - 1;
			if(*ch == 'W' || *ch == 'w') ew = 1;
		}

		else {
			var_latlon = ff_find_variable("longitude_sign", input_format);

			if(var_latlon){
				ch = input_buffer + var_latlon->start_pos - 1;
				if(*ch == '-') ew = 1;
			}

		}
	}

	if(!ch)return (0);

	if(memStrcmp(var->name, "geog_quad_code", "var->name,\"geog_quad_code\"") == 0){
		if(ns == 0){
			if(ew == 0) *nsew = '1';	/* Northeast */
			else *nsew = '2';			/* Northwest */
		}
		else {
			if(ew == 0) *nsew = '3';	/* Southeast */
			else *nsew = '4';			/* Southwest */
		}
	}

	if(memStrcmp(var->name, "WMO_quad_code", "var->name,\"WMO_quad_code\"") == 0){
		if(ns == 0){
			if(ew == 0) *nsew = '1';	/* Northeast */
			else *nsew = '7';			/* Northwest */
		}
		else {
			if(ew == 0) *nsew = '3';	/* Southeast */
			else *nsew = '5';			/* Southwest */
		}
	}

	return(1);
}

/*
 * NAME:	cv_geog_sign
 *
 * PURPOSE:	Convert between various lat/lon signs
 *
 *			CONVERT FROM			TO
 *
 *			latitude_ns				latitude_sign
 *			longitude_ew			longitude_sign
 *						-- OR --
 *			latitude_sign			latitude_ns
 *			longitude_sign			longitude_ew
 *
 * AUTHOR: T. Habermann, NGDC, (303) 497-6472, haber@ngdc.noaa.gov
 *
 * USAGE:	cv_geog_sign(
 *				VARIABLE_PTR var,		Description of Desired variable
 *				double *converted_value,Pointer to the Converted value
 *				FORMAT_PTR input_format,	Input Format Description
 *				char   *input_buffer)	Input Buffer
 *
 * COMMENTS:
 *
 *
 * RETURNS:	0 if unsuccessful, 1 if successful
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "cv_geog_sign"

int cv_geog_sign(VARIABLE_PTR var,	/* Description of Desired variable */
	double *converted_value,		/* Pointer to the Converted value */
	FORMAT_PTR input_format,			/* Input Format Description */
	FF_DATA_BUFFER input_buffer)				/* Input Buffer */
{
	VARIABLE_PTR var_source = NULL;
	char *last_underscore = NULL;
	FF_DATA_BUFFER ch = NULL;

	char *nsew;

	*converted_value = 0;	/* Initialize the return value to zero */
	nsew = (char *)converted_value;

	last_underscore  = memStrrchr(var->name, '_', "var->name, '_'");

	if(!last_underscore) return(0);

	/* Determine what is being created */
	if(memStrcmp(last_underscore + 1, "sign", "last_underscore+1,\"sign\"") == 0)
	{	/* Creating a sign */

		if(*(var->name + 1) == 'o'){		/* Longitude */
			var_source = ff_find_variable("longitude_ew", input_format);

			if(var_source){
				ch = input_buffer + var_source->start_pos - 1;
				if(*ch == 'W' || *ch == 'w') *nsew = '-';
				else *nsew = '+';
				return(1);
			}
			else return(0);
		}

		/* Latitude */
		var_source = ff_find_variable("latitude_ns", input_format);

		if(var_source){
			ch = input_buffer + var_source->start_pos - 1;
			if(*ch == 'S' || *ch == 's') *nsew = '-';
			else *nsew = '+';
			return(1);
		}
		else return(0);
	}

	if(memStrcmp(last_underscore + 1, "ew", "last_underscore+1,\"ew\"") == 0){	/* Creating ew from a sign */
		var_source = ff_find_variable("longitude_sign", input_format);

		if(var_source){
			ch = input_buffer + var_source->start_pos - 1;
			if(*ch == '+' || *ch == ' ') *nsew = 'E';
			else *nsew = 'W';
			return(1);
		}
		else return(0);

	}

	if(memStrcmp(last_underscore + 1, "ns", "last_underscore+1,\"ns\"") == 0){	/* Creating ns from a sign */

		var_source = ff_find_variable("latitude_sign", input_format);

		if(var_source){
			ch = input_buffer + var_source->start_pos - 1;
			if(*ch == '-') *nsew = 'S';
			else *nsew = 'N';
			return(1);
		}
		else return(0);
	}

	return(0);
}

/*
 * NAME:	cv_long2mag
 *
 * PURPOSE:	This is a conversion function for the FREEFORM system which is
 *			used to get one of the three magnitudes out out the variable
 *			longmag, or to creat longmag from one, two, or three of the
 *			single magnitudes.
 *
 *			Longmag is a long which contains three magnitudes:
 *
 *			ms2 is a magnitude with a precision of 2 and is multiplied by
 *			10,000,000
 *			ms1 is a magnitude with a precision of 2 which is multiplied by
 *			10,000.
 *			mb is a magnitude with a  precision of 1 which is multiplied by 10.
 *
 * AUTHOR:	T. Habermann (303) 497-6472, haber@mail.ngdc.noaa.gov
 *
 * USAGE:	cv_long2mag(
 *				VARIABLE_LIST_PTR out_var,
 *				double *mag,
 *				FORMAT_PTR input,
 *				FF_DATA_BUFFER input_buffer)
 *
 * COMMENTS:
 *
 * RETURNS: 1 on success, 0 on failure
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */

#include <freeform.h>

/***************************************************************
 *
 * Boyer-Moore string search routine
 *
 * Author:    John Rex
 * References: (1) Boyer RS, Moore JS: "A fast string searching
 *                 algorithm"  CACM 20(10):762-777, 1977
 *             (2) plus others--see text of article
 *
 * Compilers: Microsoft C V5.1  - compile as is
 *            Turbo C V2.0      - compile as is
 *
 * Compile time preprocessor switches:
 *    DEBUG - if defined, include test driver
 *
 * Usage:
 *
 *   char *pattern, *text;  - search for pattern in text
 *   unsigned length;       - length of text (the routine does
 *                            NOT stop for '\0' bytes, thus
 *                            allowing it to search strings
 *                            stored sequentially in memory.
 *   char *start;           - pointer to match
 *
 *   char *Boyer_Moore(char *, char *, unsigned);
 *
 *   start = Boyer_Moore(pattern, text, strlen(text);
 *
 *   NULL is returned if the search fails.
 *
 *   Switches: if defined:
 *
 *      DEBUG will cause the search routine to dump its tables
 *            at various times--this is useful when trying to
 *            understand how upMatchJump is generated
 *
 *      DRIVER will cause a test drive to be compiled
 *
 * Source code may be used freely if source is acknowledged.
 * Object code may be used freely.
 **************************************************************/

#include <freeform.h>
#undef ROUTINE_NAME
#define ROUTINE_NAME "ff_strnstr"

#define AlphabetSize 256

/*
 * NAME:	ff_strnstr
 *
 * PURPOSE:
 *			-- see above --
 * AUTHOR:
 *
 * USAGE:	ff_strnstr(
 *				char *pcPattern,	we search for this ...
 *				char *pcText,		... in this text ...
 *				size_t uTextLen)	... up to this length
 *
 * COMMENTS:
 *
 * RETURNS:
 *
 * ERRORS:
 *		Out of memory,"upMatchJump"
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */

FF_DATA_BUFFER ff_strnstr(char *pcPattern, FF_DATA_BUFFER pcText, size_t uTextLen)
{
             /* array of character mis-match offsets */
    unsigned uCharJump[AlphabetSize];
             /* array of offsets for partial matches */
    unsigned *upMatchJump = NULL;
             /* temporary array for upMatchJump calc */
    unsigned *upBackUp = NULL;
    unsigned u, uPatLen;
    unsigned uText, uPat, uA, uB;

	/* Error checking on NULL parameters pcPattern and pcText */

	assert(pcPattern && pcText);

	/* Setup and initialize arrays */
    uPatLen = strlen(pcPattern);
    upMatchJump = (unsigned *)
         memMalloc(2 * (sizeof(unsigned) * (uPatLen + 1)), "ff_strnstr: upmatchjump" );

    if (!upMatchJump) {
		err_push(ERR_MEM_LACK,"upMatchJump");
		return(NULL);
    }

	upBackUp = upMatchJump + uPatLen + 1;

    /* Heuristic #1 -- simple char mis-match jumps ... */
    memMemset((void *)uCharJump, 0, AlphabetSize*sizeof(unsigned),"uCharJump,0,AlphabetSize*sizeof(unsigned)");
    for (u = 0 ; u < uPatLen; u++)
        uCharJump[((unsigned char) pcPattern[u])]
                     = uPatLen - u - 1;

    /* Heuristic #2 -- offsets from partial matches ... */
    for (u = 1; u <= uPatLen; u++)
        upMatchJump[u] = 2 * uPatLen - u;
                                /* largest possible jump */
    u = uPatLen;
    uA = uPatLen + 1;
    while (u > 0) {
        upBackUp[u] = uA;
        while( uA <= uPatLen &&
          pcPattern[u - 1] != pcPattern[uA - 1]) {
            if (upMatchJump[uA] > uPatLen - u)
                upMatchJump[uA] = uPatLen - u;
            uA = upBackUp[uA];
        }
        u--;
        uA--;
    }


    for (u = 1; u <= uA; u++)
        if (upMatchJump[u] > uPatLen + uA - u)
            upMatchJump[u] = uPatLen + uA - u;

    uB = upBackUp[uA];

    while (uA <= uPatLen) {
        while (uA <= uB) {
            if (upMatchJump[uA] > uB - uA + uPatLen)
                upMatchJump[uA] = uB - uA + uPatLen;
            uA++;
        }
        uB = upBackUp[uB];
    }

    /* now search */
    uPat = uPatLen;         /* tracks position in Pattern */
    uText = uPatLen - 1;    /* tracks position in Text */
    while (uText < uTextLen && uPat != 0) {
        if (pcText[uText] == pcPattern[uPat - 1]) { /* match? */
            uText--;    /* back up to next */
            uPat--;
        }
        else { /* a mismatch - slide pattern forward */
            uA = uCharJump[((unsigned char) pcText[uText])];
            uB = upMatchJump[uPat];
            uText += (uA >= uB) ? uA : uB;  /* select larger jump */
            uPat = uPatLen;
        }
    }

    /* return our findings */
    memFree(upMatchJump, "ff_strnstr: upmatchjump");
    if (uPat == 0)
        return(pcText + (uText + 1)); /* have a match */
    else
        return (NULL); /* no match */
}

#if 0
#ifdef ROUTINE_NAME
#undef ROUTINE_NAME
#endif
#define ROUTINE_NAME "ff_get_value"

static int ff_get_value(VARIABLE_PTR var, /* The variable description */
		FF_DATA_PTR data_src, /* The address of the variable */
		FF_DATA_PTR data_dest, /* The destination for the variable */
		FF_TYPES_t format_type) /* The Format Type */
{
	unsigned variable_length;
	int error;

	char *decimal_ptr = NULL;
	char *ch  = NULL;
	char *last_char = NULL;

	char *tmp_str = NULL;

	assert(data_src && data_dest);
	FF_VALIDATE(var)

	variable_length = FF_VAR_LENGTH(var);

	if((unsigned)abs(var->precision) > variable_length)
		return(err_push(ERR_SYNTAX,"variable precision is larger than the variable_length"));

	switch (format_type & FFF_FORMAT_TYPES)
	{
		case FFF_ASCII:         /* If the format is ASCII, copy the string */
		case FFF_FLAT:         /* If the format is FLAT, copy the string */

			/* If the variable type is FFV_CHAR, copy to destination, return now */
			if (IS_TEXT(var))
			{
				memcpy(data_dest, data_src, variable_length);
				*((char *)data_dest + variable_length) = STR_END;
				return(0);
			}

			tmp_str = (char *)memMalloc(variable_length + 2, "tmp_str"); /* one more space in case of memmov */
			if (!tmp_str)
			{
				return(err_push(ROUTINE_NAME,ERR_MEM_LACK,"tmp_str"));
			}
			memMemcpy(tmp_str, data_src, variable_length, NO_TAG);

			/* define a pointer to the end of the string */
			last_char = tmp_str + variable_length;
			*last_char = STR_END;

			/* Fill in trailing blanks with zeroes */
			ch = last_char - 1;
			while (*ch == ' ' && ch >= tmp_str)
				*ch-- = '0';

			if (ch < tmp_str)        /* String is blank, fill with 0 */
				*tmp_str = '0';

			/* If a precision is specified and there is no decimal point then
			the decimal point must be added. This requires that the variable
			become a double */

			/* MAO:c This seems very curious to me.  What's happening is that an
			   integer with an implied decimal point (e.g., an integer scaled by
			   a factor of ten) is going to be converted as a double into data_dest
			   -- do calling functions know about this?
			   LASTLY, this seems inconsistent because this is not done for the
			   binary case below. */

			decimal_ptr = memStrchr(tmp_str, '.', "decimal_ptr");
			if (var->precision && !decimal_ptr)
			{
				/* shift the string to make room for the decimal point */
				memmove(last_char + (int)(1 - var->precision),
				 last_char - var->precision, var->precision + 1);
				*(last_char - var->precision) = '.';
				*(last_char + 1) = STR_END;
				*((double *)data_dest) = atof(tmp_str);
				memFree(tmp_str, "tmp_str");
				return(0);
			}

			/* We are left with non-character variables
				with decimal points in the correct place or
				with precision = 0 and no decimal points.
				These variables can be directly converted. */

			error = ff_string_to_binary(tmp_str, FFV_TYPE(var), data_dest);
			memFree(tmp_str, "tmp_str");
			return(error);

		case FFF_BINARY:
			/* In the BINARY case just copy the variable */
			memcpy(data_dest, data_src, variable_length);
			if(IS_TEXT(var))
				*((char *)data_dest + variable_length) = STR_END;
			return(0);

		default:
			return(err_push(ERR_UNKNOWN_FORM_TYPE, NULL));
	} /* End of format_type switch() */

}/* End ff_get_value() */

#endif /* 0 */

#ifdef ROUTINE_NAME
#undef ROUTINE_NAME
#endif
#define ROUTINE_NAME "cv_long2mag"

int cv_long2mag(
	VARIABLE_PTR	 	out_var,
	double 				*mag,
	FORMAT_PTR 			input,
	FF_DATA_BUFFER 		input_buffer)
{

	VARIABLE_PTR 		var_longmag	= NULL;
/*
	FF_DATA_PTR 		data_source	= NULL;
*/
	unsigned int 		tmp			= 0;
	double 				tmp_mag		= 0;
	unsigned long 		longmag		= 0;
	int error;

	/* First check the input format to make sure that longmag is present */
	var_longmag = ff_find_variable("longmag", input);
	if(!var_longmag){
		return(0);
	}

	/* Get the input variable */

	error = ff_get_double(var_longmag, input_buffer + var_longmag->start_pos - 1, &tmp_mag, input->type);
	if (error)
		return(0);

	longmag = (unsigned long)(tmp_mag + .5);

	/* Now make a decision about which magnitude to output */
	 if(memStrcmp(out_var->name, "mb",NO_TAG) == 0 ||
	 	memStrcmp(out_var->name, "magnitude_mb",NO_TAG) == 0){	/* Body Wave Magnitude */
		tmp = (unsigned int)(longmag % 100);
		*mag = tmp / 10.0;
		return(1);
	}
	if(memStrcmp(out_var->name, "ms1",NO_TAG) == 0 ||
	 	memStrcmp(out_var->name, "magnitude_ms1",NO_TAG) == 0){	/* First Surface Wave Magnitude */
		tmp = (unsigned int)(longmag % 100000);
		*mag = tmp / 10000.;
		return(1);
	}
	if(memStrcmp(out_var->name, "ms2",NO_TAG) == 0 ||
	 	memStrcmp(out_var->name, "ml",NO_TAG) 			== 0 ||
	 	memStrcmp(out_var->name, "magnitude_ms2",NO_TAG) 	== 0 ||
	 	memStrcmp(out_var->name, "magnitude_ml",NO_TAG) 	== 0 ||
	 	memStrcmp(out_var->name, "magnitude_local",NO_TAG)== 0){	/* Second Surface Wave Magnitude */
		*mag = longmag / 10000000.;
		return(1);
	}
	if(memStrcmp(out_var->name, "mb-maxlike",NO_TAG) == 0){	/* Magnitude Difference */
		tmp =(unsigned int)( longmag % 100);			/* Get mb */
		*mag = tmp / 10.0;
		tmp = (unsigned int)(longmag / 100000.);
		*mag -= tmp / 100.0;
		return(1);
	}
	return(0);
}

/*
 * NAME:	cv_mag2long
 *
 * PURPOSE:	This is a conversion function for the FREEFORM system which is
 *			used to create a long with three magnitudes from one, two,
 *			or three of the	single magnitudes.
 *
 *			CONVERTS	mb or ms1 or ms2	TO	longmag
 *
 *			Longmag is a long which contains three magnitudes:
 *
 *			ms2 is a magnitude with a precision of 2 and is multiplied by
 *			10,000,000
 *			ms1 is a magnitude with a precision of 2 which is multiplied by
 *			10,000.
 *			mb is a magnitude with a  precision of 1 which is multiplied by 10.
 *
 * AUTHOR:	T. Habermann (303) 497-6472, haber@mail.ngdc.noaa.gov
 *
 * USAGE:	cv_mag2long(
 *				VARIABLE_PTR out_var,
 *				double		*mag,
 *				FORMAT_PTR	input,
 *				FF_DATA_BUFFER	input_buffer)
 *
 * COMMENTS:
 *
 * RETURNS:	1 if successful, else 0
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "cv_mag2long"

int cv_mag2long
	(
	 VARIABLE_PTR not_used,
	 double *mag,
	 FORMAT_PTR input_format,
	 FF_DATA_BUFFER input_buffer
	)
{
	VARIABLE_PTR mag_var;

	long longmag = 0L;
	double tmp_mag;
	short got_data = 0;
	int error;

	(void)not_used; /* silence compiler warning */
	/* Check input format to see if mb is present */

	mag_var = ff_find_variable("magnitude_mb", input_format);
	if(!mag_var) mag_var = ff_find_variable("mb", input_format);

	if(mag_var){
		if(!got_data)++got_data;

		/* Get the input variable */
		error = ff_get_double(mag_var, input_buffer + mag_var->start_pos - 1, &tmp_mag, input_format->type);
		if (error)
			return(0);

		longmag = (long)(tmp_mag * 10 + .0001);
	}

	/* Check input format to see if ms1 is present */
	mag_var = 	 ff_find_variable("magnitude_ms1", input_format);
	if(!mag_var) mag_var = ff_find_variable("ms", input_format);
	if(!mag_var) mag_var = ff_find_variable("ms1", input_format);

	if(mag_var){
		if(!got_data)++got_data;

		/* Get the input variable */
		error = ff_get_double(mag_var, input_buffer + mag_var->start_pos - 1, &tmp_mag, input_format->type);
		if (error)
			return(0);

		longmag += (long)(tmp_mag * 1000 + .5);
	}

	/* Check input format to see if ms2 is present */
	mag_var = 	 ff_find_variable("magnitude_ms2", input_format);
	if(!mag_var) mag_var = ff_find_variable("magnitude_ml", input_format);
	if(!mag_var) mag_var = ff_find_variable("magnitude_local", input_format);
	if(!mag_var) mag_var = ff_find_variable("ml", input_format);
	if(!mag_var) mag_var = ff_find_variable("ms2", input_format);

	if(mag_var){
		if(!got_data)++got_data;
		/* Get the input variable */
		error = ff_get_double(mag_var, input_buffer + mag_var->start_pos - 1, &tmp_mag, input_format->type);
		if (error)
			return(0);

		longmag += (long)(tmp_mag * 10000000 + .5);
	}
	*mag = (double)longmag;
	return(got_data);
}

/*
 * NAME:	cv_mag_diff
 *
 * PURPOSE:	This is a conversion function for the FREEFORM system which is
 *			used to calculate differences between various magnitudes.
 *
 *			CONVERT FROM:				TO:
 *				magnitude_mb and		mb-max_like
 *				magnitude_max_like
 *
 * AUTHOR:	T. Habermann (303) 497-6472, haber@mail.ngdc.noaa.gov
 *
 * USAGE:	cv_mag_diff(
 *				VARIABLE_LIST_PTR out_var,
 *				double *mag,
 *				FORMAT_PTR input,
 *				FF_DATA_BUFFER input_buffer)
 *
 * COMMENTS:
 *
 * RETURNS:
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */

#include <stdio.h>
#include <string.h>
#include <freeform.h>

#undef ROUTINE_NAME
#define ROUTINE_NAME "cv_mag_diff"

int cv_mag_diff(
	VARIABLE_PTR	 	out_var,
	double 				*mag_diff,
	FORMAT_PTR 			input,
	FF_DATA_BUFFER 		input_buffer)
{

	VARIABLE_PTR 		var_mag_1	= NULL;
	VARIABLE_PTR 		var_mag_2	= NULL;
/*
	FF_DATA_PTR 		data_source	= NULL;
*/
	char				magnitude_1[64] = "magnitude_";
	char				magnitude_2[64] = "magnitude_";
	char				*minus;
	double				mag_1 = 0.0;
	double				mag_2 = 0.0;
	char *name_copy = NULL;
	int error = 0;

	/* Create the magnitude names from the output variable. That variable
	has the form mag_type_1-mag_type_2. */
	name_copy = (char *)memStrdup(out_var->name, "name_copy");
	if (!name_copy)
	{
		err_push(ERR_MEM_LACK, "");
		return(0);
	}

	minus = strchr(name_copy, '-');
	if (!minus)
		error = ERR_GENERAL;

	if (!error)
	{
		*minus = '\0';
		strncat(magnitude_1, name_copy, 63);
		strncat(magnitude_2, minus + 1, 63);

		/* Find the first magnitude */
		var_mag_1 = ff_find_variable(magnitude_1, input);
		if (!var_mag_1)
			error = ERR_GENERAL;

		if (!error)
		{
			var_mag_2 = ff_find_variable(magnitude_2, input);
			if (!var_mag_2)
				error = ERR_GENERAL;
		}

		if (!error)
		{

			error = ff_get_double(var_mag_1, input_buffer + var_mag_1->start_pos - 1, &mag_1, input->type);
			if (!error)
				error = ff_get_double(var_mag_2, input_buffer + var_mag_2->start_pos - 1, &mag_2, input->type);
		}

		if (!error)
			*mag_diff = mag_1 - mag_2;
	}

	memFree(name_copy, "name_copy");

	if (error)
		return(0);
	else
		return(1);
}

/*
 *
 * CONTAINS:
 *
 *	FUNCTION		CONVERT FROM:			TO:
 *
 *	cv_noaa_eq		NOAA bit mast			variables
 *
 */
/*
 * HISTORY:
 *	r fozzard	4/21/95		-rf01
 *		comment out search.h (not needed?)
*/

#include <stdio.h>
#include <string.h>
/*  #include <search.h> comment out search.h (not needed?) -rf01 */
#include <freeform.h>
#undef ROUTINE_NAME
#define ROUTINE_NAME "b_strcmp"


/*
 * NAME:	b_strcmp
 *
 * PURPOSE:	compare function for bsearch routine
 *
 * AUTHOR: TAM, modified by MAO
 *
 * RETURNS:	same as strcmp()
 *
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "b_strcmp"

static int b_strcmp(const char **s1, const char **s2 )
{
	return(strcmp(*s1, *s2));
}

/*
 * NAME:	cv_noaa_eq
 *
 * PURPOSE:	convert from NOAA bit mask to earthquake parameters
 *
 * AUTHOR: T. Habermann, NGDC, (303) 497-6472, haber@mail.ngdc.noaa.gov
 *
 * USAGE: 	cv_noaa_eq(
 *				VARIABLE_PTR var,		Description of Desired variable
 *				double *converted_value,	Pointer to the Converted value
 *				FORMAT_PTR input_format,	Input Format Description
 *				char   *input_buffer)		Input Buffer
 *
 * DESCRIPTION: The NOAA earthquake database includes two files for each
 *				catalog. These files are called the bit mask and the data
 *				mask. The information in both files is held in packed
 *				binary fields. This conversion function reads the bit mask
 *				format and extracts the various earthquake parameters.
 *
 * COMMENTS:
 *
 * RETURNS:	0 if unsuccessful, 1 if successful
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "cv_noaa_eq"

int cv_noaa_eq(VARIABLE_PTR var,		/* Description of Desired variable */
	double *converted_value,			/* Pointer to the Converted value */
	FORMAT_PTR input_format,			/* Input Format Description */
	FF_DATA_BUFFER input_buffer)		/* Input Buffer */

#define NUM_OUTPUT_NAMES 19

{
	long HUGE	*bit_mask	= NULL;
	long HUGE	*data_mask	= NULL;

	short 		year;		/* Year of Earthquake                   */
	short 		day;		/* Day of Earthquake                    */
	short 		hour;		/* Hour of Earthquake                   */
	short 		minute;		/* minute of Earthquake                 */
	short 		b_mag1;		/* Body Wave Magnitude                  */
	short 		b_mag2;		/* Body Wave Magnitude                  */
	short 		s_mag1;		/* S Wave Magnitude                     */
	short 		s_mag2;		/* S Wave Magnitude                     */
	short 		o_mag1;		/* Other  Magnitude                     */
	short 		o_mag2;		/* Other  Magnitude                     */
	short 		l_mag1;		/* Local  Magnitude                     */
	short 		l_mag2;		/* Local  Magnitude                     */
	short 		fe_reg;		/* Flinn Engdahl Region                 */
	short 		index;		/* Source Code                          */
	short 		intens;		/* FELT Intensity                       */
	short 		zh;			/* Z/H COMPONENT                        */
	long 		no_stat;	/* number of stations                   */
	long 		co_lat;		/* Co-latitude of earthquake		*/
	long 		east_lon;	/* East longitude of earthquake		*/
	long 		time;		/* Time code (contains year and month)  */
	float 		second;		/* second of Earthquake                 */
	float 		depth;		/* depth of Earthquake                  */
	float 		b_mag;		/* Body Wave Magnitude                  */
	float 		ms;			/* S Wave Magnitude                     */
	float 		o_mag;		/* Other Magnitude                    	*/
	float 		l_mag;		/* Local Magnitude                     	*/
	char 		culture;	/* Cultural Effects                     */

	VARIABLE_PTR var_source	= NULL;


 	char *output_names[NUM_OUTPUT_NAMES + 1] = {
		"cultural",		/* 0 */
		"day",			/* 1 */
		"depth",		/* 2 */
		"fe_region",	/* 3 */
		"hour",			/* 4 */
		"intensity",	/* 5 */
		"latitude",		/* 6 */
		"longitude",	/* 7 */
		"magnitude_mb",	/* 8 */
		"magnitude_ml",	/* 9 */
		"magnitude_mo",	/* 10 */
		"magnitude_ms",	/* 11 */
		"minute",		/* 12 */
		"month",		/* 13 */
		"no_station",	/* 14 */
		"second",		/* 15 */
		"source_code",	/* 16 */
		"year",			/* 17 */
		"zh_component",	/* 18 */

		/* TO ADD A NAME: (1)increase number_of_names
						  (2)preserve alphabetical order
						  (3)match case statement to index */
		NULL,
	};

	int 	name_number    	= -1;
	char 	**base  		= output_names;
	char 	*result 		= NULL;

	/* Check to see if a NOAA bit mask exists in the input format */
	var_source = ff_find_variable("cv_noaa_eq_bmask", input_format);
	if (var_source == NULL)
		return(0);

	/*
	** search for the output variable
	*/

	result = (char *)bsearch((const void *)&(var->name),
	                         (const void *)base,
	                         (size_t)NUM_OUTPUT_NAMES,
	                         sizeof(char *),
	                         (int (*)(const void *, const void *))b_strcmp
	                        );

	if (result)
		name_number = (result - (char *)base) / (sizeof(char *));
	else
		return(0);


	/*
	** Initialize the return value and data pointer
	*/

	*converted_value = 0.0;
	bit_mask = (long HUGE *)input_buffer;
	data_mask = (long *)input_buffer+4;

	/*
	** switch on index and convert
	*/

	switch (name_number)
	{
	 	case 0:								/* CULTURAL EFFECTS  */
			culture = (char)((*(data_mask + 2) ) & 0x0007);
			*converted_value = (char)(culture);
		break;

	  case 1:								/* DAY   */
	    day = (short)((*(data_mask + 0) >> 27) & 0x001F);
			*converted_value = (short)(day);
		break;

		case 2:								/* DEPTH */
			depth = (float)((*(bit_mask + 1) >> 15) & 0x03FF);
			if (depth == 1023)
				depth = 0.0F;
			*converted_value = (float)(depth);
		break;

	  case 3:				/* FE_REGION - FLYNN-ENGDAHL REG*/
			fe_reg  = (short)((*(bit_mask + 1) >> 5) & 0x03FF);
			*converted_value = (short)(fe_reg);
		break;

		case 4:								/* HOUR  */
			hour = (short)((*(data_mask + 0) >> 22) & 0x001F);

			if (hour !=31)
			{
				hour = (short)hour;
				*converted_value = (short)(hour);
			}
		break;

		case 5:								/* FELT INTENSITY  */
			intens  = (short)((*(bit_mask + 0) >> 11) & 0x000F);
			*converted_value = (short)(intens);
		break;

		case 6:							/* LATITUDE */
			co_lat = (long)(*(bit_mask + 2) >> 13);
			if (co_lat > 90000L)
			{
				co_lat -= 90000L;
				co_lat *= -1;
			}
			else
				co_lat = 90000L - co_lat;

			*converted_value = co_lat / 1000.0;
		break;

		case 7:							/* LONGITUDE */
			east_lon = (long)((*(bit_mask + 3) >> 12) & 524287L);
			if (east_lon > 180000L)
			{
				*converted_value = (double)(360000L - east_lon);
				*converted_value /= 1000.0;
			}
			else
				*converted_value = (double)(east_lon) / 1000.0;
		break;

		case 8:				/* MAGNITUDE_MB - BODY WAVE MAGNITUDE*/
		  b_mag1  = (short)((*(bit_mask + 2) >> 7) & 0x003F);
		  b_mag2  = (short)((*(data_mask + 1) >> 27) & 0x001F);

		if(b_mag1)
			b_mag = (float)(25 *b_mag1 + b_mag2 -300)/100.0F;
		else
			b_mag = 0.0F;
		*converted_value = (float)(b_mag);
		break;

		case 9:				/* MAGNITUDE_ML - LOCAL  MAGNITUDE*/
			l_mag1  = (short)((*(bit_mask + 3) >> 6) & 0x003F);
			l_mag2  = (short)((*(data_mask + 2) >> 27) & 0x001F);

			if (l_mag1)
				l_mag = (float)(25*l_mag1 + l_mag2 -300)/100.0F;
			else
				l_mag = 0.0F;
			*converted_value = (float)(l_mag);
			break;

		case 10:			/* MAGNITUDE_MO - OTHER  MAGNITUDE*/
			o_mag1  = (short)(*(bit_mask + 3)  & 0x003F);
			o_mag2  = (short)((*(data_mask + 1) >> 18) & 0x001F);

			if (o_mag1)
				o_mag = (float)(25* o_mag1 + o_mag2 -300)/100.0F;
			else
				o_mag = 0.0F;

			*converted_value = (float)(o_mag);
			break;

		case 11:				/* MAGNITUDE_MS - S WAVE MAGNITUDE*/
			s_mag1  = (short)((*(bit_mask + 2) >> 1) & 0x003F);
			s_mag2  = (short)((*(data_mask + 1) >> 3) & 0x000F);

			if (s_mag1)
				ms = (float)((25*s_mag1 + s_mag2 *  5 -300) /10) /10.0F;
			else
				ms = 0.0F;
			*converted_value = (float)(ms);
		break;

		case 12:							/* MINUTE */
			minute = (short)((*(data_mask + 0) >> 16) & 0x003F);
			if (minute != 63)
				*converted_value = (short)(minute);
		break;

		case 13:							/* MONTH */
			time = (long)((*(bit_mask + 0) >> 15) & 0xFFFF);
			year =  (short)(time / 13);
			*converted_value = (short)(time -  year *13);
		break;

		case 14:				/* NO_STATION - NUMBER of STATIONS*/
			no_stat = (*(data_mask + 3) >> 17) & 0x03FFF;

			*converted_value = (long)(no_stat);
		break;

		case 15:							/* SECOND */

			second = (float)((*(data_mask + 0) >> 6) & 0x03FF);

			if (second != 1023)
			{
				if (second >600)
					second -=600;
				else second = second/10.0F;
			}
			if (second ==1023)
				second = 0.0F;
			*converted_value = (float)(second);
		break;

		case 16:							/* SOURCE CODE       */
			index   = (short)((*(data_mask + 3) >> 8) & 0x01FF);
			*converted_value = (int)(index);
		break;

		case 17:							/* YEAR */
			time = (*(bit_mask + 0) >> 15) & 0xFFFF;

			*converted_value = (short)((time / 13)-2100);
		break;

		case 18:							/* Z/H COMPONENT     */
			switch  ((int)((*(data_mask +3) >>5) & 0x0007))
			{
				case 1 :
					zh = 'H';
				break;

				case 2 :
					zh = 'Z';
				break;

			default: zh =  '\0';
			}

			*converted_value = (int)(zh);
			break;

		default:
			sprintf(input_buffer,"Problem with switch in NOAAEQ functions\n");

		return(0);

	} /* end switch */

	return(1);

} /* End cv_noaa_eq() */

/*
 * NAME:	cv_sea_flags
 *
 * PURPOSE:	This is a conversion function for the seismicity data from
 *			the University of Washington.
 *
 *			Conversions which are included:
 *
 *			Convert AType into				cultural
 *			            or 					ngdc_flags
 *			Convert depth_control_code into depth_control
 *
 * AUTHOR:
 *
 * USAGE:	cv_sea_flags(
 *				VARIABLE_PTR out_var,
 *				double *dummy,
 *				FORMAT_PTR input,
 *				FF_DATA_BUFFER input_buffer)
 *
 * COMMENTS:
 *
 * RETURNS:
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "cv_sea_flags"

int cv_sea_flags(VARIABLE_PTR out_var, double *dummy, FORMAT_PTR input, FF_DATA_BUFFER input_buffer)
{
	int i;
	char *ch_ptr;

	VARIABLE_PTR in_var = NULL;
	char *data_source = NULL;

	/* Initialize the variable destination (a double) to blanks */
	for (i = 8, ch_ptr = (char *)dummy; i; i--, ++ch_ptr)
		*ch_ptr = ' ';

	ch_ptr = (char *)dummy;

	if (strcmp(out_var->name, "cultural") == 0 ||
	    strcmp(out_var->name, "ngdc_flags") == 0
	   )
	{
		/* Check the input format to make sure that AType is present */
		in_var = ff_find_variable("AType", input);
		if (!in_var)
			return(0);

		/* Get the input variable */
		/* char buffer is a character array which is in the same location as
		the double where the converted variable is to be placed. This allows
		us to use the same space to return a string as would be used to
		return the double. NOTE THAT THIS CAUSES PROBLEMS IF YOU ARE
		TRYING TO GET MORE THAN 8 BYTES!!! */

		data_source = input_buffer + in_var->start_pos - 1;

		/* Event Felt, ch_ptr points to cultural */
		if (strcmp(out_var->name, "cultural") == 0)
		{
			if (data_source[0] == 'F')
			{
				*ch_ptr = 'F';
				return(1);
			}
		}
		else
		{		/* setting ngdc_flags */
			if (data_source[0] == 'L')	/* Volcanic, ch_ptr points to ngdc_flags */
				*(ch_ptr + 3) = 'V';

			if (data_source[0] == 'X' || data_source[0] == 'P')
	   			*(ch_ptr + 4) = 'E';
			return(1);
		}
	}

	if (strcmp(out_var->name, "depth_control") == 0)
	{
		/* Check the input format to make sure that depth_control is present */
		in_var = ff_find_variable("depth_control", input);
		if (!in_var)
			return(0);

		/* Get the input variable */
		data_source = input_buffer + in_var->start_pos - 1;

		if (data_source[0] == '*')
		{
				*ch_ptr = 'G';
				return(1);
		}

		if (data_source[0] == '$')
		{
				*ch_ptr = '?';
				return(1);
		}

		if (data_source[0] == '#')
		{
				*ch_ptr = '?';
				return(1);
		}
	}
	return(0);
}

/*
 * NAME:	cv_slu_flags
 *
 * PURPOSE:	This is a conversion function for the seismicity data from
 *			the St. Louis University.
 *
 *			Conversions which are included:
 *
 *			Convert slu_line2 into			non_tectonic
 *			Convert slu_line2 into			cultural
 *			Convert slu_line2 into			intensity
 *			Convert slu_line2 into			magnitude_ml
 *			Convert slu_line2 into			scale
 *			Convert slu_line2 into			ml_authority
 *
 * AUTHOR:
 *
 * USAGE:	cv_slu_flags(
 *				VARIABLE_PTR out_var,
 *				double        *dummy,
 *				FORMAT_PTR    input,
 *				FF_DATA_BUFFER   input_buffer)
 *
 * COMMENTS:
 *
 * RETURNS:	1 if successful, else 0
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "cv_slu_flags"

int cv_slu_flags(VARIABLE_PTR out_var, double *dummy, FORMAT_PTR input, FF_DATA_BUFFER input_buffer)
{
	VARIABLE_PTR 	in_var;
	FF_DATA_PTR 		data_source 	= NULL;
	int 			i;
	size_t length;
	char HUGE		*ch_ptr 	= NULL;
	char HUGE		*dummy_ch_ptr 	= NULL;
/*	char _temp[100]; */

	/* Initialize the variable destination (a double) to blanks */
	for(i = 8, dummy_ch_ptr = (char *)dummy; i; i--, ++dummy_ch_ptr) *dummy_ch_ptr = ' ';
	dummy_ch_ptr = (char *)dummy;

	/* Make the non-tectonic flag from the comment line */
	if(memStrcmp(out_var->name, "non_tectonic",NO_TAG) == 0){

		/* Check the input format to make sure that slu_line2 is present */
		in_var = ff_find_variable("slu_line2", input);
		if(!in_var){
			return(0);
		}

		length = (size_t)(in_var->end_pos - in_var->start_pos + 1);

		/* Get the input variable */
		data_source = (void HUGE *)(input_buffer + in_var->start_pos - 1);
		ch_ptr = ff_strnstr("BLAST", data_source, length);
		if(ch_ptr) *(dummy_ch_ptr) = 'E';
		else *(dummy_ch_ptr) = ' ';
		return (1);
	}

	/* Make the cultural flag from the comment line */
	if(memStrcmp(out_var->name, "cultural",NO_TAG) == 0){

		/* Check the input format to make sure that slu_line2 is present */
		in_var = ff_find_variable("slu_line2", input);
		if(!in_var){
			return(0);
		}

		length = (size_t)(in_var->end_pos - in_var->start_pos + 1);

		/* Get the input variable */
		data_source = (void HUGE *)(input_buffer + in_var->start_pos - 1);
		ch_ptr = ff_strnstr("FELT", data_source, length);
		if(ch_ptr) *(dummy_ch_ptr) = 'F';
		else *(dummy_ch_ptr) = ' ';
		return (1);
	}

	/* Make the intensity from the comment line */
	if(memStrcmp(out_var->name, "intensity",NO_TAG) == 0){

		/* Check the input format to make sure that slu_line2 is present */
		in_var = ff_find_variable("slu_line2", input);
		if(!in_var){
			return(0);
		}

		length = (size_t)(in_var->end_pos - in_var->start_pos + 1);

		/* Get the input variable */
		data_source = (void HUGE *)(input_buffer + in_var->start_pos - 1);
		ch_ptr = ff_strnstr("MM ", data_source, length);
		if(ch_ptr) *(dummy_ch_ptr) = *(ch_ptr + 3);
		else *(dummy_ch_ptr) = ' ';
		return (1);
	}

	/* Make the magnitude_ml from the comment line */
	/* There are a number of potential magnitudes in the comments which
	might be selected as ML. The most consistent selection, suggested by Bob
	Hermann 4/30/91, is to take any magnitude with FVM (FVMZ in the original
	catalog). These are magnitudes from the French Village vertical
	instrument. We select these first. If they do not exist we look for
	MD, duration magnitudes calculated by CERI. IF BOTH FVM and MD EXIST
	the MD is ignored.
	*/

	if(memStrcmp(out_var->name, "magnitude_ml",NO_TAG) == 0){

		/* Check the input format to make sure that slu_line2 is present */
		in_var = ff_find_variable("slu_line2", input);
		if(!in_var){
			return(0);
		}

		length = (size_t)(in_var->end_pos - in_var->start_pos + 1);

		/* Get the input variable */
		data_source = (void HUGE *)(input_buffer + in_var->start_pos - 1);

		/* Check for FVM */
		ch_ptr = ff_strnstr("FVM", data_source, length);
		if(ch_ptr){
			if(*(ch_ptr - 5) == ' ')*(dummy) = strtod(ch_ptr - 4, NULL);
			else *(dummy) = strtod(ch_ptr - 5, NULL);
			return (1);
		}


		ch_ptr = ff_strnstr("MD ", data_source, length);
		if(ch_ptr)
			*(dummy) = strtod(ch_ptr + 3, NULL);

		return (1);
	}

	/* Make the scale from the comment line, (SEE ML NOTES ABOVE) */
	if(memStrcmp(out_var->name, "scale",NO_TAG) == 0){

		/* Check the input format to make sure that slu_line2 is present */
		in_var = ff_find_variable("slu_line2", input);
		if(!in_var){
			return(0);
		}

		length = (size_t)(in_var->end_pos - in_var->start_pos + 1);

		/* Get the input variable */
		data_source = (void  HUGE *)(input_buffer + in_var->start_pos - 1);

		/* Check for FVM */
		ch_ptr = ff_strnstr("FVM", data_source, length);
		if(ch_ptr){
			*(dummy_ch_ptr) = 'L';
			*(dummy_ch_ptr + 1) = 'G';
			return (1);
		}

		ch_ptr = ff_strnstr("MD ", data_source, length);
		if(ch_ptr){
			*(dummy_ch_ptr) = 'D';
			*(dummy_ch_ptr + 1) = 'R';
		}
		return (1);
	}

	/* Make the authority from the comment line */
	if(memStrcmp(out_var->name, "ml_authority",NO_TAG) == 0){

		/* Check the input format to make sure that slu_line2 is present */
		in_var = ff_find_variable("slu_line2", input);
		if(!in_var){
			return(0);
		}

		length = (size_t)(in_var->end_pos - in_var->start_pos + 1);

		/* Get the input variable */
		data_source = (void HUGE *)(input_buffer + in_var->start_pos - 1);

		/* Check for FVM */
		ch_ptr = ff_strnstr("FVM", data_source, length);
		if(ch_ptr){
			*(dummy_ch_ptr) = 'S';
			*(dummy_ch_ptr + 1) = 'L';
			*(dummy_ch_ptr + 2) = 'M';
			return (1);
		}
		ch_ptr = ff_strnstr("MD ", data_source, length);
		if(ch_ptr){
			*(dummy_ch_ptr) = 'T';
			*(dummy_ch_ptr + 1) = 'E';
			*(dummy_ch_ptr + 2) = 'I';
		}
		return (1);
	}
	return(0);
}

/*
 * NAME:  	cv_ymd2ser
 *
 * PURPOSE:	This function converts a time given as year, month, day, hour,
 *			minute, second into a serial_day_1980 data with January 1, 1980 as 0,
 *			or into year_decimal.
 *
 * AUTHOR:	The code was modified from that given by Michael Covington,
 *			PC Tech Journal, December, 1985, PP. 136-142.
 *			Ted Habermann, NGDC, (303)497-6472, haber@mail.ngdc.noaa.gov
 *
 * USAGE:	cv_ymd2ser(VARIABLE_PTR out_var,
 *			double		*serial_day_1980,
 *			FORMAT_PTR	input_format,
 *			FF_DATA_BUFFER	input_buffer)
 *
 * COMMENTS:
 *
 * RETURNS:	1 if successful, else 0
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "cv_ymd2ser"

int cv_ymd2ser(VARIABLE_PTR out_var, double *serial_day_1980, FORMAT_PTR input_format, FF_DATA_BUFFER input_buffer)
{
	char scratch_buffer[256];
	VARIABLE_PTR in_var = NULL;
	long century = 0;
	long year = 0;
	long month = 0;
	long day = 0;
	long hour = 0;
	long minute = 0;
	int		jul_mon = 0;
	int		centuries = 0;
	int		real_leap_yrs = 0;
	int		correct = 0;
	long	jul_year = 0;
	double second = 0.0;
	double decimal, jul_num;

	short got_data = 0;
	int error;

	int variable_length;
	/* Define the Variables */

	/* Check input format to see if variables are present */

	in_var = ff_find_variable("century", input_format);	/* Get the century */
	if(in_var){
		++got_data;
		assert(FF_VAR_LENGTH(in_var) < sizeof(scratch_buffer));

		variable_length = min(sizeof(scratch_buffer) - 1, FF_VAR_LENGTH(in_var));

		memcpy(scratch_buffer, input_buffer + in_var->start_pos - 1, variable_length);
		scratch_buffer[FF_VAR_LENGTH(in_var)] = STR_END;
		error = ff_get_double(in_var, scratch_buffer, &decimal, input_format->type);
	  if (error)
	  	return(0);

		century = (short)decimal;
	}

	in_var = ff_find_variable("year", input_format);	/* Get the year */
	if(in_var){
		++got_data;
		assert(FF_VAR_LENGTH(in_var) < sizeof(scratch_buffer));

		variable_length = min(sizeof(scratch_buffer) - 1, FF_VAR_LENGTH(in_var));

		memcpy(scratch_buffer, input_buffer + in_var->start_pos - 1, variable_length);
		scratch_buffer[FF_VAR_LENGTH(in_var)] = STR_END;
		error = ff_get_double(in_var, scratch_buffer, &decimal, input_format->type);
	  if (error)
	  	return(0);

		year = (short)decimal;
		if (year < 100)
		{
			if (century == 0)
				; /* no change */
			else if (century > 0)
				year += century * 100;
			else
				year = century * 100 - year;
		}
	}
	else
	{		/* Year not found */
		in_var = ff_find_variable("century_and_year", input_format);	/* Get the century_and_year */
		if(in_var){
			++got_data;
			assert(FF_VAR_LENGTH(in_var) < sizeof(scratch_buffer));

			variable_length = min(sizeof(scratch_buffer) - 1, FF_VAR_LENGTH(in_var));

			memcpy(scratch_buffer, input_buffer + in_var->start_pos - 1, variable_length);
			scratch_buffer[FF_VAR_LENGTH(in_var)] = STR_END;
			error = ff_get_double(in_var, scratch_buffer, &decimal, input_format->type);
		  if (error)
		  	return(0);

			year = (short)decimal;
		}
		else	return(0);	/* Can't Do Conversion */
	}

	in_var = ff_find_variable("month", input_format);	/* Get the month */
	if(in_var){
		++got_data;
		assert(FF_VAR_LENGTH(in_var) < sizeof(scratch_buffer));

		variable_length = min(sizeof(scratch_buffer) - 1, FF_VAR_LENGTH(in_var));

		memcpy(scratch_buffer, input_buffer + in_var->start_pos - 1, variable_length);
		scratch_buffer[FF_VAR_LENGTH(in_var)] = STR_END;
		error = ff_get_double(in_var, scratch_buffer, &decimal, input_format->type);
	  if (error)
	  	return(0);

		month = (short)decimal;
	}
	in_var = ff_find_variable("day", input_format);	/* Get the day */
	if(in_var){
		++got_data;
		assert(FF_VAR_LENGTH(in_var) < sizeof(scratch_buffer));

		variable_length = min(sizeof(scratch_buffer) - 1, FF_VAR_LENGTH(in_var));

		memcpy(scratch_buffer, input_buffer + in_var->start_pos - 1, variable_length);
		scratch_buffer[FF_VAR_LENGTH(in_var)] = STR_END;
		error = ff_get_double(in_var, scratch_buffer, &decimal, input_format->type);
	  if (error)
	  	return(0);

		day = (short)decimal;
	}

	in_var = ff_find_variable("hour", input_format);	/* Get the hour */
	if(in_var){
		++got_data;
		assert(FF_VAR_LENGTH(in_var) < sizeof(scratch_buffer));

		variable_length = min(sizeof(scratch_buffer) - 1, FF_VAR_LENGTH(in_var));

		memcpy(scratch_buffer, input_buffer + in_var->start_pos - 1, variable_length);
		scratch_buffer[FF_VAR_LENGTH(in_var)] = STR_END;
		error = ff_get_double(in_var, scratch_buffer, &decimal, input_format->type);
	  if (error)
	  	return(0);

		hour = (short)decimal;
	}

	in_var = ff_find_variable("minute", input_format);	/* Get the minute */
	if(in_var){
		++got_data;
		assert(FF_VAR_LENGTH(in_var) < sizeof(scratch_buffer));

		variable_length = min(sizeof(scratch_buffer) - 1, FF_VAR_LENGTH(in_var));

		memcpy(scratch_buffer, input_buffer + in_var->start_pos - 1, variable_length);
		scratch_buffer[FF_VAR_LENGTH(in_var)] = STR_END;
		error = ff_get_double(in_var, scratch_buffer, &decimal, input_format->type);
	  if (error)
	  	return(0);

		minute = (short)decimal;
	}

	in_var = ff_find_variable("second", input_format);	/* Get the second */
	if(in_var){
		++got_data;
		assert(FF_VAR_LENGTH(in_var) < sizeof(scratch_buffer));

		variable_length = min(sizeof(scratch_buffer) - 1, FF_VAR_LENGTH(in_var));

		memcpy(scratch_buffer, input_buffer + in_var->start_pos - 1, variable_length);
		scratch_buffer[FF_VAR_LENGTH(in_var)] = STR_END;
		error = ff_get_double(in_var, scratch_buffer, &second, input_format->type);
	  if (error)
	  	return(0);
	}

	/* Several of the time variables cannot have values of 0 (i.e. month and day). If these
		variables do have these values, it is assumed that they are unspecified. For example,
		we know that an event occurred during a specific year (month), but do not know
		any more. This case can also occur if we know the year, month, and day,
		but not the time of day. In these cases, all unknown variables are set to 1. */

	if (   day == 0
	    || (hour == 0 && minute == 0 && second == 0)
	   )
	{
		if (century == 0) century = 1;
		if (year == 0) year = 1;
		if (month == 0) month = 1;
		if (day == 0) day = 1;
		hour = 1;
		minute = 1;
		second = 1.0;
	}

	decimal  = second / 60.0;
	decimal += minute;
	decimal /= 60.0;
	decimal += hour;
	decimal /= 24.0;

 	/* Calculate decimal year if that is the output variable name */
 	if (!strcmp(out_var->name, "year_decimal"))
 	{
		short days_per_month[13] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};

 		day += days_per_month[month - 1];
 		decimal += day - 1;

 		/* Leap years */
		if ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)
		{
			if (month >= 3)
 				decimal += 1.0;
			*serial_day_1980 = year + ((year >= 0) ? 1 : -1) * (decimal / 366.0);
 		}
 		else
 			*serial_day_1980 = year + ((year >= 0) ? 1 : -1) * (decimal / 365.0);

 		return(got_data);
 	}

	if (0 < month && month < 3)
	{
		month += 12;
		year  -= 1;
	}

	jul_year = (long)(365.25 * year + 0.1);

	jul_mon = (int)(30.6001 * (month + 1) - 63);

	jul_num = jul_year + jul_mon + day - 723182.0;

	/* convert to Gregorian calendar */
	centuries = (int)floor(year / 100.0); /* The number of centuries counted (including the year 0)*/
	real_leap_yrs = (int)floor(centuries / 4.0); /* The number of centuries that were leap years (divisible by 400) */
	correct = 2 - centuries + real_leap_yrs;
	*serial_day_1980 = jul_num + correct + decimal;

	return(got_data);
}


/*
 * NAME:  	cv_ymd2ipe
 *
 * PURPOSE:	This function converts a time given as year, month, day, hour,
 *			minute, second into a date used by the Institute of the Physics
 *			of the Earth in Moscow. The time is in minutes A.D.
 *
 * AUTHOR:	Ted Habermann, NGDC, (303)497-6472, haber@mail.ngdc.noaa.gov
 *
 * USAGE:	cv_ymd2ipe(
 *				VARIABLE_PTR out_var,
 *				double        *serial_day_1980,
 *				FORMAT        *input_format,
 *				char          *input_buffer)
 *
 * COMMENTS:
 *
 * RETURNS:	1 if successful, else 0
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */

#undef ROUTINE_NAME
#define ROUTINE_NAME "cv_ymd2ipe"

int cv_ymd2ipe(VARIABLE_PTR out_var, double *serial_day_1980, FORMAT_PTR input_format, FF_DATA_BUFFER input_buffer)
{

	/* This function uses the cv_ymd2ser function to create a serial_day_1980 date in
	days after January, 1980 and then converts that to the IPE Date.
	*/

	int error;

	error = cv_ymd2ser(out_var, serial_day_1980, input_format, input_buffer);

	if(error == 0) return(0);

	/* There are 1440 minutes in a day, so convert serial_day_1980 into minutes */
	*serial_day_1980 *= 1440.0;

	/* 1040874840 = number of minutes to end of 1979 */

	*serial_day_1980 += 1040874840;

	return(1);
}

/*
 * NAME:  	cv_ser2ipe
 *
 * PURPOSE:	This function converts a time given as serial_day_1980 date (1980 = 0)
 *			into a date used by the Institute of the Physics
 *			of the Earth in Moscow. The time is in minutes A.D.
 *
 * AUTHOR:	Ted Habermann, NGDC, (303)497-6472, haber@mail.ngdc.noaa.gov
 *
 * USAGE:	cv_ser2ipe(
 *				VARIABLE_PTR out_var,
 *				double      *serial_day_1980,
 *				FORMAT      *input_format,
 *				FF_DATA_BUFFER	input_buffer)
 *
 * COMMENTS:
 *
 * RETURNS:	1 if successful, else 0
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */

#undef ROUTINE_NAME
#define ROUTINE_NAME "cv_ser2ipe"

int cv_ser2ipe(VARIABLE_PTR not_used, double *serial_day_1980, FORMAT_PTR input_format, FF_DATA_BUFFER input_buffer)
{
	char scratch_buffer[256];
	VARIABLE_PTR in_var = NULL;
	int error;
	int variable_length;

	(void)not_used; /* silence compiler warning */

	/* Find the serial_day_1980 variable */
	in_var = ff_find_variable("serial_day_1980", input_format);	/* Get the serial date */

	if(!in_var){
		/* Check for old variable name */
		in_var = ff_find_variable("serial", input_format);
		if(!in_var)	return (0);
	}

	if(!in_var) return (0);

	assert(FF_VAR_LENGTH(in_var) < sizeof(scratch_buffer));

	variable_length = min(sizeof(scratch_buffer) - 1, FF_VAR_LENGTH(in_var));

	memcpy(scratch_buffer, input_buffer + in_var->start_pos - 1, variable_length);
	scratch_buffer[FF_VAR_LENGTH(in_var)] = STR_END;
	error = ff_get_double(in_var, scratch_buffer, serial_day_1980, input_format->type);
  if (error)
  	return(0);

	/* There are 1440 minutes in a day, so convert serial_day_1980 into minutes */
	*serial_day_1980 *= 1440.0;

	/* 1040874840 = number of minutes to end of 1979 */

	*serial_day_1980 += 1040874840;

	/* Truncate to avoid rounding */
	*serial_day_1980 = (long)*serial_day_1980;

	return(1);
}

/*
 * NAME:  	cv_ipe2ser
 *
 * PURPOSE:	This function converts a date used by the Institute of the
 * 			Physics	of the Earth in Moscow into a time given as serial_day_1980
 *			date (1980 = 0).
 *
 * AUTHOR:	Ted Habermann, NGDC, (303)497-6472, haber@mail.ngdc.noaa.gov
 *
 * USAGE:	cv_ipe2ser(
 *				VARIABLE_PTR out_var,
 *				double		*serial_day_1980,
 *				FORMAT_PTR	input_format,
 *				FF_DATA_BUFFER	input_buffer)
 *
 * COMMENTS:
 *
 * RETURNS:	1 if successful, else 0
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */

#undef ROUTINE_NAME
#define ROUTINE_NAME "cv_ipe2ser"

int cv_ipe2ser(VARIABLE_PTR not_used, double *serial_day_1980, FORMAT_PTR input_format, FF_DATA_BUFFER input_buffer)
{
	char scratch_buffer[256];
	VARIABLE_PTR in_var = NULL;
	int error;
	int variable_length;

	(void)not_used; /* silence compiler warning */

	/* Find the ipe_date variable */
	in_var = ff_find_variable("ipe_date", input_format);	/* Get the ipe date */

	if(!in_var) return (0);

	assert(FF_VAR_LENGTH(in_var) < sizeof(scratch_buffer));

	variable_length = min(sizeof(scratch_buffer) - 1, FF_VAR_LENGTH(in_var));

	memcpy(scratch_buffer, input_buffer + in_var->start_pos - 1, variable_length);
	scratch_buffer[FF_VAR_LENGTH(in_var)] = STR_END;
	error = ff_get_double(in_var, scratch_buffer, serial_day_1980, input_format->type);
	if (error)
		return(0);

	/* 1040874840 = number of minutes to end of 1979 */

	*serial_day_1980 -= 1040874840;

	/* There are 1440 minutes in a day, so convert serial_day_1980 into minutes */
	*serial_day_1980 /= 1440.0;

	return(1);
}

/*
 * NAME:  	cv_time_string
 *
 * PURPOSE:	This function converts bewteen various string representations
 *			of times. The presently supported strings are:
 *				hhmmss					(time_hhmmss)
 *				hh:mm:ss				(time_hh:mm:ss)
 *			These also work for strings in which seconds are not present.
 *
 *
 * AUTHOR:	Ted Habermann, NGDC, (303)497-6472, haber@mail.ngdc.noaa.gov
 *			modified by Terry Miller
 *
 * USAGE:	cv_time_string(
 *				VARIABLE_PTR out_var,
 *				double        *serial_day_1980,
 *				FORMAT        *input_format,
 *				char          *input_buffer)
 *
 * COMMENTS:
 *
 * RETURNS:	1 if successful, else 0
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */

#undef ROUTINE_NAME
#define ROUTINE_NAME "cv_time_string"

int cv_time_string(VARIABLE_PTR out_var, double *output, FORMAT_PTR input_format, FF_DATA_BUFFER input_buffer)
{
	char scratch_buffer[256];
	VARIABLE_PTR in_var = NULL;
	char *time_names[] = {
		  "time_h:m:s",
		  "time_hhmmss",

		/* DON'T EVEN THINK OF ADDING SOMETHING HERE WITHOUT INCREASING
		THE number_of_names. */

		NULL,
	};
	unsigned int number_of_names = 2;

	char *ch_ptr = NULL;

	int variable_length = 0;
	int string_length = 0;
	unsigned int string_number = 0;

	char hour[4];
	char minute[4];
	char second[5];

	*hour = STR_END;
	*minute = STR_END;
	*second = STR_END;

	assert(sizeof(*output) == sizeof(double));
	memset((void *)output, ' ', sizeof(*output));

	in_var = NULL;

	/* Search the Input Format to Find the Time String */
	/* The first time this function is called it is an experimental call, to
	see if the conversion can be done. If this call succeeds, the desired
	conversion variable is added to the input format. The next time it is
	called, to do the actual conversions, this variable is there as a FFV_CONVERT
	variable. We are not interested in finding the convert variable here, we
	need the input variable. */
	while((((in_var = ff_find_variable(time_names[string_number], input_format)) == NULL) &&
		time_names[string_number]) ||
		in_var->type == FFV_CONVERT)
		++string_number;

	if(string_number < number_of_names){		/* Input variable is data string */

		/* Copy The String to buffer */
		assert(FF_VAR_LENGTH(in_var) < sizeof(scratch_buffer));

		variable_length = min(sizeof(scratch_buffer) - 1, FF_VAR_LENGTH(in_var));

		memcpy(scratch_buffer, input_buffer + in_var->start_pos - 1, variable_length);
		scratch_buffer[variable_length] = STR_END;

		/* Skip leading blanks */
		ch_ptr = scratch_buffer;
		while(*ch_ptr == ' ')
			++ch_ptr;

		string_length = strlen(ch_ptr);


		switch(string_number)
		{
		case 0:		/* Time String is hh:mm:ss */
		 	snprintf(hour, 4, "%02d", atoi(strtok(ch_ptr, "/:|, ")));
		 	snprintf(minute, 4, "%02d", atoi(strtok(NULL, "/:|, ")));
			if (string_length < 6)
				*second = STR_END;
			else
		 		memStrcpy(second,strtok(NULL, "/:|, "),NO_TAG);
			break;

		case 1:		/* Time String is hhmmss */
			if(string_length == 5){
				memMemmove(ch_ptr + 1, ch_ptr, 6,NO_TAG);
				*ch_ptr = '0';
			}
		 	memMemmove(hour ,ch_ptr, 2,NO_TAG);
		 	memMemmove(minute,ch_ptr + 2, 2,NO_TAG);
		 	memMemmove(second  ,ch_ptr + 4, 2,NO_TAG);
			*(hour + 2) = STR_END;
			*(minute + 2) = STR_END;
			*(second + 2) = STR_END;
			break;

			default:
				assert(!ERR_SWITCH_DEFAULT);
				err_push(ERR_SWITCH_DEFAULT, "%s, %s:%d", ROUTINE_NAME, os_path_return_name(__FILE__), __LINE__);
				return(0);
		}
	}
	else {		/* Input is data, not a string */
		in_var = ff_find_variable("hour", input_format);
		if(in_var)ff_get_string(in_var, input_buffer + in_var->start_pos - 1, hour, input_format->type);

		in_var = ff_find_variable("minute", input_format);
		if(in_var)ff_get_string(in_var, input_buffer + in_var->start_pos - 1, minute, input_format->type);

		in_var = ff_find_variable("second", input_format);
		if(in_var)ff_get_string(in_var, input_buffer + in_var->start_pos - 1, second, input_format->type);
	}
	if(!in_var) return(0);

	/* Now the input data is known, Determine the output type */

	string_number = 0;
	while (strcmp(out_var->name, time_names[string_number]))
		++string_number;

	ch_ptr = (char *)output;

	switch(string_number)
	{
	case 0:		/* Time String is hh:mm:ss */
	 	snprintf(ch_ptr, sizeof(output), "%s:%s:%s", hour, minute, second);
		break;

	case 1:		/* Date String is hhmmss */
		if(*(second + 1) == STR_END){
			*(second + 1) = *second;
			*(second + 2) = STR_END;
			*second = '0';
		}
		if(*(minute + 1) == STR_END){
			*(minute + 1) = *minute;
			*(minute + 2) = STR_END;
			*minute = '0';
		}

	 	sprintf(ch_ptr,"%s%s%s", hour, minute, second);
		break;

		default:
			assert(!ERR_SWITCH_DEFAULT);
			err_push(ERR_SWITCH_DEFAULT, "%s, %s:%d", ROUTINE_NAME, os_path_return_name(__FILE__), __LINE__);
			return(0);
	}

	/* convert leading zero's to spaces */

	for (ch_ptr = (char *)output; *ch_ptr == '0' /* zero */; ch_ptr++)
		*ch_ptr = ' ';

	return(1);
}

/*
 * NAME:	cv_geo44tim
 *
 * PURPOSE:	This file contains the function which is used to convert the time in
 *			arbitrary seconds to a time offset for the GEO44 data. The offset is
 *			calculated from the time which is in the header record.
 *
 * AUTHOR:	T. Habermann (303) 497-6472, haber@mail.ngdc.noaa.gov
 *
 * USAGE:	Time is converted in arbitrary seconds to a time offset for the
 *			GEO44 data. The offset is calculated from the time which is in
 *			the header record.
 *
 *			Header records are identified by a 1000 in the gravity_uncertainty
 *			variable.
 *
 *			If a header record is being processed, change variables:
 *
 *				The uncertainty needs to be set to 10 so that the integer
 *				representation becomes 1000.
 *
 *				The gravity anomaly needs to be reduced by a factor of 100 so
 *				that its integer value is correct.
 *
 *			cv_geo44tim(
 *				VARIABLE_PTR out_var,
 *				double        *offset,
 *				FORMAT_PTR    input,
 * 				FF_DATA_BUFFER   input_buffer)
 *
 * COMMENTS:
 *
 * RETURNS:
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */

#undef ROUTINE_NAME
#define ROUTINE_NAME "cv_geo44tim"

int cv_geo44tim
	(
	 VARIABLE_PTR not_used,
	 double *offset,
	 FORMAT_PTR input,
	 FF_DATA_BUFFER input_buffer
	)
{
	VARIABLE_PTR 	var			= NULL;
	FF_DATA_PTR 		data_source = NULL;

	char HUGE 			*decimal_pt	= NULL;

	char 				ten[] 		= {"  10.00"};

	static double 		start_time 	= 0.0;
	double 				d_value		= 0.0;

	(void)not_used; /* silence compiler warning */

	/* Get the value of the time */
	var = ff_find_variable("time_seconds", input);
	data_source = (void HUGE *)(input_buffer + var->start_pos - 1);
	if (ff_get_double(var, data_source, &d_value, input->type))
		return(0);

	/* Get the value of the key */

	var = ff_find_variable("gravity_uncertainty", input);
	if (!var)
		return(0);

	data_source = (void HUGE *)(input_buffer + var->start_pos - 1);

	if (strncmp(data_source, "1000", 4))
	{		/* Not header */
		*offset = (d_value - start_time) / 0.489;
		return(1);
	}

	/* Deal with header */

	start_time = d_value;
	*offset = 0.0;

	var = ff_find_variable("gravity_uncertainty", input);
	memMemcpy(input_buffer + var->start_pos - 1, ten, 7,NO_TAG);

	var = ff_find_variable("gravity_anom", input);

	decimal_pt = strchr(input_buffer + var->start_pos - 1, '.');

	memMemmove(decimal_pt + 1, decimal_pt - 2, 2,NO_TAG);
	memMemmove(decimal_pt - 2, decimal_pt - 4, 2,NO_TAG);
	*(decimal_pt - 4) = *(decimal_pt - 3) = ' ';
	if (*(decimal_pt + 1) == ' ')
		*(decimal_pt + 1) = '0';

	return(1);
}


