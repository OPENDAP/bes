/* FILENAME:  bufform.c
 *
 * CONTAINS:  main
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

	FILE *ofp = NULL;

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

	if (std_args->input_file)
	{
		error = ff_file_to_bufsize(std_args->input_file, &std_args->input_bufsize);
		if (error)
			goto main_exit;

		std_args->input_file = NULL;
	}

	if (std_args->output_file)
	{
		/* how big? */
		std_args->output_bufsize = ff_create_bufsize(10000000);
		if (!std_args->output_bufsize)
		{
			error = err_push(ERR_MEM_LACK, "");
			goto main_exit;
		}

		ofp = fopen(std_args->output_file, "wb");
		if (!ofp)
		{
			error = err_push(ERR_CREATE_FILE, "%s", std_args->output_file);
			goto main_exit;
		}

		std_args->output_file = NULL;
	}

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

	if (fwrite(std_args->output_bufsize->buffer, std_args->output_bufsize->bytes_used, 1, ofp) != 1)
	{
		error = err_push(ERR_WRITE_FILE, "output data file");
		goto main_exit;
	}

	fclose(ofp);

main_exit:

	if (error || err_state())
		error = err_disp(std_args);

	if (std_args)
		ff_destroy_std_args(std_args);
	
	memExit(error && error < ERR_WARNING_ONLY ? EXIT_FAILURE : EXIT_SUCCESS, "main");
}

