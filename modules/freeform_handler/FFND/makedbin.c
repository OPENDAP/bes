/*
 * FILENAME:  makedbin.c
 *              
 * CONTAINS:  
 * Public functions:
 *
 * db_destroy
 * db_init
 * parse_command_line
 *
 * Private functions:
 *
 * ask_change_input_img_format
 * change_input_img_format
 * check_file_write_access
 * check_vars_types_for_keywords
 * cmp_array_conduit
 * create_array_conduit_list
 * db_make
 * destroy_format_data_list
 * dynamic_change_input_data_format
 * dynamic_change_output_data_format
 * dynamic_update_format
 * dynamic_update_var
 * get_default_var
 * is_redirecting_stdin
 * merge_redundant_conduits
 * 
 * option_B
 * option_C
 * option_F_
 * option_I__
 * option_M_
 * option_O__
 * option_P
 * option_Q
 * option_V
 * 
 * set_format_mappings
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

/*
 * NAME:        db_make
 *
 * PURPOSE:     To create a data bin
 *
 * USAGE:       db_make(char *title)
 *
 * RETURNS:     A pointer to the new data bin, else NULL
 *
 * DESCRIPTION: This function allocates memory for and initializes a data bin.
 *
 * AUTHOR:      T. Habermann, NGDC, (303) 497 - 6472, haber@ngdc.noaa.gov
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

#undef ROUTINE_NAME
#define ROUTINE_NAME "db_make"

/*static*/ DATA_BIN_PTR db_make(char *title)
{
	DATA_BIN_PTR dbin;

	dbin = (DATA_BIN_PTR)memMalloc(sizeof(DATA_BIN), "dbin");

	if (dbin == NULL)
	{
		err_push(ERR_MEM_LACK, "Data Bin");
		return(NULL);
	}

	/* Initialize data_bin */

#ifdef FF_CHK_ADDR
	dbin->check_address = (void*)dbin;
#endif

	if (title)
	{
		dbin->title = (char *)memStrdup(title, "dbin->title"); /* (char *) for Think C -rf01 */
		if (dbin->title == NULL)
		{
			err_push(ERR_MEM_LACK, "Data Bin Title");
			memFree(dbin, "dbin");
			return(NULL);
		}
	}
	else
		dbin->title = NULL;

	dbin->table_list = NULL;
	
	dbin->array_conduit_list = NULL;

	dbin->eqn_info = NULL;

	return(dbin);
}

/* The following function names have the first letter of an option
   flag.  Single letter option flags have simple functions below (e.g., -b),
   but multiple letter options flags have more complex functions with
   additional levels of switch and case statements (e.g., -ift).
   
   Currently, I have broken only one level of switch and cases into "dispatch
   center" functions.  Presumably, when certain options become more complex
   (e.g., -i*, which could add another letter for header or record format,
   and title, such as -ihft) then I will have to use function calls for those.
   
   In naming these functions, the number of underscores following the first
   letter of the option flag indicates the number of possible additional
   characters to follow.  For each additional character is another level of
   switches.
*/

#undef ROUTINE_NAME
#define ROUTINE_NAME "parse_command_line"

static int option_B(char *argv[], FF_STD_ARGS_PTR std_args, int *i)
{
	int error = 0;

	switch (toupper(argv[*i][2])) /* -b? */
	{
		case STR_END :
			(*i)++;
	
			if (!argv[*i] || argv[*i][0] == '-')
				error = err_push(ERR_PARAM_VALUE, "Buffer size must be positive (following %s)", argv[*i - 1]);
			else
			{
				char *endptr = NULL;

				errno = 0;
				std_args->cache_size = strtol(argv[*i], &endptr, 10);
				if (errno == ERANGE)
					error = err_push(errno, argv[*i]);
				
				if (ok_strlen(endptr))
					error = err_push(ERR_PARAM_VALUE, "Numeric conversion of \"%s\" stopped at \"%s\"", argv[*i], endptr);
			}
		break;
	
		default:
			error = err_push(ERR_UNKNOWN_OPTION, "==> %s <==", argv[*i]);
		break;
	}
	
	return(error);
}

static int option_C(char *argv[], FF_STD_ARGS_PTR std_args, int *i)
{
	int error = 0;

	switch (toupper(argv[*i][2])) /* -c? */
	{
		case STR_END :
			(*i)++;

			if (!argv[*i])
				error = err_push(ERR_PARAM_VALUE, "Expecting a value for count (following %s)", argv[*i - 1]);
			else
			{
				char *endptr = NULL;

				errno = 0;
				std_args->records_to_read = strtol(argv[*i], &endptr, 10);
				if (errno == ERANGE)
					error = err_push(errno, argv[*i]);
			
				if (ok_strlen(endptr))
					error = err_push(ERR_PARAM_VALUE, "Numeric conversion of \"%s\" stopped at \"%s\"", argv[*i], endptr);
			}
		break;
							
		default:
			error = err_push(ERR_UNKNOWN_OPTION, "==> %s <==", argv[*i]);
		break;
	}
	
	return(error);
}

static int option_D(char *argv[], FF_STD_ARGS_PTR std_args, int *i)
{
	int error = 0;

	switch (toupper(argv[*i][2])) /* -c? */
	{
		case STR_END :
			(*i)++;

			if (!argv[*i])
				error = err_push(ERR_PARAM_VALUE, "Expecting a file name (following %s)", argv[*i - 1]);
			else
			{
				if (!os_file_exist(argv[*i]))
					error = err_push(ERR_OPEN_FILE, argv[*i]);

				std_args->input_file = argv[*i];
				std_args->user.is_stdin_redirected = 0;
			}
		break;
							
		default:
			error = err_push(ERR_UNKNOWN_OPTION, "==> %s <==", argv[*i]);
		break;
	}
	
	return(error);
}

static int option_E_(char *argv[], FF_STD_ARGS_PTR std_args, int *i)
{
	int error = 0;

	switch (toupper(argv[*i][2])) /* -e? */
	{
		case 'P' : /* -ep? */
			switch (toupper(argv[*i][3]))
			{
				case STR_END:
					std_args->error_prompt = FALSE;
				break;
							
				default:
					error = err_push(ERR_UNKNOWN_OPTION, "==> %s <==", argv[*i]);
				break;
			}
		break;
							
		case 'L' : /* -el? */
			switch (toupper(argv[*i][3]))
			{
				case STR_END:
					(*i)++;

					if (!argv[*i])
						error = err_push(ERR_PARAM_VALUE, "Need a name for error log file (following %s)", argv[*i - 1]);
					else
					{
						FILE *fp = NULL;

						std_args->error_log = argv[*i];

						fp = fopen(std_args->error_log, "w");
						if (fp)
						{
							(void) fclose(fp);
							(void) remove(std_args->error_log);
						}
						else
							error = err_push(ERR_CREATE_FILE, std_args->error_log);
					}
				break;
							
				default:
					error = err_push(ERR_UNKNOWN_OPTION, "==> %s <==", argv[*i]);
				break;
			}
		break;
							
		default:
			error = err_push(ERR_UNKNOWN_OPTION, "==> %s <==", argv[*i]);
		break;
	}
	
	return(error);
}

static int option_F_(char *argv[], FF_STD_ARGS_PTR std_args, int *i)
{
	int error = 0;

	switch (toupper(argv[*i][2])) /* -f? */
	{
		case STR_END : /* single format file */
			(*i)++;
			/* Check File */

			if (!argv[*i])
				error = err_push(ERR_PARAM_VALUE, "Need a name for format file (following %s)", argv[*i - 1]);
			else
			{
				if (!os_file_exist(argv[*i]))
					error = err_push(ERR_OPEN_FILE, argv[*i]);
	
				std_args->output_format_file = argv[*i];
				std_args->input_format_file = argv[*i];

				std_args->user.format_file = 1;
			}
		break;
							
		case 'T' : /* single format title */
			switch (toupper(argv[*i][3])) /* -ft? */
			{
				case STR_END :
					(*i)++;

					if (!argv[*i])
						error = err_push(ERR_PARAM_VALUE, "Need a title for formats (following %s)", argv[*i - 1]);
					else
					{
						std_args->input_format_title = argv[*i];
						std_args->output_format_title = argv[*i];

						std_args->user.format_title = 1;
					}
				break;
								
				default :
					error = err_push(ERR_UNKNOWN_OPTION, "==> %s <==", argv[*i]);
				break;
			}
		break;
							
		default :
			error = err_push(ERR_UNKNOWN_OPTION, "==> %s <==", argv[*i]);
		break;
	} /* switch on second letter of -f flag */
	
	return(error);
}

static int option_G(char *argv[], FF_STD_ARGS_PTR std_args, int *i)
{
	int error = 0;

	switch (toupper(argv[*i][2])) /* -g? */
	{
		case STR_END :
			(*i)++;

			if (!argv[*i])
				error = err_push(ERR_PARAM_VALUE, "Need a value for grid size(s) (following %s)", argv[*i - 1]);
			else
			{
				char *endptr = NULL;

				errno = 0;
				std_args->SDE_grid_size = strtod(argv[*i], &endptr);
				if (errno == ERANGE)
					error = err_push(errno, argv[*i]);
			
				if (ok_strlen(endptr))
					error = err_push(ERR_PARAM_VALUE, "Numeric conversion of \"%s\" stopped at \"%s\"", argv[*i], endptr);

				if (argv[*i + 1] && argv[*i + 1][0] != '-')
				{
					++(*i);

					errno = 0;
					std_args->SDE_grid_size2 = strtod(argv[*i], &endptr);
					if (errno == ERANGE)
						error = err_push(errno, argv[*i]);
				
					if (ok_strlen(endptr))
						error = err_push(ERR_PARAM_VALUE, "Numeric conversion of \"%s\" stopped at \"%s\"", argv[*i], endptr);
				}

				if (argv[*i + 1] && argv[*i + 1][0] != '-')
				{
					++(*i);

					errno = 0;
					std_args->SDE_grid_size3 = strtod(argv[*i], &endptr);
					if (errno == ERANGE)
						error = err_push(errno, argv[*i]);
				
					if (ok_strlen(endptr))
						error = err_push(ERR_PARAM_VALUE, "Numeric conversion of \"%s\" stopped at \"%s\"", argv[*i], endptr);
				}
			}
		break;

		default:
			error = err_push(ERR_UNKNOWN_OPTION, "==> %s <==", argv[*i]);
		break;
	}
	
	return(error);
}

static int option_I__(char *argv[], FF_STD_ARGS_PTR std_args, int *i)
{
	int error = 0;

	switch (toupper(argv[*i][2])) /* -i? */
	{
		case STR_END:
			error = err_push(ERR_UNKNOWN_OPTION, "==> %s <==", argv[*i]);
		break;
							
		case 'F' :
			switch (toupper(argv[*i][3])) /* -if? */
			{
				case STR_END : /* input format file */
					(*i)++;
					/* Check File */

					if (!argv[*i])
							error = err_push(ERR_PARAM_VALUE, "Need a name for input format file (following %s)", argv[*i - 1]);
					else
					{
						if (!os_file_exist(argv[*i]))
							error = err_push(ERR_OPEN_FILE, argv[*i]);
	
						std_args->input_format_file = argv[*i];
					}
				break;
								
				case 'T' : /* input format title */
					switch (toupper(argv[*i][4]))
					{
						case STR_END :
							(*i)++;

							if (!argv[*i])
								error = err_push(ERR_PARAM_VALUE, "Need a title for input format (following %s)", argv[*i - 1]);
							else
								std_args->input_format_title = argv[*i];
						break;
							
						default:
							error = err_push(ERR_UNKNOWN_OPTION, "==> %s <==", argv[*i]);
						break;
					}
				break;
									
				default :
					error = err_push(ERR_UNKNOWN_OPTION, "==> %s <==", argv[*i]);
				break;
			} /* switch on third letter of -I flag */
		break;

		default :
			error = err_push(ERR_UNKNOWN_OPTION, "==> %s <==", argv[*i]);
		break;
	} /* switch on second letter of -I flag */
	
	return(error);
}

static int option_M_(char *argv[], FF_STD_ARGS_PTR std_args, int *i)
{
	int error = 0;

	switch (toupper(argv[*i][2])) /* -m? */
	{
		case STR_END :
			(*i)++;
								
			if (!argv[*i] || argv[*i][0] == '-')
				error = err_push(ERR_PARAM_VALUE, "Need a positive value for maxbins (following %s)", argv[*i - 1]);
			else
			{
				char *endptr = NULL;

				errno = 0;
				std_args->cv_maxbins = (int)strtod(argv[*i], &endptr);
				if (errno == ERANGE)
					error = err_push(errno, argv[*i]);
			
				if (ok_strlen(endptr))
					error = err_push(ERR_PARAM_VALUE, "Numeric conversion of \"%s\" stopped at \"%s\"", argv[*i], endptr);
			}
		break;
							
		case 'M' :
			switch (toupper(argv[*i][3])) /* -mm? */
			{
				case STR_END:
					std_args->cv_maxmin_only = TRUE;
				break;

				default:
					error = err_push(ERR_UNKNOWN_OPTION, "==> %s <==", argv[*i]);
				break;
			}
		break;
							
		default:
			error = err_push(ERR_UNKNOWN_OPTION, "==> %s <==", argv[*i]);
		break;
	} /* switch on second letter of -m flag */
	
	return(error);
}

static int option_O__(char *argv[], FF_STD_ARGS_PTR std_args, int *i)
{
	int error = 0;

	switch (toupper(argv[*i][2])) /* -o? */
	{
		case STR_END:
			(*i)++;

			if (!argv[*i])
				error = err_push(ERR_PARAM_VALUE, "Need a name for the output file (following %s).", argv[*i - 1]);
			else
			{
				std_args->output_file = argv[*i];
				std_args->user.is_stdout_redirected = 0;
			}
		break;
							
		case 'D' :
		
			switch (toupper(argv[*i][3])) /* -od? */
			{
				case STR_END : /* output directory for variable summary files */
					(*i)++;
					
					if (!argv[*i])
						error = err_push(ERR_PARAM_VALUE, "Expecting a directory for variable summary files (following %s)", argv[*i - 1]);
					else
						std_args->cv_list_file_dir = argv[*i];
				break;
								
				default :
					error = err_push(ERR_UNKNOWN_OPTION, "==> %s <==", argv[*i]);
				break;
			} /* switch on third letter of -O flag */

		break;
							
		case 'F' :
			switch (toupper(argv[*i][3])) /* -of? */
			{
				case STR_END : /* output format file */
					(*i)++;
					/* Check File */

					if (!argv[*i])
						error = err_push(ERR_PARAM_VALUE, "Need a name for output format file (following %s)", argv[*i - 1]);
					else
					{
						if (!os_file_exist(argv[*i]))
							error = err_push(ERR_OPEN_FILE, argv[*i]);
	
						std_args->output_format_file = argv[*i];
					}
				break;
								
				case 'T' : /* output format title */
					switch (toupper(argv[*i][4])) /* -oft? */
					{
						case STR_END :
							(*i)++;

							if (!argv[*i])
								error = err_push(ERR_PARAM_VALUE, "Need a title for output format (following %s)", argv[*i - 1]);
							else
								std_args->output_format_title = argv[*i];
						break;
							
						default:
							error = err_push(ERR_UNKNOWN_OPTION, "==> %s <==", argv[*i]);
						break;
					}
				break;
									
				default :
					error = err_push(ERR_UNKNOWN_OPTION, "==> %s <==", argv[*i]);
				break;
			} /* switch on third letter of -O flag */
		break;

		case 'L':
			switch (toupper(argv[*i][3])) /* -ol? */
			{
				case STR_END : /* log file */
					(*i)++;
					/* Check File */

					if (!argv[*i])
						error = err_push(ERR_PARAM_VALUE, "Need a name for log file (following %s)", argv[*i - 1]);
					else
					{
						FILE *fp;

						std_args->log_file = argv[*i];
						fp = fopen(std_args->log_file, "w");
						if (fp)
						{
							(void) fclose(fp);
							(void) remove(std_args->log_file);
						}
						else
							error = err_push(ERR_CREATE_FILE, std_args->log_file);
					}
				break;
									
				default :
					error = err_push(ERR_UNKNOWN_OPTION, "==> %s <==", argv[*i]);
				break;
			}

		break;

		default :
			error = err_push(ERR_UNKNOWN_OPTION, "==> %s <==", argv[*i]);
		break;

	} /* switch on second letter of -O flag */
	
	return(error);
}

static int option_P(char *argv[], FF_STD_ARGS_PTR std_args, int *i)
{
	int error = 0;

	switch (toupper(argv[*i][2])) /* -p? */
	{
		case STR_END :
			(*i)++;

			if (!argv[*i])
				error = err_push(ERR_PARAM_VALUE, "Need a value for precision (following %s)", argv[*i - 1]);
			else
			{
				char *endptr = NULL;

				std_args->user.set_cv_precision = 1;

				errno = 0;
				std_args->cv_precision = (int)strtod(argv[*i], &endptr);
				if (errno == ERANGE)
					error = err_push(errno, argv[*i]);
			
				if (ok_strlen(endptr))
					error = err_push(ERR_PARAM_VALUE, "Numeric conversion of \"%s\" stopped at \"%s\"", argv[*i], endptr);
			}
		break;

		default:
			error = err_push(ERR_UNKNOWN_OPTION, "==> %s <==", argv[*i]);
		break;
	}
	
	return(error);
}

static int option_S(char *argv[], FF_STD_ARGS_PTR std_args, int *i)
{
	int error = 0;

	switch (toupper(argv[*i][2])) /* -s? */
	{
		case STR_END :
			std_args->cv_subset = TRUE;
		break;

		default:
			error = err_push(ERR_UNKNOWN_OPTION, "==> %s <==", argv[*i]);
		break;
	}
	
	return(error);
}

static int option_T(char *argv[], FF_STD_ARGS_PTR std_args, int *i)
{
	int error = 0;

	switch (toupper(argv[*i][2])) /* -t? */
	{
		case STR_END :
			(*i)++;

			if (!argv[*i])
				error = err_push(ERR_PARAM_VALUE, "Need a terms file name (following %s)", argv[*i - 1]);
			else
			{
				if (!os_file_exist(argv[*i]))
					error = err_push(ERR_OPEN_FILE, argv[*i]);
			
				std_args->sdts_terms_file = argv[*i];
			}
		break;

		default:
			error = err_push(ERR_UNKNOWN_OPTION, "==> %s <==", argv[*i]);
		break;
	}
	
	return(error);
}

static int option_Q(char *argv[], FF_STD_ARGS_PTR std_args, int *i)
{
	int error = 0;

	switch (toupper(argv[*i][2])) /* -q? */
	{
		case STR_END :
			(*i)++;

			if (!argv[*i])
				error = err_push(ERR_PARAM_VALUE, "Need a name for query file (following %s)", argv[*i - 1]);
			else
			{
				if (!os_file_exist(argv[*i]))
					error = err_push(ERR_OPEN_FILE, argv[*i]);
			
				std_args->query_file = argv[*i];
			}
		break;
							
		default:
			error = err_push(ERR_UNKNOWN_OPTION, "==> %s <==", argv[*i]);
		break;
	}
	
	return(error);
}

static int option_V(char *argv[], FF_STD_ARGS_PTR std_args, int *i)
{
	int error = 0;

	switch (toupper(argv[*i][2])) /* -v? */
	{
		case STR_END :
			(*i)++;

			if (!argv[*i])
				error = err_push(ERR_PARAM_VALUE, "Need a name for variable file (following %s)", argv[*i - 1]);
			else
			{
				if (!os_file_exist(argv[*i]))
					error = err_push(ERR_OPEN_FILE, argv[*i]);
			
				std_args->var_file = argv[*i];
			}
		break;

		default:
			error = err_push(ERR_UNKNOWN_OPTION, "==> %s <==", argv[*i]);
		break;
	}
	
	return(error);
}

/*
static int option_(char *argv[], FF_STD_ARGS_PTR std_args, int *i)
{
	int error = 0;

	
	return(error);
}
*/

/*****************************************************************************
 * NAME:  is_redirecting_stdin
 *
 * PURPOSE:  Determine if stdin is being redirected
 *
 * USAGE:  if (is_redirecting_stdin(argc, argv)) { }
 *
 * RETURNS:  TRUE if stdin is redirected, FALSE if not
 *
 * DESCRIPTION:
 *
 * AUTHOR:  Mark Ohrenschall, NGDC, (303) 497-6124, mao@ngdc.noaa.gov
 *
 * SYSTEM DEPENDENT FUNCTIONS:  isatty, fileno
 *
 * GLOBALS:
 *
 * COMMENTS:
 *
 * KEYWORDS:
 *
 * ERRORS:  This test may be always returning TRUE on Sun Solaris when run
 * from a web-server CGI.  It may be that stdin is never associated with the
 * console in that environment.
 ****************************************************************************/

static BOOLEAN is_redirecting_stdin(void)
{
	BOOLEAN redirecting_stdin = FALSE;
	char *cp = NULL;

	cp = os_get_env("HTTP_HOST");
	if (cp)
	{
		memFree(cp, "cp");
		return(FALSE);
	}

	if (isatty(fileno(stdin)))
		return(FALSE);
	else
		return(TRUE);

	return(redirecting_stdin);
}

static BOOLEAN is_redirecting_stdout(void)
{
	BOOLEAN redirecting_stdout = FALSE;

	if (isatty(fileno(stdout)))
		return(FALSE);
	else
		return(TRUE);

	return(redirecting_stdout);
}

void show_command_line(int argc, char *argv[])
{
	int i;
	char cline[2 * MAX_PATH] = {""};

	sprintf(cline, "==>%s%s", argv[0], argc > 1 ? " " : "");

	for (i = 1; i < argc; i++)
		sprintf(cline + strlen(cline), "%s%s", argv[i], i < argc - 1 ? " " : "");

	sprintf(cline + strlen(cline), "<==");

	err_push(ERR_GENERAL, cline);
}

/*****************************************************************************
 * NAME:  parse_command_line()
 *
 * PURPOSE:  parses a command line for standard FreeForm arguments
 *
 * USAGE:  error = fill_std_args_from_command_line(argc, argv, std_args_ptr);
 *
 * RETURNS:  Zero on success, LAST error code on failure
 *
 * DESCRIPTION:  For each command line argument after the zero'th, look at the
 * first character, and then possibly every character thereafter in succession.
 * The "first" command line argument is treated a little differently, since it
 * can be either a file name, or an option flag (if input is being redirected).
 *
 * If the first character is a minus, then this indicates the beginning of an
 * option flag.  If it is an option flag, then examine every successive
 * character in turn to determine exactly which option.  This is done until
 * the NULL-terminator for the option string has been reached.  Once the option has
 * been determined, the next command line parameter may be examined, as
 * appropriate for that option.
 *
 * Multiple errors are possible -- each command line argument
 * is parsed in turn, each one possibly generating an error.  Only the last
 * error, which may not be the most severe, generates the return error code.
 * 
 * std_args must be a pre-allocated structure.
 *
 * 	Checks for these command line arguments:
 * 
 * -b   local_buffer_size
 * -c   count
 * -d   data_file_name
 * -ep  passive error messages
 * -el  error logging file
 * -if  input_format_file
 * -ift input_format_title
 * -f   format_file
 * -ft  format_title
 * -m   maximum number of bins (checkvar only)
 * -md  missing data value (checkvar only)
 * -mm  maxmin only processing (checkvar only)
 * -o   output_data_file
 * -of  output_format_file
 * -oft output_format_title
 * -ol  log file
 * -p   precision (checkvar only)
 * -q   query_file
 * -v   variable file
 *
 * checkvar can have optional arguments:
 *  -m maxbins
 *  -mm (maxmin only)
 *  -md (ignore missing data)
 *  -p precision
 *
 * The following are possible error return codes:
 *
 * ERR_PARAM_VALUE (because a numeric string could not be converted to an integer)
 * ERANGE (because of an overflow or underflow in converting above)
 * ERR_OPEN_FILE (because a specified file does not exist)
 * ERR_FILE_EXISTS (because the output file would be overwritten)
 * ERR_UNKNOWN_OPTION (because of an unrecognized flag typed on the command line)
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

int parse_command_line(int argc, char *argv[], FF_STD_ARGS_PTR std_args)
{
	int i;
	int error;
	int num_flags = 0;
	int last_error = 0;

	assert(std_args);
	
	if (!std_args)
		return(err_push(ERR_MEM_LACK, "standard args structure"));
	
	error = 0;

	/* Has user specified no command line parameters but is redirecting stdin? 
	   A format file will have to specified -- err-out when failing to create formats.
	*/

	if (argc == 1 && is_redirecting_stdin())
		std_args->user.is_stdin_redirected = 1;
	
	/* Interpret the command line */
	for (i = 1; i < argc; i++)
	{
		switch (argv[i][0]) /* ? */
		{
			case '-' :
				num_flags++;
				
				switch (toupper(argv[i][1])) /* -? */
				{
					case 'B' : /* cache size */
						error = option_B(argv, std_args, &i);
					break;
					
					case 'C' : /* head 'n tail count */
						error = option_C(argv, std_args, &i);
					break;

					case 'D' : /* data file name */
						error = option_D(argv, std_args, &i);
					break;

					case 'E' : /* error logging option */
						error = option_E_(argv, std_args, &i);
					break;
				
					case 'F' : /* single format file/title */
						error = option_F_(argv, std_args, &i);
					break;
					
					case 'G' : /* SDE grid sizes */
						error = option_G(argv, std_args, &i);
					break;
					
					case 'I' : /* input format file/title */
						error = option_I__(argv, std_args, &i);
					break;
	
					case 'M' : /* Checkvar maxbins, missing data, or max/min only */
						error = option_M_(argv, std_args, &i);
					break;
					
					case 'O' : /* output data file, or output format file/title */
						error = option_O__(argv, std_args, &i);
					break;
					
					case 'P' : /* Checkvar precision */
						error = option_P(argv, std_args, &i);
					break;
					
					case 'S' : /* Checkvar subsetting */
						error = option_S(argv, std_args, &i);
					break;
					
					case 'T' : /* FF2PSDTS terms file */
						error = option_T(argv, std_args, &i);
					break;
					
					case 'Q' : /* query file */
						error = option_Q(argv, std_args, &i);
					break;
					
					case 'V' : /* variable file */
						error = option_V(argv, std_args, &i);
					break;
					
					default :
						error = err_push(ERR_UNKNOWN_OPTION, "==> %s <==", argv[i]);
					break;
				} /* switch on flag letter code */
				
				if (error)
					last_error = error;
			break; /* case '-' */

			default :
		
				/* Has user omitted the optional -D option flag? */
				if (i == 1)
				{
					if (!os_file_exist(argv[1]))
						last_error = err_push(ERR_OPEN_FILE, argv[1]);

					std_args->input_file = argv[1];
				}
				else
					last_error = err_push(ERR_UNKNOWN_OPTION, "Expecting an option flag (beginning with '-')\n==> %s <==", argv[i]);
			break;
		} /* switch on argv[?][0] */
	} /* End of Command line interpretation (for loop)*/

	/* Is user redirecting stdin? */
	if (!std_args->input_file)
	{
		if (is_redirecting_stdin())
			std_args->user.is_stdin_redirected = 1;
		else if (argc > 1)
			last_error = err_push(ERR_GENERAL, "Expecting a data file to process");
	}

	if (!std_args->output_file && is_redirecting_stdout())
		std_args->user.is_stdout_redirected = 1;

	if (last_error)
		show_command_line(argc, argv);

	/* Has user requested a log file but not an error log?  Why make errors interactive
	   when they are logging?  So send errors and log to the same file
	*/
	if (!std_args->error_log && std_args->log_file)
		std_args->error_log = std_args->log_file;

	return(last_error);
}

static BOOLEAN cmp_array_conduit
	(
	 FF_ARRAY_CONDUIT_PTR src_conduit,
	 FF_ARRAY_CONDUIT_PTR trg_conduit
	)
{
	FF_VALIDATE(src_conduit);
	FF_VALIDATE(trg_conduit);

	if (src_conduit->input && trg_conduit->input)
		return(ff_format_comp(src_conduit->input->fd->format, trg_conduit->input->fd->format));
	else if (src_conduit->output && trg_conduit->output)
		return(ff_format_comp(src_conduit->output->fd->format, trg_conduit->output->fd->format));
	else
		return 0;
}
	 
static int merge_redundant_conduits(FF_ARRAY_CONDUIT_LIST conduit_list)
{
	int error = 0;

	error = list_replace_items((pgenobj_cmp_t)cmp_array_conduit, conduit_list); 
	return(error);
}

#ifdef ND_FP 
static void release_file_handles
	(
	 DATA_BIN_PTR dbin,
	 FF_TYPES_t io
	)
{
	PROCESS_INFO_LIST plist = NULL;
	PROCESS_INFO_PTR pinfo = NULL;

	FF_VALIDATE(dbin);

	if (!db_ask(dbin, DBASK_PROCESS_INFO, io, &plist))
	{
		plist = dll_first(plist);
		pinfo = FF_PI(plist);
		while (pinfo)
		{
			if (PINFO_SUPER_ARRAY(pinfo)->fp)
				fclose(PINFO_SUPER_ARRAY(pinfo)->fp);
			else if (PINFO_SUB_ARRAY(pinfo)->fp)
				fclose(PINFO_SUB_ARRAY(pinfo)->fp);

			plist = dll_next(plist);
			pinfo = FF_PI(plist);
		}

		ff_destroy_process_info_list(plist);
	}
}
#endif

static void remove_header_from_ac_list
	(
	 DATA_BIN_PTR dbin,
	 char *name
	)
{
	FF_ARRAY_CONDUIT_LIST aclist = NULL;
	FF_ARRAY_CONDUIT_PTR acptr = NULL;

	FF_VALIDATE(dbin);

	aclist = dbin->array_conduit_list;
	aclist = dll_next(aclist);
	acptr = FF_AC(aclist);
	while (acptr)
	{
		FF_VALIDATE(acptr);

		if (!strcmp(name, acptr->output->fd->format->name))
		{
			ff_destroy_array_pole(acptr->output);

			if (acptr->input)
				acptr->input->mate = NULL;

			acptr->output = NULL;

			break;
		}

		aclist = dll_next(aclist);
		acptr = FF_AC(aclist);
	}
}

static int check_file_access(DATA_BIN_PTR dbin)
{
	PROCESS_INFO_LIST plist = NULL;
	PROCESS_INFO_PTR pinfo = NULL;

	int error = 0;

	error = db_ask(dbin, DBASK_PROCESS_INFO, FFF_OUTPUT, &plist);
	if (!error)
	{
		BOOLEAN no_overwrite = FALSE;

		if (nt_askexist(dbin, NT_ANYWHERE, "nooverwrite"))
			no_overwrite = TRUE;

		plist = dll_first(plist);
		pinfo = FF_PI(plist);
		while (pinfo)
		{
			if (PINFO_IS_FILE(pinfo))
			{
				if (os_file_exist(PINFO_FNAME(pinfo)))
				{
					if (PINFO_MATE(pinfo) && PINFO_MATE_IS_FILE(pinfo) && !strcmp(PINFO_FNAME(pinfo), PINFO_MATE_FNAME(pinfo)))
						error = err_push(ERR_GENERAL, "Input and output %s files have the same name!", IS_DATA(PINFO_FORMAT(pinfo)) ? "data" : "header");
					else if (!PINFO_IS_BROKEN(pinfo))
					{
						if (no_overwrite)
							error = err_push(ERR_FILE_EXISTS, PINFO_FNAME(pinfo));
						else
						{
							if (IS_SEPARATE(PINFO_FORMAT(pinfo)) && IS_FILE_HEADER(PINFO_FORMAT(pinfo)))
							{
								/* Is this a zero length file?  If so, go ahead and overwrite it. */
								if (os_filelength(PINFO_FNAME(pinfo)))
								{
									err_push(ERR_WARNING_ONLY + ERR_FILE_EXISTS, "Output header (%s) will not be overwritten", PINFO_FNAME(pinfo));

									remove_header_from_ac_list(dbin, PINFO_FORMAT(pinfo)->name);
								}
							}
							else
								err_push(ERR_WARNING_ONLY + ERR_WILL_OVERWRITE_FILE, "%s: \"%s\"", PINFO_FNAME(pinfo), PINFO_NAME(pinfo));
						}
					}
				}
			}

			plist = dll_next(plist);
			pinfo = FF_PI(plist);
		}

		ff_destroy_process_info_list(plist);
		error = db_ask(dbin, DBASK_PROCESS_INFO, FFF_OUTPUT, &plist);
		if (!error)
		{
			plist = dll_first(plist);
			pinfo = FF_PI(plist);
			while (pinfo)
			{
				if (PINFO_IS_FILE(pinfo) && !PINFO_IS_BROKEN(pinfo))
				{
					 /* Can we write to file? */
					if ((!error || error > ERR_WARNING_ONLY) && (!no_overwrite || !os_file_exist(PINFO_FNAME(pinfo))))
					{
#ifdef ND_FP 
						PINFO_SUB_ARRAY(pinfo)->fp = fopen(PINFO_FNAME(pinfo), "w");
						if (PINFO_SUB_ARRAY(pinfo)->fp)
						{
							fclose(PINFO_SUB_ARRAY(pinfo)->fp);

							PINFO_SUB_ARRAY(pinfo)->fp = fopen(PINFO_FNAME(pinfo), "w+b");
							if (!PINFO_SUB_ARRAY(pinfo)->fp)
							{
								release_file_handles(dbin, FFF_OUTPUT);
								break;
							}
						}
#else
						FILE *fp = NULL;

						fp = fopen(PINFO_FNAME(pinfo), "w");
						if (fp)
							fclose(fp);
#endif
						else
							error = err_push(ERR_CREATE_FILE, "%s: \"%s\"", PINFO_FNAME(pinfo), PINFO_NAME(pinfo));
					}
				}

				plist = dll_next(plist);
				pinfo = FF_PI(plist);
			}

			ff_destroy_process_info_list(plist);
		}
		else if (error == ERR_GENERAL)
			error = 0;
	}
	else if (error == ERR_GENERAL)
		error = 0;

#ifdef ND_FP 
	if (!error)
	{
		error = db_ask(dbin, DBASK_PROCESS_INFO, FFF_INPUT, &plist);
		if (!error)
		{
			plist = dll_first(plist);
			pinfo = FF_PI(plist);
			while (pinfo)
			{
				if (PINFO_IS_FILE(pinfo) && !PINFO_IS_BROKEN(pinfo))
				{
					PINFO_SUPER_ARRAY(pinfo)->fp = fopen(PINFO_FNAME(pinfo), "rb");
					if (!PINFO_SUPER_ARRAY(pinfo)->fp)
					{
						release_file_handles(dbin, FFF_INPUT);
						release_file_handles(dbin, FFF_OUTPUT);
						break;
					}
				}

				plist = dll_next(plist);
				pinfo = FF_PI(plist);
			}
		}

		ff_destroy_process_info_list(plist);
	}
#endif

	return(error);
}

void fd_destroy_format_data_list(FORMAT_DATA_LIST format_data_list)
{
	dll_free_holdings(format_data_list);
}

/*
 * NAME:                db_free
 *              
 * PURPOSE:     To free dbin and associated data structure
 *
 * USAGE:       db_free(DATA_BIN_PTR dbin)
 *
 * RETURNS:     void
 *
 * DESCRIPTION: This function frees the data bin structure and its associated
 *                              data structure, closes all opened input, output, and header file.
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none
 *
 * GLOBALS:     none
 *
 * AUTHOR:      Liping Di, NGDC, (303) 497 - 6284, lpd@mail.ngdc.noaa.gov
 *
 * COMMENTS:    
 *
 * KEYWORDS:    dbin
 *
 */

#undef ROUTINE_NAME
#define ROUTINE_NAME "db_free"

void db_destroy(DATA_BIN_PTR dbin)
{
	FF_VALIDATE(dbin);
	
	if (dbin->eqn_info)
	{
		ee_free_einfo(dbin->eqn_info);
		dbin->eqn_info = NULL;
	}

	if (dbin->array_conduit_list)
	{
		ff_destroy_array_conduit_list(dbin->array_conduit_list);
		dbin->array_conduit_list = NULL;
	}

	if (dbin->table_list)
	{
		fd_destroy_format_data_list(dbin->table_list);
		dbin->table_list = NULL;
	}

	if (dbin->title)
	{
		memFree(dbin->title, "dbin->title");
		dbin->title = NULL;
	}

#ifdef FF_CHK_ADDR
	dbin->check_address = NULL;
#endif
	
	memFree(dbin, "dbin");

	return;
}

#ifdef ROUTINE_NAME
#undef ROUTINE_NAME
#endif 
#define ROUTINE_NAME "variable_comp"

/*****************************************************************************
 * NAME: variable_comp
 *
 * PURPOSE:  Compare two variables
 *
 * USAGE:  are_same = variable_comp(variable1, variable2);
 *
 * RETURNS:  TRUE if variables are identical, FALSE if not
 *
 * DESCRIPTION:  Compares each element of the variables until a difference
 * is found.
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

static BOOLEAN variable_comp
	(
	 VARIABLE_PTR v1,
	 VARIABLE_PTR v2
	)
{
	FF_VALIDATE(v1);
	FF_VALIDATE(v2);
	
	if (!v1 || !v2)
		return(FALSE);
	
#ifdef FF_CHK_ADDR
	if ((void *)v1 != v1->check_address)
		return(FALSE);
		
	if ((void *)v2 != v2->check_address)
		return(FALSE);
	
#endif
	if (IS_ARRAY(v1) && IS_ARRAY(v2) && strcmp(v1->array_desc_str, v2->array_desc_str))
		return(FALSE);

	if (strcmp(v1->name, v2->name))
		return(FALSE);

	if (v1->type != v2->type)
		return(FALSE);
		
	if (v1->start_pos != v2->start_pos)
		return(FALSE);
		
	if (v1->end_pos != v2->end_pos)
		return(FALSE);
	
	if (v1->precision != v2->precision)
		return(FALSE);

	assert(!IS_CONVERT(v1));
	if (IS_CONVERT(v1) && v1->misc.cv_var_num != v2->misc.cv_var_num)
		return(FALSE);
	else if (IS_TRANSLATOR(v1) && !nt_comp_translator_sll(v1, v2))
		return(FALSE);
	
	return(TRUE);
}

#ifdef ROUTINE_NAME
#undef ROUTINE_NAME
#endif 
#define ROUTINE_NAME "variable_list_comp"

/*****************************************************************************
 * NAME: variable_list_comp
 *
 * PURPOSE:  Compare two variable lists
 *
 * USAGE:  are_same = variable_list_comp(format1->variables, format2->variables);
 *
 * RETURNS:  TRUE if variable lists are identical, FALSE if not
 *
 * DESCRIPTION:  Compares each variable in the lists until a
 * difference is found.
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

static BOOLEAN variable_list_comp
	(
	 VARIABLE_LIST vlist1,
	 VARIABLE_LIST vlist2
	)
{
	VARIABLE_PTR v1;
	VARIABLE_PTR v2;
	
	if (!vlist1 || !vlist2)
		return(FALSE);
	
	vlist1 = dll_first(vlist1);
	v1 = FF_VARIABLE(vlist1);
	
	vlist2 = dll_first(vlist2);
	v2 = FF_VARIABLE(vlist2);
	
	while (v1 || v2)
	{
		if (!(v1 && v2))
			return(FALSE);

		if (!variable_comp(v1, v2))
			return(FALSE);
		
		vlist1 = dll_next(vlist1);
		v1 = FF_VARIABLE(vlist1);
		
		vlist2 = dll_next(vlist2);
		v2 = FF_VARIABLE(vlist2);
	}
	
	return(TRUE);
}

#ifdef ROUTINE_NAME
#undef ROUTINE_NAME
#endif 
#define ROUTINE_NAME "ff_format_comp"

/*****************************************************************************
 * NAME: ff_format_comp
 *
 * PURPOSE:  Compare two formats
 *
 * USAGE:  are_same = ff_format_comp(format1, format2);
 *
 * RETURNS:  TRUE if formats are identical, FALSE if not
 *
 * DESCRIPTION:  Compares each element of the formats until a difference
 * is found.  If no difference, the formats' variable lists are then
 * compared.
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

BOOLEAN ff_format_comp
	(
	 FORMAT_PTR f1,
	 FORMAT_PTR f2
	)
{
	FF_VALIDATE(f1);
	FF_VALIDATE(f2);
	
	if (!f1 || !f2)
		return(FALSE);
	
	if (strcmp(f1->name, f2->name))
		return(FALSE);

	if (strcmp(f1->locus, f2->locus))
		return(FALSE);	

	if (f1->type != f2->type)
		return(FALSE);
	
	if (f1->num_vars != f2->num_vars)
		return(FALSE);
	
	if (f1->length != f2->length)
		return(FALSE);
	
	if (!variable_list_comp(f1->variables, f2->variables))
		return(FALSE);
	
	return(TRUE);
}

/* Longest: Binary Output Separate Varied Record Header */
static int get_format_type
	(
	 FORMAT_PTR format,
	 char *title
	)
{	
	FF_VALIDATE(format);

	strcpy(title, ff_lookup_string(format_types, FFF_FORMAT_TYPE(format)));
	os_str_replace_char(title, '_', ' ');

	strcat(title, "\b");

	return(0);
}

static int make_unique_format_titles(DATA_BIN_PTR dbin)
{
	PROCESS_INFO_LIST plist = NULL;
	PROCESS_INFO_PTR pinfo = NULL;

	int error = 0;
	int SCRATCH = strlen("Binary Output Separate Varied Record Header: ") + 1; /* Longest */

	char *cp = NULL;

	FF_VALIDATE(dbin);

	error = db_ask(dbin, DBASK_PROCESS_INFO, 0, &plist);
    if (error)
        goto exit;

	plist = dll_first(plist);
	pinfo = FF_PI(plist);
	while (pinfo && !error)
	{
		FF_VALIDATE(pinfo);

		cp = (char *)memRealloc(PINFO_FORMAT(pinfo)->name, strlen(PINFO_FORMAT(pinfo)->name) + SCRATCH + 1, "PINFO_FORMAT(pinfo)->name");
		if (cp)
		{
			PINFO_FORMAT(pinfo)->name = cp;
			memmove(cp + SCRATCH, cp, strlen(cp) + 1);
		}
		else
		{
			error = err_push(ERR_MEM_LACK, "");
			break;
		}

		error = get_format_type(PINFO_FORMAT(pinfo), cp);
		if (error)
			break;

		memmove(cp + strlen(cp), cp + SCRATCH, strlen(cp + SCRATCH) + 1);

		plist = dll_next(plist);
		pinfo = FF_PI(plist);
	}

	ff_destroy_process_info_list(plist);

	exit:

	return error;
}


#undef ROUTINE_NAME
#define ROUTINE_NAME "db_init"

/*****************************************************************************
 * NAME:  db_create()
 *
 * PURPOSE:  Makes a data bin and calls db_set() events according to FreeForm
 * standard arguments structure
 *
 * USAGE:  error = db_create(std_args, &dbin);
 *
 * RETURNS:  0 on success or an error code defined in err.h or a db_set event
 * code on failure.
 *
 * DESCRIPTION:    The error code indicates the event which failed.
 *
 * The following are error return codes defined in err.h:
 *
 * ERR_MEM_LACK   (indicates a memory allocation failure)
 * ERR_GEN_ARRAY  (indicates a problem with an array description)
 * ERR_OPEN_FILE  (as when an output format's INITIAL file cannot be opened)
 *
 * The following are error return codes which are db_set events:
 *
 * DBSET_READ_EQV (an error in reading a .eqv file)
 * DBSET_INPUT_FORMATS (an error in processing input formats)
 * DBSET_OUTPUT_FORMATS (an error in processing output formats)
 * DBSET_VARIABLE_RESTRICTION (an error in processing variable thinning file)
 * DBSET_HEADER_FILE_NAMES (an error in locating external headers)
 * DBSET_HEADERS (an error in reading an input header)
 * DBSET_QUERY_RESTRICTION (an error in processing a query file)
 *
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

int db_init
	(
	 FF_STD_ARGS_PTR std_args,
	 DATA_BIN_HANDLE dbin_h,
	 int (*error_cb)(int)
	)
{
	FORMAT_DATA_LIST format_data_list = NULL;
	int error = 0;
	int num_errors = 0;
	
	assert(dbin_h);

	if (!dbin_h)
		return(err_push(ERR_API, "NULL DATA_BIN_HANDLE in %s", ROUTINE_NAME));

	if (!*dbin_h)
	{
		*dbin_h = db_make(std_args->input_file ? std_args->input_file : "Application Program");
		if (!*dbin_h)
			return(err_push(ERR_MEM_LACK, "Standard Data Bin"));
	}

	/* Now set the formats and the auxillary files */

	if (db_set(*dbin_h, DBSET_READ_EQV, std_args->input_file))
	{
		err_push(ERR_SET_DBIN, "making name table for %s", std_args->input_file);
		return(DBSET_READ_EQV);
	}
	
	if (db_set(*dbin_h,
	           DBSET_INPUT_FORMATS,
	           std_args->input_file,
	           std_args->output_file,
	           std_args->input_format_file,
	           std_args->input_format_buffer,
	           std_args->input_format_title,
	           &format_data_list
	          )
	   )
	{
		if (format_data_list)
			dll_free_holdings(format_data_list);

		err_push(ERR_SET_DBIN, "setting an input format for %s", std_args->input_file);
		return(DBSET_INPUT_FORMATS);
	}

	num_errors = err_count();

	if (db_set(*dbin_h,
				  DBSET_OUTPUT_FORMATS,
				  std_args->input_file,
				  std_args->output_file,
				  std_args->output_format_file,
				  std_args->output_format_buffer,
				  std_args->output_format_title,
				  &format_data_list
				 )
		)
	{
		if (!error_cb || (*error_cb)(DBSET_OUTPUT_FORMATS))
		{
			dll_free_holdings(format_data_list);

			err_push(ERR_SET_DBIN, "setting an output format for %s", std_args->input_file);
			return(DBSET_OUTPUT_FORMATS);
		}
		else
		{
			while (err_count() > num_errors)
				err_pop();
		}
	}

	error = db_set(*dbin_h, DBSET_CREATE_CONDUITS, std_args, format_data_list);
	dll_free_holdings(format_data_list);
	if (error)
	{
		err_push(ERR_SET_DBIN, "creating array information for %s", std_args->input_file);
		return(DBSET_CREATE_CONDUITS);
	}

	/* Check for variable file */
	if (std_args->var_file)
	{
		FORMAT_DATA_PTR output_fd = fd_get_data(*dbin_h, FFF_OUTPUT);

		if (IS_ARRAY(output_fd->format))
		{
			err_push(ERR_SET_DBIN, "Cannot use variable file with arrays");
			return(DBSET_VARIABLE_RESTRICTION);
		}

		if (db_set(*dbin_h, DBSET_VARIABLE_RESTRICTION, std_args->var_file, output_fd->format))
		{
			err_push(ERR_SET_DBIN, "Unable to use variable file \"%s\"", std_args->var_file);
			return(DBSET_VARIABLE_RESTRICTION);
		}
	}

	if (db_set(*dbin_h, DBSET_EQUATION_VARIABLES))
	{
		err_push(ERR_SET_DBIN, "setting equation variables for %s", std_args->input_file);
		return(DBSET_EQUATION_VARIABLES);
	}

	if (db_set(*dbin_h, DBSET_HEADER_FILE_NAMES, FFF_INPUT, std_args->input_file))
	{
		err_push(ERR_SET_DBIN, "Determining input header file names for %s", std_args->input_file);
		return(DBSET_HEADER_FILE_NAMES);
	}

	num_errors = err_count();

	if (db_set(*dbin_h, DBSET_HEADER_FILE_NAMES, FFF_OUTPUT, std_args->output_file))
	{
		if (!error_cb || (*error_cb)(DBSET_OUTPUT_FORMATS))
		{
			err_push(ERR_SET_DBIN, "Determining output header file names for %s", std_args->output_file);
			return(DBSET_HEADER_FILE_NAMES);
		}
		else
		{
			while (err_count() > num_errors)
				err_pop();
		}
	}

	if (db_set(*dbin_h, DBSET_BYTE_ORDER, FFF_INPUT | FFF_HEADER))
	{
		err_push(ERR_SET_DBIN, "Defining input data byte order");
		return(DBSET_BYTE_ORDER);
	}

	if (db_set(*dbin_h, DBSET_HEADERS))
	{
		err_push(ERR_SET_DBIN, "getting header file for %s", std_args->input_file);
		return(DBSET_HEADERS);
	}
	
	if (db_set(*dbin_h, DBSET_USER_UPDATE_FORMATS))
	{
		err_push(ERR_SET_DBIN, "user update of a format for %s", std_args->input_file);
		return(DBSET_USER_UPDATE_FORMATS);
	}

	if (db_set(*dbin_h, DBSET_BYTE_ORDER, FFF_INPUT))
	{
		err_push(ERR_SET_DBIN, "Defining input data byte order");
		return(DBSET_BYTE_ORDER);
	}

	num_errors = err_count();

	if (db_set(*dbin_h, DBSET_BYTE_ORDER, FFF_OUTPUT))
	{
		if (!error_cb || (*error_cb)(DBSET_OUTPUT_FORMATS))
		{
			err_push(ERR_SET_DBIN, "Defining output data byte order");
			return(DBSET_BYTE_ORDER);
		}
		else
		{
			while (err_count() > num_errors)
				err_pop();
		}
	}

	if (std_args->cache_size == 0)
		std_args->cache_size = DEFAULT_CACHE_SIZE;

	/* Check for query file */
	if (std_args->query_file)
	{
		if (db_set(*dbin_h, DBSET_QUERY_RESTRICTION, std_args->query_file))
		{
			err_push(ERR_GEN_QUERY, "setting query using %s", std_args->query_file);
			return(DBSET_QUERY_RESTRICTION);
		}
	}

	if (db_set(*dbin_h, DBSET_INIT_CONDUITS, FFF_DATA, std_args->records_to_read))
	{
		err_push(ERR_SET_DBIN, "creating array information for %s", std_args->input_file);
		return(DBSET_INIT_CONDUITS);
	}

	if (error)
		return(error);

	error = merge_redundant_conduits((*dbin_h)->array_conduit_list);
	if (error)
		return(error);

	if (fd_get_data(*dbin_h, FFF_INPUT) &&
	    db_set(*dbin_h, DBSET_CACHE_SIZE, std_args->cache_size)
	   )
	{
		return(err_push(ERR_MEM_LACK, "setting data cache for %s", std_args->input_file));
	}

	if (db_set(*dbin_h, DBSET_FORMAT_MAPPINGS))
	{
		err_push(ERR_SET_DBIN, "mapping input to output formats for %s", std_args->input_file);
		return(DBSET_FORMAT_MAPPINGS);
	}

	error = make_unique_format_titles(*dbin_h);
	if (error)
		return ERR_MEM_LACK;

	num_errors = err_count();

	if (db_set(*dbin_h, DBSET_VAR_MINMAX))
	{
		if (!error_cb || (*error_cb)(DBSET_OUTPUT_FORMATS))
		{
			err_push(ERR_SET_DBIN, "setting variable minimums and maximums for %s", std_args->input_file);
			return(DBSET_VAR_MINMAX);
		}
		else
		{
			while (err_count() > num_errors)
				err_pop();
		}
	}
			
	error = check_file_access(*dbin_h);
	if (error)
		return error;

	return(error);
}

