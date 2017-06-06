/* FILENAME:  newform.c
 *
 * CONTAINS:  newform()
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

#ifdef TIMER
#include <time.h>
#endif 

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
	{
		error = err_push(ERR_IGNORED_OPTION, "precision (checkvar only)");
	}
	
	if (std_args->cv_maxbins)
	{
		error = err_push(ERR_IGNORED_OPTION, "maximum number of histogram bins (checkvar only)");
	}
	
	if (std_args->user.is_stdin_redirected && std_args->records_to_read)
	{
		error = err_push(ERR_IGNORED_OPTION, "Records to read when redirecting standard input");
	}
	
	return(error);
}
	
int main(int argc, char *argv[])
{
	FF_BUFSIZE_PTR newform_log = NULL;

	char log_file_write_mode[4];
  
	int error = 0;
	FF_STD_ARGS_PTR std_args = NULL;

	std_args = ff_create_std_args();
	if (!std_args)
	{
		fprintf(stderr, "Insufficient memory -- free more memory and try again");
		error = ERR_MEM_LACK;
		goto main_exit;
	}

	error = parse_command_line(argc, argv, std_args);
	if (error)
		goto main_exit;

	if (std_args->log_file)
	{
		newform_log = ff_create_bufsize(SCRATCH_QUANTA);
		if (!newform_log)
		{
			error = ERR_MEM_LACK;
			goto main_exit;
		}
	}

	error = check_for_unused_flags(std_args);
	if (error)
		goto main_exit;

	error = newform(std_args, newform_log, stderr);
	
	fprintf(stderr,"\n");

	/* Is user asking for both error logging and a log file? */
	if (std_args->error_log && newform_log)
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
	else if (newform_log)
	{
#if FF_OS == FF_OS_UNIX
			strcpy(log_file_write_mode, "w");
#else
			strcpy(log_file_write_mode, "wt");
#endif
	}

	if (newform_log)
	{
		FILE *fp = NULL;

		fp = fopen(std_args->log_file, log_file_write_mode);
		if (fp)
		{
			size_t bytes_written = fwrite(newform_log->buffer, 1, (size_t)newform_log->bytes_used, fp);

			if (bytes_written != (size_t)newform_log->bytes_used)
				error = err_push(ERR_WRITE_FILE, "Wrote %d bytes of %d to %s", (int)bytes_written, (int)newform_log->bytes_used, std_args->log_file);

			fclose(fp);
		}
		else
			error = err_push(ERR_CREATE_FILE, std_args->log_file);

		ff_destroy_bufsize(newform_log);
	}

	if (std_args->user.is_stdin_redirected)
		ff_destroy_bufsize(std_args->input_bufsize);

main_exit:

	if (error || err_state())
		error = err_disp(std_args);

	if (std_args)
		ff_destroy_std_args(std_args);
	
	memExit(error && error < ERR_WARNING_ONLY ? EXIT_FAILURE : EXIT_SUCCESS, "main");
}

