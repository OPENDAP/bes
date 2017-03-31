/*
 * FILENAME:  chkform.c
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
#include <time.h>

#define BAD_NUMBER 1
#define BAD_DECIMAL_POINT 2
#define RANGE_ERROR 3
#define FOUND_NON_PRINT 4

const char *messages[] = { "Badly Formatted Number\x20",
                           "Expecting a Decimal Point\x20",
                           "Out of Range\x20",
									"Non-printable Character\x20"
                         };
                       
static int chkform
	(
	 time_t start_time,
	 FF_STD_ARGS_PTR std_args,
	 DATA_BIN_PTR dbin,
	 FF_BUFSIZE_PTR chkform_log
	);

static int do_checks
	(
	 DATA_BIN_PTR dbin,
	 FF_BUFSIZE_PTR chkform_log,
	 unsigned long *line_count,
	 unsigned long *sep_line_count,
	 unsigned long *total_errors
	);

static int check_buffer
	(
	 FORMAT_PTR format,
	 char *buffer,
	 unsigned long num_bytes,
	 FF_BUFSIZE_PTR chkform_log,
	 unsigned long *line_count,
	 unsigned long *total_errors
	);

/*****************************************************************************
 * NAME:  check_for_unused_flags()
 *
 * PURPOSE:  Has user asked for an unimplemented option?
 *
 * USAGE:  check_for_unused_flags(std_args_ptr);
 *
 * RETURNS:  void
 *
 * DESCRIPTION:  All FreeForm utilities do not employ all of the "standard"
 * FreeForm command line options.  Check if the user has unwittingly asked
 * for any options which this utility will ignore.
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

static int check_for_unused_flags(FF_STD_ARGS_PTR std_args)
{
	int error = 0;

	if (std_args->user.set_cv_precision)
		error = err_push(ERR_IGNORED_OPTION, "precision (checkvar only)");
	
	if (std_args->cv_maxbins)
		error = err_push(ERR_IGNORED_OPTION, "maximum number of histogram bins (checkvar only)");
	
	if (std_args->cv_maxmin_only)
		error = err_push(ERR_IGNORED_OPTION, "maximum and minimum processing only (checkvar only)");

	if (std_args->user.is_stdin_redirected && std_args->records_to_read)
		error = err_push(ERR_IGNORED_OPTION, "Records to read when redirecting standard input");
	
	if (std_args->output_file)
		error = err_push(ERR_IGNORED_OPTION, "output_file file (Newform only)");
	
	if (std_args->output_format_file && std_args->user.format_file) 
		error = err_push(ERR_IGNORED_OPTION, "Format file (use -if)");
	else if (std_args->output_format_file) 
		error = err_push(ERR_IGNORED_OPTION, "Output format file (Newform only)");
	
	if (std_args->output_format_title)
		error = err_push(ERR_IGNORED_OPTION, "Output format title (Newform only)");
	
	if (std_args->var_file)
		error = err_push(ERR_IGNORED_OPTION, "Variable file (Newform only)");

	if (std_args->output_format_title && std_args->user.format_title)
		error = err_push(ERR_IGNORED_OPTION, "Format title (use -ift)");
	else if (std_args->output_format_title)
		error = err_push(ERR_IGNORED_OPTION, "Output format title");
	
	return(error);
}
	
static void print_positions(FF_BUFSIZE_PTR chkform_log, int count)
{
	int i = 0; 

	for (i = 0; i < count / 10; i++)
		do_log(chkform_log, "%10d", i + 1);

	do_log(chkform_log, "\n");
	
	for (i = 0; i < count / 10; i++)
		do_log(chkform_log, "1234567890");

	for (i = 0; i < count % 10; i++)
		do_log(chkform_log, "%d", i + 1);

	do_log(chkform_log, "\n");
}

static void print_buffer(FF_BUFSIZE_PTR chkform_log, char *buffer, int length)
{
	if (length)
	{
		int i = 0;

		for (i = 0; i < length; i++)
			do_log(chkform_log, "%c", buffer[i] == STR_END ? ' ' : buffer[i]);

		do_log(chkform_log, "\n");
	}
	else
		do_log(chkform_log, "%s\n", buffer);
}

int events_to_do(int event)
{
	if (event == DBSET_OUTPUT_FORMATS)
		return(FALSE);
	else
		return(TRUE);
}

#undef ROUTINE_NAME
#define ROUTINE_NAME "chkform"

/*
 * NAME:  chkform.c
 *
 * PURPOSE:  To check an ASCII (.DAT) file with a freeform description for possible
 * formatting errors with any numeric field.
 *
 * AUTHOR:  Ted Habermann, NGDC, (303)497-6472, haber@ngdc1.colorado.edu
 *				  Modified by TAM
 *          Modified by MAO
 *
 * USAGE:  check_format [options] <format file> <input file>
 *
 * COMMENTS:  Only the -q option (quick scan) is implemented.
 * A quick scan parses numeric fields as character strings (e.g., check for spaces
 * between digits), whereas a full scan also checks the value of numeric fields for
 * range or precision violations (e.g., uchar should not have values greater than 255,
 * and floats should not have a precision greater than 7).
 *
 * Further notes:  
 * 1) A field filled with spaces is considered in error.
 * 2) A field padded on the left with spaces is okay, but tabs are not okay.
 * 3) Range checking is generally dependent on the external variable errno being set
 * to ERR_RANGE by the family of strtol(), strtoul(), strtod(), atoi() functions.
 */

/* long lnum=0; DECLARED BUT NEVER USED */

int main(int argc, char **argv)
{
	char *greeting = 
{
#ifdef FF_ALPHA
"\nWelcome to Chkform alpha "FFND_LIB_VER" "__DATE__" -- an NGDC FreeForm ND application\n\n"
#elif defined(FF_BETA)
"\nWelcome to Chkform beta "FFND_LIB_VER" "__DATE__" -- an NGDC FreeForm ND application\n\n"
#else
"\nWelcome to Chkform release "FFND_LIB_VER" -- an NGDC FreeForm ND application\n\n"
#endif
};

	char *command_line_usage = {
"Several Chkform command line elements have default extensions:\n\
data_file: .dat = ASCII  .dab = flat ASCII\n\
input_format_file: .fmt = format description file\n\
Chkform [-d data_file] [-if input_format_file] [-ift \"input format title\"]\n\
        [-b cache_size] Sets the input data cache size in bytes\n\
        [-c count] No. records to process at head(+)/tail(-) of file\n\
        [-q query] Output records matching criteria in query file\n\
        [-ol log] File to contain log of processing information\n\n\
        [-el error_log_file] error messages go to error_log_file\n\
        [-ep] passive error messages - non-interactive\n\
See the FreeForm User's Guide for detailed information.\n"
};

	FF_BUFSIZE_PTR bufsize = NULL;
	FF_BUFSIZE_PTR chkform_log = NULL;

	char log_file_write_mode[4];
  
	DATA_BIN_PTR dbin = NULL;
	int error = 0;
	FF_STD_ARGS_PTR std_args = NULL;

	time_t start_time;
	(void)time(&start_time);

	fprintf(stderr, "%s", greeting);
	
	std_args = ff_create_std_args();
	if (!std_args)
	{
		error = ERR_MEM_LACK;
		goto main_exit;
	}
	
	error = parse_command_line(argc, argv, std_args);
	if (error)
		goto main_exit;

	error = check_for_unused_flags(std_args);
	if (error)
		goto main_exit;

	if (std_args->output_format_file && !strcmp(std_args->output_format_file, std_args->input_format_file))
		std_args->output_format_file = NULL;

	if (std_args->log_file)
	{
		chkform_log = ff_create_bufsize(SCRATCH_QUANTA);
		if (chkform_log)
			do_log(chkform_log, "%s", greeting);
		else
		{
			error = err_push(ERR_MEM_LACK, "");
			goto main_exit;
		}
	}

	/* Check number of command args */
	if (argc == 1 && !std_args->user.is_stdin_redirected)
	{
		fprintf(stderr, "%s", command_line_usage);
		ff_destroy_std_args(std_args);
	
		memExit(EXIT_FAILURE, "main, argc<2");
	}

	error = db_init(std_args, &dbin, &events_to_do);
	if (error && error < ERR_WARNING_ONLY)
		goto main_exit;
	else if (error)
		error = 0;

	if (std_args->user.is_stdin_redirected)
	{
		error = db_set(dbin, DBSET_SETUP_STDIN, std_args);
		if (error)
			goto main_exit;
	}

	if (!isatty(fileno(stdout)))
	{
#if FF_OS == FF_OS_DOS || FF_OS == FF_OS_MACOS
		setmode(fileno(stdout), O_BINARY);
#endif
		error = db_do(dbin, DBDO_CHECK_STDOUT);
		if (error)
			goto main_exit;
	}

	if (std_args->query_file)
		do_log(chkform_log, "Using query file: %s\n", std_args->query_file);

	/* Display some information about the data, ignore errors */
	(void) db_ask(dbin, DBASK_FORMAT_SUMMARY, FFF_INPUT, &bufsize);
	do_log(chkform_log, "%s\n", bufsize->buffer);
	ff_destroy_bufsize(bufsize);

	error = chkform(start_time, std_args, dbin, chkform_log);
	
	if (std_args->user.is_stdin_redirected)
		ff_destroy_bufsize(std_args->input_bufsize);

	wfprintf(stderr,"\n");

main_exit:

	if (dbin)
		db_destroy(dbin);
	
	if (error || err_state())
		err_disp(std_args);

	/* Is user asking for both error logging and a log file? */
	if (std_args->error_log && chkform_log)
	{
		if (strcmp(std_args->error_log, std_args->log_file))
		{
#if FF_OS == FF_OS_UNIX
			strcpy(log_file_write_mode, "w");
#else
			strcpy(log_file_write_mode, "wt");
#endif
		}
		else
		{
#if FF_OS == FF_OS_UNIX
			strcpy(log_file_write_mode, "a");
#else
			strcpy(log_file_write_mode, "at");
#endif
		}
	}
	else if (chkform_log)
	{
#if FF_OS == FF_OS_UNIX
			strcpy(log_file_write_mode, "w");
#else
			strcpy(log_file_write_mode, "wt");
#endif
	}

	if (chkform_log)
	{
		FILE *fp = NULL;

		fp = fopen(std_args->log_file, log_file_write_mode);
		if (fp)
		{
			size_t bytes_written = fwrite(chkform_log->buffer, 1, (size_t)chkform_log->bytes_used, fp);

			if (bytes_written != (size_t)chkform_log->bytes_used)
				error = err_push(ERR_WRITE_FILE, "Wrote %d bytes of %d to %s", (int)bytes_written, (int)chkform_log->bytes_used, std_args->log_file);

			fclose(fp);
		}
		else
			error = err_push(ERR_CREATE_FILE, std_args->log_file);

		ff_destroy_bufsize(chkform_log);
	}

	if (std_args)
		ff_destroy_std_args(std_args);
	
	memExit(error ? EXIT_FAILURE : EXIT_SUCCESS, "main");
}

static long total_bytes_to_process(DATA_BIN_PTR dbin)
{
	int error = 0;
	long bytes_to_process = 0;

	PROCESS_INFO_LIST pinfo_list = NULL;
	PROCESS_INFO_PTR pinfo = NULL;

	FF_VALIDATE(dbin);

	if (!fd_get_header(dbin, FFF_REC))
	{
		error = db_ask(dbin, DBASK_PROCESS_INFO, FFF_INPUT | FFF_DATA, &pinfo_list);
		if (error)
			return -1;

		pinfo_list = dll_first(pinfo_list);
		pinfo      = FF_PI(pinfo_list);
		while (pinfo)
		{
			bytes_to_process += PINFO_SUPER_ARRAY_BYTES(pinfo);

			pinfo_list = dll_next(pinfo_list);
			pinfo      = FF_PI(pinfo_list);
		}

		ff_destroy_process_info_list(pinfo_list);
	}

	return(bytes_to_process);
}

static FF_NDX_t scan_for_nonprintable(char *num_as_str, FF_NDX_t end_pos)
{
	FF_NDX_t i = 0;

	for (i = 0; i < end_pos; i++)
	{
		if ((!isspace((unsigned int)num_as_str[i]) 
		     && !isprint((unsigned int)num_as_str[i]))
		    || num_as_str[i] >= 0x7F)
			return(i);
	}

	return(end_pos);
}
 
static int chkform
	(
	 time_t start_time,
	 FF_STD_ARGS_PTR std_args,
	 DATA_BIN_PTR dbin,
	 FF_BUFSIZE_PTR chkform_log
	)
{
	char num_as_str[MAX_PV_LENGTH + 1];
	int error = 0;
	int percent_done = 0;

	long bytes_remaining = 0;
	long total_bytes = 0L;

	BOOLEAN done_processing = FALSE;

	long elapsed_time = 0;
	time_t finish_time = 0;

	unsigned long line_count = 0;
	unsigned long sep_line_count = 0;
	unsigned long total_errors = 0;

	FF_VALIDATE(dbin);
	
	total_bytes = total_bytes_to_process(dbin);

	while (!error && !done_processing)
	{
		error = db_do(dbin, DBDO_READ_FORMATS, FFF_INPUT);
		if (error == EOF)
		{
			error = 0;
			done_processing = TRUE;
		}

		if (!error)
			error = do_checks(dbin, chkform_log, &line_count, &sep_line_count, &total_errors);
	
		/* Calculate percentage left to process and display */
		bytes_remaining = db_ask(dbin, DBASK_BYTES_TO_PROCESS, FFF_INPUT);

		percent_done = (int)((1 - ((float)bytes_remaining / total_bytes)) * 100);
		wfprintf(stderr,"\r%3d%% processed", percent_done);

		(void)time(&finish_time);
		elapsed_time = (long)difftime(finish_time, start_time);
		wfprintf(stderr, "     Elapsed time - %02d:%02d:%02d",
		        (int)(elapsed_time / (3600)),
		        (int)((elapsed_time / 60) % 60),
		        (int)(elapsed_time % 60)
		);

		if (!error && std_args->user.is_stdin_redirected)
			error = db_do(dbin, DBDO_READ_STDIN, std_args, &done_processing);

		if (!error && !done_processing && std_args->user.is_stdin_redirected)
		{
			FF_BUFSIZE_PTR bufsize = NULL;

			/* Display some information about the data, ignore errors */
			(void) db_ask(dbin, DBASK_FORMAT_SUMMARY, &bufsize);
			do_log(chkform_log, "%s\n", bufsize->buffer);
			ff_destroy_bufsize(bufsize);
		}
	}
	
	/* End Processing */
	
	bytes_remaining = db_ask(dbin, DBASK_BYTES_TO_PROCESS, FFF_INPUT);
	if (bytes_remaining)
		error = err_push(ERR_PROCESS_DATA + ERR_WARNING_ONLY, "%ld BYTES OF DATA NOT READ.", bytes_remaining);

	if (chkform_log)
	{
		do_log(chkform_log, "%3d%% processed     Elapsed time - %02d:%02d:%02d\n",
		            percent_done,
		            (int)(elapsed_time / (3600)),
		            (int)((elapsed_time / 60) % 60),
		            (int)(elapsed_time % 60)
		           );
	}

	if (total_errors)
		sprintf(num_as_str, "%lu", total_errors);
	else
		sprintf(num_as_str, "No");

	do_log(chkform_log, "\n\n%s error%s found (%s%lu line%s checked)\n", num_as_str, total_errors == 1 ? "" : "s", sep_line_count ? "A total of " : "", sep_line_count + line_count, sep_line_count + line_count == 1 ? "" : "s");

	return(error);
}

static int do_checks
	(
	 DATA_BIN_PTR dbin,
	 FF_BUFSIZE_PTR chkform_log,
	 unsigned long *line_count,
	 unsigned long *sep_line_count,
	 unsigned long *total_errors
	)
{
	int error = 0;
	int last_error = 0;
	PROCESS_INFO_LIST pinfo_list = NULL;
	PROCESS_INFO_PTR input_pinfo = NULL;
	
	FF_VALIDATE(dbin);
	
	error = db_ask(dbin, DBASK_PROCESS_INFO, FFF_INPUT, &pinfo_list);
	if (error)
		return error;

	pinfo_list = dll_first(pinfo_list);
	input_pinfo = FF_PI(pinfo_list);

	while (input_pinfo)
	{
#if 0
		PROCESS_INFO_PTR pinfo = NULL;
#endif
		FF_VALIDATE(input_pinfo);

		if (PINFO_NEW_RECORD(input_pinfo))
		{
			error = check_buffer(PINFO_FORMAT(input_pinfo), PINFO_BUFFER(input_pinfo), PINFO_BYTES_USED(input_pinfo), chkform_log, IS_SEPARATE(PINFO_FORMAT(input_pinfo)) ? sep_line_count : line_count, total_errors);
			if (error)
				last_error = error;

			PINFO_NEW_RECORD(input_pinfo) = 0;
		}

		pinfo_list = dll_next(pinfo_list);
		input_pinfo = FF_PI(pinfo_list);
	} /* while input_format_data and the right type */

	ff_destroy_process_info_list(pinfo_list);

	return last_error;
}

static int check_buffer
	(
	 FORMAT_PTR format,
	 char *buffer,
	 unsigned long num_bytes,
	 FF_BUFSIZE_PTR chkform_log,
	 unsigned long *line_count,
	 unsigned long *total_errors
	)
{
	FF_BUFSIZE_PTR markers = 0;

	unsigned long recs_in_buf = 0;
	unsigned long i;

	int line_error_count = 0;

	VARIABLE_PTR    var        = NULL;
	VARIABLE_LIST   v_list     = NULL;
	char *record     = NULL;

	char *num_as_str = NULL;

	markers = ff_create_bufsize(format->length);
	if (!markers)
		return(err_push(ERR_MEM_LACK, "cannot allocate %d byte bufsize", format->length));

	num_as_str = (char *)memMalloc(format->length + 1, "num_as_str");
	if (!num_as_str)
		return err_push(ERR_MEM_LACK, "Cannot allocate %d byte string", format->length + 1);

	record = buffer;

	recs_in_buf = num_bytes / format->length;

	for (i = 0; i < recs_in_buf; i++)
	{
		memset(markers->buffer, ' ', format->length);

		(*line_count)++;

		v_list = dll_first(format->variables);

		var = FF_VARIABLE(v_list);
		while (var)
		{ 
			double dvalue = 0.0;
			double dest_value = 0;
			char *endptr = NULL;
			int error = 0;

			FF_NDX_t null_pos = 0;

			memcpy(num_as_str, record + var->start_pos - 1, FF_VAR_LENGTH(var));
			num_as_str[FF_VAR_LENGTH(var)] = STR_END;

			null_pos = scan_for_nonprintable(num_as_str, FF_VAR_LENGTH(var));
			if (null_pos != FF_VAR_LENGTH(var))
			{
				error = FOUND_NON_PRINT;
				memset(markers->buffer + var->start_pos - 1, '*', FF_VAR_LENGTH(var));
				memset(markers->buffer + var->start_pos + null_pos - 1, '^', 1);
			}

			if (IS_REAL(var) || IS_INTEGER(var))
			{
				if (!error)
				{
					errno = 0;

					if (IS_REAL(var))
						dvalue = strtod(num_as_str, &endptr);
					else if (IS_INTEGER(var))
					{
						if (IS_UNSIGNED(var))
							dvalue = strtoul(num_as_str, &endptr, 10);
						else
							dvalue = strtol(num_as_str, &endptr, 10);
					}
				
					if (FF_STRLEN(endptr))
					{
						memset(markers->buffer + var->start_pos - 1, '*', FF_VAR_LENGTH(var));
						memset(markers->buffer + var->end_pos - FF_STRLEN(endptr), '^', 1);
						error = BAD_NUMBER;
					}
					else
					{
						error = btype_to_btype(&dvalue, FFV_DOUBLE, &dest_value, FFV_DATA_TYPE(var));
						if (error || errno)
						{
							if (error)
								err_clear();

							if (errno)
								errno = 0;

							error = RANGE_ERROR;
							memset(markers->buffer + var->start_pos - 1, '*', FF_VAR_LENGTH(var));
						}
					}

					if (!error && IS_REAL(var) && var->precision)
					{
						if (num_as_str[FF_VAR_LENGTH(var) - var->precision - 1] != '.')
						{
							error = BAD_DECIMAL_POINT;
							memset(markers->buffer + var->start_pos - 1, '*', FF_VAR_LENGTH(var));
							memset(markers->buffer + var->end_pos - var->precision - 1, '^', 1);
						}
					}
				}
			}/* if (FFV_DATA_TYPE(var)) */

			if (error)
			{
				do_log(chkform_log, "Error on line %lu: %s -- %s\n", *line_count, var->name, messages[error - 1]);

				++line_error_count;
			}

			v_list = dll_next(v_list);
			var = FF_VARIABLE(v_list);
		}/* end variable loop */

		/* Record has been checked, output variable and record information if error */
		if (line_error_count)
		{
			print_positions(chkform_log, (int)FORMAT_LENGTH(format) - (IS_ASCII(format) ? FF_VAR_LENGTH(FF_VARIABLE(dll_previous(format->variables))) : 0));

			print_buffer(chkform_log, record, (int)FORMAT_LENGTH(format) - (IS_ASCII(format) ? FF_VAR_LENGTH(FF_VARIABLE(dll_previous(format->variables))) : 0));
			print_buffer(chkform_log, markers->buffer, (int)markers->total_bytes);
			print_buffer(chkform_log, "", 0);
			
			*total_errors += line_error_count;
			line_error_count = 0;
		}

		record += format->length;
	}

	memFree(num_as_str, "num_as_str");
	ff_destroy_bufsize(markers);
	return 0;
}
