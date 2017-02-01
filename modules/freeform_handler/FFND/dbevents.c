/* FILENAME: dbevents.c
 *
 * CONTAINS: db_do()
 *           write_output_fmt_file (a module-level function to db_do)
 *           read_formats
 *           convert_formats
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
 * NAME:        db_do
 *              
 * PURPOSE:     Handle events for data bins.
 *
 * USAGE:       db_do(DATA_BIN_PTR, event, [args])
 *
 * RETURNS:     0 if all goes well.
 *
 * DESCRIPTION: This function handles events which are sent to data bins.
 * The function processes a list of argument groups which have
 * the form: event [arguments].
 * The presently supported events and their arguments are:
 *
 *
 * ERRORS:
 *
 * SYSTEM DEPENDENT FUNCTIONS: 
 *
 * AUTHOR:      T. Habermann, NGDC, (303) 497 - 6472, haber@ngdc.noaa.gov
 *
 * COMMENTS: 
 *
 * KEYWORDS: databins
 *
*/

#include <freeform.h>

static int restore_input_eqn_bufsize(PROCESS_INFO_PTR pinfo)
{
	BOOLEAN has_eqn_vars = FALSE;
	int error = 0;

	FF_NDX_t file_record_length = 0;
	FF_BSS_t new_size = 0;
	unsigned long num_recs = 0;

	VARIABLE_LIST vlist = NULL;
	VARIABLE_PTR var = NULL;

	FF_VALIDATE(pinfo);

	/* first time in? */
	if (!PINFO_BYTES_USED(pinfo))
		return 0;

	num_recs = PINFO_BYTES_USED(pinfo) / PINFO_RECL(pinfo);

	vlist = FFV_FIRST_VARIABLE(PINFO_FORMAT(pinfo));
	var = FF_VARIABLE(vlist);
	while (var)
	{
		FF_VALIDATE(var);

		file_record_length += FF_VAR_LENGTH(var);

		/* skip equation variable companion; it has an inflated size */
		if (IS_EQN(var))
		{
			has_eqn_vars = TRUE;
			vlist = dll_next(vlist);
		}

		vlist = dll_next(vlist);
		var = FF_VARIABLE(vlist);
	}

	if (!has_eqn_vars)
		return 0;

	new_size = num_recs * file_record_length;

	if (new_size + 1 != PINFO_TOTAL_BYTES(pinfo))
	{
		error = ff_resize_bufsize(new_size + 1, &PINFO_DATA(pinfo));
		if (error)
			return error;
	}

	PINFO_BYTES_USED(pinfo) = new_size;

	/* Fudge format length for nt_ask since we're changing buffer size */
	PINFO_FORMAT(pinfo)->length = file_record_length;

	return 0;
}

/*****************************************************************************
 * NAME:  resize_for_input_eqn
 *
 * PURPOSE:
 *
 * USAGE:
 *
 * RETURNS:
 *
 * DESCRIPTION:
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

static int resize_for_input_eqn(PROCESS_INFO_PTR pinfo)
{
	BOOLEAN has_eqn_vars = FALSE;
	int error = 0;

	unsigned long rec = 0;
	unsigned long num_recs = 0;
	FF_BSS_t new_size = 0;

	FORMAT_PTR format = NULL;

	VARIABLE_LIST vlist = NULL;
	VARIABLE_PTR var = NULL;

	VARIABLE_LIST nvlist = NULL;
	VARIABLE_PTR nvar = NULL;

	FF_VALIDATE(pinfo);

	vlist = FFV_FIRST_VARIABLE(PINFO_FORMAT(pinfo));
	var = FF_VARIABLE(vlist);
	while (var)
	{
		FF_VALIDATE(var);

		if (IS_EQN(var))
		{
			FF_NDX_t bytes_per_pixel = 0;
			VARIABLE_PTR var0 = FF_VARIABLE(dll_next(vlist));

			FF_VALIDATE(var0);

			has_eqn_vars = TRUE;

			var0->type = FFV_DOUBLE;

			/* watch precision!!! */
			if (IS_BINARY(PINFO_FORMAT(pinfo)))
				bytes_per_pixel = sizeof(double);
			else
				bytes_per_pixel = ffv_ascii_type_size(var0);

			if (bytes_per_pixel == FF_VAR_LENGTH(var0))
			{
				/* format length has been fudged in restore_input_eqn_bufsize; compensate */
				PINFO_FORMAT(pinfo)->length += bytes_per_pixel - FF_VAR_LENGTH(var);

				update_format_var(var->type & ~FFV_EQN, FF_VAR_LENGTH(var), var0, PINFO_FORMAT(pinfo));
			}
		}

		vlist = dll_next(vlist);
		var = FF_VARIABLE(vlist);
	}

	if (!has_eqn_vars)
		return 0;

	format = ff_copy_format(PINFO_FORMAT(pinfo));
	if (!format)
		return ERR_MEM_LACK;

	vlist = FFV_FIRST_VARIABLE(PINFO_FORMAT(pinfo));
	var = FF_VARIABLE(vlist);
	while (var)
	{
		FF_VALIDATE(var);

		if (IS_EQN(var))
		{
			FF_NDX_t bytes_per_pixel = 0;
			VARIABLE_PTR var0 = FF_VARIABLE(dll_next(vlist));

			FF_VALIDATE(var0);

			has_eqn_vars = TRUE;

			/* watch precision!!! */
			if (IS_BINARY(PINFO_FORMAT(pinfo)))
				bytes_per_pixel = sizeof(double);
			else
			{
				var0->type = FFV_DOUBLE; /* var0->type gets restored in update_format_var below */

				bytes_per_pixel = ffv_ascii_type_size(var0);
			}

			if (bytes_per_pixel != FF_VAR_LENGTH(var0))
				update_format_var(var->type & ~FFV_EQN, bytes_per_pixel, var0, PINFO_FORMAT(pinfo));
		}

		vlist = dll_next(vlist);
		var = FF_VARIABLE(vlist);
	}

	num_recs = PINFO_BYTES_USED(pinfo) / FORMAT_LENGTH(format);
	new_size = num_recs * PINFO_RECL(pinfo);

	if (new_size + 1 != PINFO_TOTAL_BYTES(pinfo))
	{
		error = ff_resize_bufsize(new_size + 1, &PINFO_DATA(pinfo));
		if (error)
			return error;
	}

	PINFO_BYTES_USED(pinfo) = new_size;

	for (rec = num_recs; rec; rec--)
	{
		nvlist = dll_last(PINFO_FORMAT(pinfo)->variables);
		nvar = FF_VARIABLE(nvlist);
		vlist = dll_last(format->variables);
		var = FF_VARIABLE(vlist);
		while (var)
		{
			FF_VALIDATE(var);

			memmove(PINFO_BUFFER(pinfo) + ((rec - 1) * PINFO_RECL(pinfo)) + nvar->start_pos - 1,
			        PINFO_BUFFER(pinfo) + ((rec - 1) * FORMAT_LENGTH(format)) + var->start_pos - 1,
			        FF_VAR_LENGTH(var)
			       );

			nvlist = dll_previous(nvlist);
			nvar = FF_VARIABLE(nvlist);
			vlist = dll_previous(vlist);
			var = FF_VARIABLE(vlist);
		}
	}

	ff_destroy_format(format);

	return error;
}

static int calculate_input(PROCESS_INFO_PTR pinfo)
{
	int error = 0;

	unsigned long rec = 0;

	VARIABLE_LIST vlist = NULL;
	VARIABLE_PTR var = NULL;

	FF_VALIDATE(pinfo);

	error = resize_for_input_eqn(pinfo);
	if (error)
		return error;

	vlist = FFV_FIRST_VARIABLE(PINFO_FORMAT(pinfo));
	var = FF_VARIABLE(vlist);
	while (var)
	{
		FF_VALIDATE(var);

		if (IS_EQN(var))
		{
			for (rec = PINFO_BYTES_USED(pinfo) / PINFO_RECL(pinfo); rec; rec--)
			{
				double d = 0;

				error = calculate_variable(var, PINFO_FORMAT(pinfo), PINFO_BUFFER(pinfo) + (rec - 1) * PINFO_RECL(pinfo), &d);
				if (error)
					return error;

				memset(PINFO_BUFFER(pinfo) + ((rec - 1) * PINFO_RECL(pinfo)) + FF_VARIABLE(dll_next(vlist))->start_pos - 1,
				       IS_BINARY(PINFO_FORMAT(pinfo)) ? 0 : ' ',
				       FF_VAR_LENGTH(FF_VARIABLE(dll_next(vlist)))
				      );

				FF_VARIABLE(dll_next(vlist))->type = FFV_DOUBLE; /* now it's a double */

				error = ff_put_binary_data(FF_VARIABLE(dll_next(vlist)), &d, sizeof(d), FFV_DOUBLE, PINFO_BUFFER(pinfo) + (rec - 1) * PINFO_RECL(pinfo) + var->start_pos - 1, FFF_TYPE(PINFO_FORMAT(pinfo)));
				if (error)
					return error;
			}
		}

		vlist = dll_next(vlist);
		var = FF_VARIABLE(vlist);
	}

	return error;
}

#ifdef ROUTINE_NAME
#undef ROUTINE_NAME
#endif
#define ROUTINE_NAME "write_output_fmt_file"

/*****************************************************************************
 * NAME: write_output_fmt_file
 *
 * PURPOSE:  Write a format file for output data file
 *
 * USAGE:  error = write_output_fmt_file(dbin, fmt_filename);
 *
 * RETURNS:  Zero on success, error code on failure.
 *
 * DESCRIPTION:  Write the dbin's output formats as input formats in fmt_filename.
 * (The calling routine should ensure that writing to fmt_filename does not
 *  overwrite the input data file's format file.)
 *
 * Write the input equivalence section as an input equivalence section in
 * fmt_filename.  This is temporary.  After setdbin.c:check_dynamic_output_header_format
 * uses output eqv's to restructure dynamic headers, then write the output
 * eqv as the input eqv in fmt_filename.
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

static int write_output_fmt_file(DATA_BIN_PTR dbin, char *fmtfname)
{
	NAME_TABLE_PTR table = NULL;

	PROCESS_INFO_LIST plist = NULL;
	PROCESS_INFO_PTR pinfo = NULL;
	int error = 0;
	
	FF_BUFSIZE_PTR bufsize = NULL;

	FF_VALIDATE(dbin);

	bufsize = ff_create_bufsize(SCRATCH_QUANTA);
	if (!bufsize)
		return ERR_MEM_LACK;

	table = fd_find_format_data(dbin->table_list, FFF_GROUP, FFF_TABLE | FFF_OUTPUT);
	if (table)
	{
		FF_VALIDATE(table);

		strcpy(bufsize->buffer + bufsize->bytes_used, "input_eqv\n");
		bufsize->bytes_used += strlen(bufsize->buffer + bufsize->bytes_used);

		error = nt_show(table, bufsize);
		if (error)
			return error;

		strcpy(bufsize->buffer + bufsize->bytes_used, "\n");
		bufsize->bytes_used += strlen(bufsize->buffer + bufsize->bytes_used);
	}

	error = db_ask(dbin, DBASK_PROCESS_INFO, FFF_OUTPUT, &plist);
	if (error)
		return(error);
	
	plist = dll_first(plist);
	pinfo = FF_PI(plist);
	while (pinfo)
	{
		FF_VALIDATE(pinfo);

		PINFO_TYPE(pinfo) &= ~FFF_OUTPUT;
		PINFO_TYPE(pinfo) |=   FFF_INPUT;
				
		plist = dll_next(plist);
		pinfo = FF_PI(plist);
	}
	
	error = db_ask(dbin, DBASK_FORMAT_LIST_DESCRIPTION_TO_USER, plist, bufsize);
	if (error)
		return(error);

	plist = dll_first(plist);
	pinfo = FF_PI(plist);
	while (pinfo)
	{
		FF_VALIDATE(pinfo);

		PINFO_TYPE(pinfo) &= ~FFF_INPUT;
		PINFO_TYPE(pinfo) |=   FFF_OUTPUT;
				
		plist = dll_next(plist);
		pinfo = FF_PI(plist);
	}
	
	ff_destroy_process_info_list(plist);

	error = ff_bufsize_to_textfile_overwrite(fmtfname, bufsize);
	
	ff_destroy_bufsize(bufsize);

	return(error);
}

static int ff_lock__
	(
	 PROCESS_INFO_PTR pinfo,
	 void **hbuffer,
	 unsigned long *pused,
	 unsigned long *pcapacity
	)
{
	int error = 0;

	if (hbuffer)
		*hbuffer = NULL;

	if (pused)
		*pused = 0;

	if (pcapacity)
		*pcapacity = 0;

	if (PINFO_LOCKED(pinfo))
		error = err_push(ERR_API_BUF_LOCKED, "");
	else
	{
		if (hbuffer)
		{
			*hbuffer = (char HUGE *)PINFO_BUFFER(pinfo);

#ifdef FF_CHK_ADDR
			pinfo->locked_buffer = hbuffer;
#endif
		}
		
		if (pused/* && PINFO_NEW_RECORD(pinfo)*/)
			*pused = (unsigned long)PINFO_BYTES_USED(pinfo);
		
		if (pcapacity)
			*pcapacity = (unsigned long)PINFO_TOTAL_BYTES(pinfo) - 1;

		PINFO_LOCKED(pinfo) = 1;
	}
		
	return(error);
}

static int ff_unlock__
	(
	 PROCESS_INFO_PTR pinfo,
	 void **hbuffer,
	 long size
	)
{
	int error = 0;

	if (!PINFO_LOCKED(pinfo))
		error = ERR_API_BUF_NOT_LOCKED;
	else
	{
		if (hbuffer)
		{
#ifdef FF_CHK_ADDR
			assert((char HUGE *)hbuffer == (char HUGE *)pinfo->locked_buffer);
#endif
			*hbuffer = NULL;
		}
   
		PINFO_LOCKED(pinfo) = 0;
		
		if (size)
		{
			PINFO_BYTES_USED(pinfo) = (FF_BSS_t)size;
			PINFO_NEW_RECORD(pinfo) = 1; /* buffer has been filled */
		}
		else
			PINFO_NEW_RECORD(pinfo) = 0;
	}
		
	return(error);
}

int ff_unlock
	(
	 PROCESS_INFO_PTR pinfo,
	 void **hbuffer
	)
{
	int error = 0;

	assert(hbuffer);
	assert(*hbuffer);
	
	error = ff_unlock__(pinfo, hbuffer, 0L);
	return(error);
}

/*****************************************************************************
 * NAME: ff_lock
 *
 * PURPOSE: To lock a buffer of data for use
 *
 * USAGE:  error = ff_lock(pinfo, &buffer, &size);
 *
 * RETURNS:  Zero on success, an error code if buffer cannot be locked.
 *
 * DESCRIPTION:  buffer and size will be set according to information in
 * pinfo.  pinfo is a format-info item that is an element in a list of
 * format-info's created by calling db_show, e.g.,
 * error = db_show(dbin, SHOW_FORMAT_INFO, &plist);
 *
 * Call ff_unlock to release the buffer.  Processing cannot continue until
 * the buffer is unlocked, as its memory must be reused.  This means that
 * while you have the buffer locked this is your one chance to access its
 * data.  Attempting to lock the buffer again after unlocking it will either
 * fail, or will lock the buffer with the buffer containing different data.
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

int ff_lock
	(
	 PROCESS_INFO_PTR pinfo,
	 void **hbuffer,
	 unsigned long *psize
	)
{
	int error = 0;
	
	assert(hbuffer);
	assert(psize);

	error = ff_lock__(pinfo, hbuffer, psize, NULL);
	if (error)
		return(error);
	
	return(0);
}

#ifdef ROUTINE_NAME
#undef ROUTINE_NAME
#endif
#define ROUTINE_NAME "write_formats"

/*****************************************************************************
 * NAME:
 *
 * PURPOSE:
 *
 * USAGE:
 *
 * RETURNS:
 *
 * DESCRIPTION:
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

static int write_formats
	(
	 DATA_BIN_PTR dbin,
	 FF_TYPES_t format_type
	)
{
	unsigned long bytes_to_write = 0;
	unsigned long bytes_written = 0;

	int error = 0;

	PROCESS_INFO_LIST plist = NULL;
	PROCESS_INFO_PTR output_pinfo = NULL;
	
	FF_VALIDATE(dbin);

	error = db_ask(dbin, DBASK_PROCESS_INFO, FFF_OUTPUT | format_type, &plist);
	if (error)
		return error;

	plist = dll_first(plist);
	output_pinfo = FF_PI(plist);
	while (output_pinfo && !error)
	{
		void *buffer = NULL;
		
		FF_VALIDATE(output_pinfo);
		
		if (!PINFO_NEW_RECORD(output_pinfo) || (IS_ARRAY(PINFO_FORMAT(output_pinfo)) && PINFO_ARRAY_DONE(output_pinfo)))
		{
			plist = dll_next(plist);
			output_pinfo = FF_PI(plist);
						
			continue;
		}

		error = ff_lock(output_pinfo, &buffer, &bytes_to_write);
		if (error)
			return(error);
		
		if (PINFO_IS_FILE(output_pinfo) || PINFO_LOCUS_BUFSIZE(output_pinfo))
		{
			bytes_written = ndarr_reorient(PINFO_ARRAY_MAP(output_pinfo),
			                               NDARRS_BUFFER,
			                               buffer,
			                               bytes_to_write,
			                               PINFO_ID(output_pinfo),
			                               PINFO_IS_FILE(output_pinfo) ? PINFO_FNAME(output_pinfo) : PINFO_LOCUS_BUFFER(output_pinfo) + PINFO_CURRENT_ARRAY_OFFSET(output_pinfo),
			                               PINFO_IS_FILE(output_pinfo) ? PINFO_CURRENT_ARRAY_OFFSET(output_pinfo) : PINFO_LOCUS_SIZE(output_pinfo),
			                               &PINFO_ARRAY_DONE(output_pinfo)
			                              );
		}
		else
			bytes_written = fwrite(buffer, 1, (size_t)bytes_to_write, stdout);

		/* bytes_written might be more than bytes_to_write if separation on output */
		if ((long)bytes_written == -1 || bytes_written < bytes_to_write)
			error = err_push(ERR_WRITE_FILE, "Writing to \"%s\" (processing \"%s\")", PINFO_IS_FILE(output_pinfo) ? PINFO_FNAME(output_pinfo) : "buffer", PINFO_NAME(output_pinfo));
		else if (!PINFO_IS_FILE(output_pinfo) && PINFO_LOCUS_BUFSIZE(output_pinfo))
			PINFO_LOCUS_FILLED(output_pinfo) += min(bytes_written, bytes_to_write);

		ff_unlock(output_pinfo, &buffer);

		PINFO_BYTES_LEFT(output_pinfo) -= min(bytes_written, bytes_to_write);

		if (IS_DATA(PINFO_FORMAT(output_pinfo)) && PINFO_ARRAY_DONE(output_pinfo) && !IS_ARRAY(PINFO_FORMAT(output_pinfo)))
		{
			PROCESS_INFO_LIST rech_plist = NULL;

			if (!db_ask(dbin, DBASK_PROCESS_INFO, FFF_OUTPUT | FFF_REC | FFF_HEADER, &rech_plist))
			{
				PROCESS_INFO_PTR rech_pinfo = FF_PI(dll_first(rech_plist));

				if (rech_pinfo && output_pinfo && IS_REC_HEADER_TYPE(PINFO_TYPE(rech_pinfo)))
				{
					if (IS_SEPARATE_TYPE(PINFO_TYPE(rech_pinfo)))
					{
						assert(PINFO_IS_FILE(rech_pinfo));

						if (PINFO_IS_FILE(output_pinfo))
							PINFO_CURRENT_ARRAY_OFFSET(output_pinfo) += PINFO_SUB_ARRAY_BYTES(output_pinfo);
						else
							assert(PINFO_IS_FILE(output_pinfo));
					}
					else
					{
						if (1 || PINFO_IS_FILE(rech_pinfo))
							PINFO_CURRENT_ARRAY_OFFSET(rech_pinfo) += PINFO_SUB_ARRAY_BYTES(output_pinfo);
						else
							assert(PINFO_IS_FILE(rech_pinfo));

						if (1 || PINFO_IS_FILE(output_pinfo))
							PINFO_CURRENT_ARRAY_OFFSET(output_pinfo) = PINFO_CURRENT_ARRAY_OFFSET(rech_pinfo) + PINFO_RECL(rech_pinfo);
						else
							assert(PINFO_IS_FILE(output_pinfo));
					}
				}
				else if (output_pinfo)
				{
					if (1 || PINFO_IS_FILE(output_pinfo))
						PINFO_CURRENT_ARRAY_OFFSET(output_pinfo) += PINFO_SUB_ARRAY_BYTES(output_pinfo);
					else
					{
						assert(PINFO_IS_FILE(output_pinfo));
					}
				}

				ff_destroy_process_info_list(rech_plist);
			}
		} /* if IS_DATA(PINFO_FORMAT(output_pinfo)) && PINFO_ARRAY_DONE(output_pinfo) && !IS_ARRAY(PINFO_FORMAT(output_pinfo)) */
		else if (IS_REC(PINFO_FORMAT(output_pinfo))) /* This can be done better -- maybe focus on record headers instead of data */
		{
			if (1 || PINFO_IS_FILE(output_pinfo))
				PINFO_CURRENT_ARRAY_OFFSET(output_pinfo) += PINFO_RECL(output_pinfo);
		}

		plist = dll_next(plist);
		output_pinfo = FF_PI(plist);
	} /* End of format_list processing (while) */

	ff_destroy_process_info_list(plist);

	return(error);
}

#ifdef ROUTINE_NAME
#undef ROUTINE_NAME
#endif
#define ROUTINE_NAME "dbin_convert_data"

static int dbin_convert_data(FORMAT_DATA_MAPPING_PTR format_data_mapping)
{
	int error;
	
	long output_bytes = 0L;

	FF_VALIDATE(format_data_mapping);

	output_bytes = (format_data_mapping->input->data->bytes_used /
	                FORMAT_LENGTH(format_data_mapping->input->format)
	               ) * FORMAT_LENGTH(format_data_mapping->output->format);
	if ((unsigned long)output_bytes > (unsigned long)UINT_MAX)
		return(err_push(ERR_MEM_LACK, "reallocation size for output data is too big"));
					
	if ((FF_BSS_t)output_bytes >= format_data_mapping->output->data->total_bytes)
	{
		assert(!format_data_mapping->output->state.locked);
		
		if (ff_resize_bufsize(output_bytes + 1, &format_data_mapping->output->data))
			return(err_push(ERR_MEM_LACK, "reallocation of buffer"));
	}

	error = ff_process_format_data_mapping(format_data_mapping);
	if (error)
		format_data_mapping->output->data->bytes_used -= FORMAT_LENGTH(format_data_mapping->output->format);

	return(error);
}

#ifdef ROUTINE_NAME
#undef ROUTINE_NAME
#endif
#define ROUTINE_NAME "convert_formats"

static int convert_formats
	(
	 DATA_BIN_PTR dbin,
	 FF_TYPES_t format_type
	)
{ 
	int error = 0;
	
	PROCESS_INFO_LIST plist = NULL;
	PROCESS_INFO_PTR output_pinfo = NULL;
	PROCESS_INFO_PTR input_pinfo = NULL;
	
	format_type &= ~FFF_IO;
	format_type |= FFF_OUTPUT;

	error = db_ask(dbin, DBASK_PROCESS_INFO, format_type, &plist);
	if (error)
		return err_push(error, "No formats to convert");

	FF_VALIDATE(plist);
	
	plist = dll_first(plist);
	output_pinfo = FF_PI(plist);
	while (output_pinfo)
	{
		FF_VALIDATE(output_pinfo);

		input_pinfo = PINFO_MATE(output_pinfo);
		if (input_pinfo)
		{
			FF_VALIDATE(input_pinfo);
			
			if (PINFO_NEW_RECORD(input_pinfo) && !PINFO_NEW_RECORD(output_pinfo))
			{
				error = db_do(dbin, DBDO_CONVERT_DATA, PINFO_FORMAT_MAP(output_pinfo));
				PINFO_BYTES_DONE(output_pinfo) += PINFO_BYTES_USED(output_pinfo);
				if (error)
				{
					unsigned long record = 0;

					record = 1 + (PINFO_BYTES_DONE(output_pinfo) / PINFO_RECL(output_pinfo));

					err_push(ERR_CONVERT, "In %s %lu", IS_ARRAY(PINFO_FORMAT(output_pinfo)) ? "element" : "record", record);
					break;
				}

				error = db_do(dbin, DBDO_BYTE_SWAP, PINFO_FD(output_pinfo));
				if (error)
					break;

				PINFO_NEW_RECORD(input_pinfo) = 0;
			}
		}
		else if (!(PINFO_NEW_RECORD(output_pinfo) || (IS_FILE_HEADER(PINFO_FORMAT(output_pinfo)) && PINFO_ARRAY_DONE(output_pinfo))))
		{ /* Hack Hack Hack Hack Hack Hack Hack Hack Hack Hack Hack Hack Hack Hack */
			unsigned long output_bytes = 0L;

			output_bytes = PINFO_SUB_ARRAY_ELS(output_pinfo) * PINFO_RECL(output_pinfo);

			if (output_bytes + 1 > PINFO_TOTAL_BYTES(output_pinfo))
			{
				error = ff_resize_bufsize(output_bytes + 1, &PINFO_DATA(output_pinfo));
				if (error)
					break;
			}

			if (output_bytes != PINFO_BYTES_USED(output_pinfo))
			{
				PINFO_BYTES_USED(output_pinfo) = 0;
				while (output_bytes)
				{
					output_bytes -= PINFO_FORMAT_MAP(output_pinfo)->middle->data->bytes_used;
					memcpy(PINFO_BUFFER(output_pinfo) + output_bytes, PINFO_FORMAT_MAP(output_pinfo)->middle->data->buffer, PINFO_FORMAT_MAP(output_pinfo)->middle->data->bytes_used);
					PINFO_BYTES_USED(output_pinfo) += PINFO_FORMAT_MAP(output_pinfo)->middle->data->bytes_used;
				}
			}

			assert(PINFO_BYTES_USED(output_pinfo) == PINFO_SUB_ARRAY_ELS(output_pinfo) * PINFO_RECL(output_pinfo));

			PINFO_NEW_RECORD(output_pinfo) = 1;
		}

		plist = dll_next(plist);
		output_pinfo = FF_PI(plist);
	} /* while input_format_data and the right type */
	
	ff_destroy_process_info_list(plist);

	if (error)
		return error;

	format_type &= ~FFF_IO;
	format_type |= FFF_INPUT;

	error = db_ask(dbin, DBASK_PROCESS_INFO, format_type, &plist);
	if (error)
		return 0;

	FF_VALIDATE(plist);
	
	plist = dll_first(plist);
	input_pinfo = FF_PI(plist);
	while (input_pinfo)
	{
		FF_VALIDATE(input_pinfo);

		output_pinfo = PINFO_MATE(input_pinfo);

		if (!output_pinfo)
			PINFO_NEW_RECORD(input_pinfo) = 0;

		plist = dll_next(plist);
		input_pinfo = FF_PI(plist);
	}
	
	ff_destroy_process_info_list(plist);

	return(error);
}

static FF_NDX_t pinfo_file_recl(PROCESS_INFO_PTR pinfo)
{
	FF_NDX_t format_length = 0;

	VARIABLE_LIST vlist = NULL;
	VARIABLE_PTR var = NULL;

	FF_VALIDATE(pinfo);

	format_length = FORMAT_LENGTH(PINFO_FORMAT(pinfo));

	vlist = FFV_FIRST_VARIABLE(PINFO_FORMAT(pinfo));
	var = FF_VARIABLE(vlist);
	while (var)
	{
		FF_VALIDATE(var);

		if (IS_EQN(var))
		{
			/* resize_for_input_eqn increases format length by the difference
			   in size between the original equation variable type (e.g., short)
			   and the cloned variable type (i.e., double in either binary or
			   ASCII representation.

			   Therefore, subtract this difference from the format length.
			*/

			format_length += FF_VAR_LENGTH(var);

			vlist = dll_next(vlist);
			var = FF_VARIABLE(vlist);
			
			format_length -= FF_VAR_LENGTH(var);
		}

		vlist = dll_next(vlist);
		var = FF_VARIABLE(vlist);
	}

	return format_length;
}

#define IS_INPUT_FILE_HEADER(f) (IS_INPUT_TYPE(PINFO_TYPE(f)) && \
                                    IS_HEADER_TYPE(PINFO_TYPE(f)) && \
                                  IS_FILE_TYPE(PINFO_TYPE(f)) \
                                )
#define IS_INPUT_DATA(f) (IS_INPUT_TYPE(PINFO_TYPE(f)) && \
                          IS_DATA_TYPE(PINFO_TYPE(f)) \
                         )
#define WAIT_FOR_NEXT_RECORD_HEADER(f) (IS_INPUT_TYPE(PINFO_TYPE(f)) && \
                                        IS_HEADER_TYPE(PINFO_TYPE(f)) && \
                                        IS_REC_TYPE(PINFO_TYPE(f)) \
                                       )
static BOOLEAN need_to_read_next_record_header
	(
	 PROCESS_INFO_PTR pinfo,
	 DATA_BIN_PTR dbin
	)
{
	FF_VALIDATE(pinfo);
	FF_VALIDATE(dbin);

	if (IS_INPUT_TYPE(PINFO_TYPE(pinfo)) && 
	    IS_HEADER_TYPE(PINFO_TYPE(pinfo)) && 
	    IS_REC_TYPE(PINFO_TYPE(pinfo))
	   )
	{
		PROCESS_INFO_LIST plist = NULL;

		int error = 0;
		
		long record_count = 0;

		error = db_ask(dbin, DBASK_PROCESS_INFO, FFF_INPUT | FFF_REC, &plist);
		if (!error)
		{
			BOOLEAN answer = FALSE;

			if (!nt_ask(dbin, FFF_INPUT | FFF_REC, "count", FFV_LONG, &record_count) ||
				 !nt_ask(dbin, NT_INPUT, "record_count", FFV_LONG, &record_count)
				)
			{
				if (record_count == 0)
				{
 					PROCESS_INFO_PTR rech_pinfo = NULL;

					answer = TRUE;
 					
					rech_pinfo = FF_PI(dll_first(plist));

					/* note opposite test in read_formats */
					if (IS_EMBEDDED_TYPE(PINFO_TYPE(rech_pinfo)))
						PINFO_CURRENT_ARRAY_OFFSET(rech_pinfo) += pinfo_file_recl(rech_pinfo);
				}
			}

			ff_destroy_process_info_list(plist);

			if (TRUE == answer)
				return(answer);
		}

		error = db_ask(dbin, DBASK_PROCESS_INFO, FFF_INPUT | FFF_DATA, &plist);
		if (!error)
		{
 			PROCESS_INFO_PTR data_pinfo = NULL;
			BOOLEAN answer = FALSE;

			data_pinfo = FF_PI(dll_first(plist));

			if (PINFO_ARRAY_DONE(data_pinfo))
				answer = TRUE;

			ff_destroy_process_info_list(plist);

			return(answer);
		}
 	}

	return(FALSE);
}

/*****************************************************************************
 * NAME:
 *
 * PURPOSE:
 *
 * USAGE:
 *
 * RETURNS:
 *
 * DESCRIPTION:
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

static int check_for_array_EOF(PROCESS_INFO_LIST plist)
{
	PROCESS_INFO_PTR pinfo = NULL;

	FF_VALIDATE(plist);

	plist = dll_first(plist);
	pinfo = FF_PI(plist);
	while (pinfo)
	{
		FF_VALIDATE(pinfo);

		if (!IS_ARRAY(PINFO_FORMAT(pinfo)) && IS_DATA(PINFO_FORMAT(pinfo)))
			return(0);

		if (!PINFO_ARRAY_DONE(pinfo))
			return(0);
			
		plist = dll_next(plist);
		pinfo = FF_PI(plist);
	}

	return(EOF);
}

#ifdef ROUTINE_NAME
#undef ROUTINE_NAME
#endif
#define ROUTINE_NAME "read_formats"

/*****************************************************************************
 * NAME:
 *
 * PURPOSE:
 *
 * USAGE:
 *
 * RETURNS:
 *
 * DESCRIPTION:
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

static int read_formats
	(
	 DATA_BIN_PTR dbin,
	 FF_TYPES_t format_type
	)
{
	void *buffer = NULL;
	unsigned long bytes_to_read = 0;
	unsigned long bytes_read = 0;

	long count = 1L;
	
	int error = 0;

	PROCESS_INFO_LIST plist = NULL;
	PROCESS_INFO_PTR pinfo = NULL;

	format_type &= ~FFF_IO;
	format_type |= FFF_INPUT;

	error = db_ask(dbin, DBASK_PROCESS_INFO, format_type, &plist);
	if (error)
		return error;

	FF_VALIDATE(plist);
	
	plist = dll_first(plist);
	pinfo = FF_PI(plist);
	while (pinfo && !error)
	{
		unsigned long bsize = 0;
		unsigned long bcapacity = 0;
		
		FF_VALIDATE(pinfo);
		
		if (PINFO_NEW_RECORD(pinfo) || (!PINFO_IS_FILE(pinfo)  && !PINFO_LOCUS_BUFSIZE(pinfo)))
		{
			plist = dll_next(plist);
			pinfo = FF_PI(plist);
						
			continue;
		}

		error = restore_input_eqn_bufsize(pinfo);
		if (error)
			return error;

		error = ff_lock__(pinfo, &buffer, &bsize, &bcapacity);
		if (error)
			return(error);
		
		if (IS_INPUT_FILE_HEADER(pinfo))
		{
			if (PINFO_ARRAY_DONE(pinfo))
			{
				error = ff_unlock__(pinfo, &buffer, 0);
				if (error)
					return(error);
					
				plist = dll_next(plist);
				pinfo = FF_PI(plist);
						
				continue;
			}

			bytes_to_read = PINFO_RECL(pinfo);
			if (bytes_to_read >= PINFO_TOTAL_BYTES(pinfo))
			{
				error = ff_resize_bufsize(bytes_to_read + 1, &PINFO_DATA(pinfo));
				if (error)
					goto read_formats_exit;

				buffer = PINFO_BUFFER(pinfo);
			}

			if (PINFO_IS_BUFFER(pinfo))
				assert(0 == PINFO_CURRENT_ARRAY_OFFSET(pinfo));

			bytes_read = ndarr_reorient(PINFO_ARRAY_MAP(pinfo),
			                            PINFO_ID(pinfo),
			                            PINFO_IS_FILE(pinfo) ? PINFO_FNAME(pinfo) : PINFO_LOCUS_BUFFER(pinfo) + PINFO_CURRENT_ARRAY_OFFSET(pinfo), /* dangerous -- assumes PINFO_LOCUS_BUFSIZE */
			                            PINFO_IS_FILE(pinfo) ? PINFO_CURRENT_ARRAY_OFFSET(pinfo) : bytes_to_read,
			                            NDARRS_BUFFER,
			                            buffer,
			                            bytes_to_read,
			                            &PINFO_ARRAY_DONE(pinfo)
			                           );
			if (bytes_read != bytes_to_read)
			{
				error = err_push(ERR_READ_FILE, "Reading file header");
				goto read_formats_exit;
			}

			assert(PINFO_ARRAY_DONE(pinfo));

			ff_unlock__(pinfo, &buffer, (long)bytes_read);

			PINFO_BYTES_LEFT(pinfo) -= bytes_read;
			assert(PINFO_BYTES_LEFT(pinfo) == 0);

			error = db_do(dbin, DBDO_BYTE_SWAP, PINFO_FD(pinfo));
			if (error)
			{
				error = err_push(ERR_PROCESS_DATA, "Byte-swapping to native byte order");
				goto read_formats_exit;
			}

			error = calculate_input(pinfo);
			if (error)
				goto read_formats_exit;
		} /* if input header file format */
		else if (need_to_read_next_record_header(pinfo, dbin))
		{
			bytes_to_read = PINFO_RECL(pinfo);
			if (bytes_to_read >= PINFO_TOTAL_BYTES(pinfo))
			{
				error = ff_resize_bufsize(bytes_to_read + 1, &PINFO_DATA(pinfo));
				if (error)
					goto read_formats_exit;

				buffer = PINFO_BUFFER(pinfo);
			}

			error = make_tabular_format_array_mapping(pinfo, 1, 1, 1);
			if (error)
				goto read_formats_exit;
					
			if (PINFO_MATE(pinfo))
			{
				error = make_tabular_format_array_mapping(PINFO_MATE(pinfo), 1, 1, 1);
				if (error)
					goto read_formats_exit;
			}
					
			assert(PINFO_BYTES_LEFT(pinfo) == PINFO_RECL(pinfo));
			if (PINFO_MATE(pinfo))
				assert(PINFO_MATE_BYTES_LEFT(pinfo) == PINFO_MATE_RECL(pinfo));

			if (PINFO_IS_FILE(pinfo) || PINFO_CURRENT_ARRAY_OFFSET(pinfo) < PINFO_LOCUS_FILLED(pinfo))
			{
				bytes_read = ndarr_reorient(PINFO_ARRAY_MAP(pinfo),
				                            PINFO_ID(pinfo),
				                            PINFO_IS_FILE(pinfo) ? PINFO_FNAME(pinfo) : PINFO_LOCUS_BUFFER(pinfo) + PINFO_CURRENT_ARRAY_OFFSET(pinfo),
				                            PINFO_IS_FILE(pinfo) ? PINFO_CURRENT_ARRAY_OFFSET(pinfo) : bytes_to_read,
					                        NDARRS_BUFFER,
				                            buffer,
				                            bytes_to_read,
				                            &PINFO_ARRAY_DONE(pinfo)
				                           );
			}
			else
				bytes_read = 0;

			if ((bytes_read != bytes_to_read) || (PINFO_IS_BUFFER(pinfo) && PINFO_CURRENT_ARRAY_OFFSET(pinfo) >= PINFO_LOCUS_FILLED(pinfo)))
			{
				if (bytes_read)
					error = err_push(ERR_READ_FILE, "Reading record header");
				else
				{
					error = EOF;
					PINFO_BYTES_LEFT(pinfo) -= PINFO_RECL(pinfo);

					if (PINFO_MATE(pinfo))
						PINFO_MATE_BYTES_LEFT(pinfo) -= PINFO_MATE_RECL(pinfo);
				}

				goto read_formats_exit;
			}

			assert(PINFO_ARRAY_DONE(pinfo));

			ff_unlock__(pinfo, &buffer, (long)bytes_read);

			/* note opposite test in need_to_read_next_record_header */
			if (IS_SEPARATE_TYPE(PINFO_TYPE(pinfo)))
			{
				if (PINFO_IS_FILE(pinfo))
					PINFO_CURRENT_ARRAY_OFFSET(pinfo) += bytes_read;
				else
					assert(PINFO_IS_FILE(pinfo));
			}
			/* else don't know next offset -- it will be set when handling data format */

			PINFO_BYTES_LEFT(pinfo) -= bytes_read;

			/* Must swap bytes before using record header */
			error = db_do(dbin, DBDO_BYTE_SWAP, PINFO_FD(pinfo));
			if (error)
			{
				err_push(ERR_PROCESS_DATA, "Byte-swapping to native byte order");
				goto read_formats_exit;
			}

			error = calculate_input(pinfo);
			if (error)
				goto read_formats_exit;
		} /* else if need_to_read_next_record_header(pinfo, dbin) */
		else if (IS_INPUT_DATA(pinfo))
		{
			if (PINFO_ARRAY_DONE(pinfo) && IS_ARRAY(PINFO_FORMAT(pinfo)))
			{
				error = ff_unlock__(pinfo, &buffer, 0);
				if (error)
					return(error);
					
				plist = dll_next(plist);
				pinfo = FF_PI(plist);
						
				continue;
			}
			else if (PINFO_ARRAY_DONE(pinfo))
			{
				PROCESS_INFO_LIST rech_plist = NULL;
				PROCESS_INFO_PTR rech_pinfo = NULL;

				if (!db_ask(dbin, DBASK_PROCESS_INFO, FFF_INPUT | FFF_REC | FFF_HEADER, &rech_plist))
				{
					long record_count = 0;
					unsigned long previous_input_array_size = 0;

					rech_pinfo = FF_PI(dll_first(rech_plist));

					if (nt_ask(dbin, FFF_INPUT | FFF_REC, "count", FFV_LONG, &record_count) &&
						 nt_ask(dbin, NT_INPUT, "record_count", FFV_LONG, &record_count)
						)
					{
						error = err_push(ERR_GENERAL, "Cannot get record count");
						goto read_formats_exit;
					}

					if (record_count < 0)
					{
						error = err_push(ERR_PARAM_VALUE, "Invalid record count (%ld)", record_count);
						goto read_formats_exit;
					}
					else if (record_count == 0)
					{
						/* This might happen with an IDRISI vector file in which the 0-0 footer
						   is described as a record header.  It's conceivable that there might
							be record headers with empty data blocks, perhaps to show that a
							station exists but is not reporting any data.
						*/ 
						error = ff_unlock__(pinfo, &buffer, 0);
						if (error)
							return(error);
					
						plist = dll_next(plist);
						pinfo = FF_PI(plist);
									
						continue;
					}
					
					if (PINFO_ARRAY_MAP(pinfo)) /* virginity clause */
						previous_input_array_size = PINFO_SUPER_ARRAY_BYTES(pinfo);
					else
						previous_input_array_size = 0;

					error = make_tabular_format_array_mapping(pinfo, record_count, 1, record_count);
					if (error)
						goto read_formats_exit;
					
					if (PINFO_MATE(pinfo))
					{
						error = make_tabular_format_array_mapping(PINFO_MATE(pinfo), record_count, 1, record_count);
						if (error)
							goto read_formats_exit;
					}
					
					assert(PINFO_BYTES_LEFT(pinfo) == record_count * PINFO_RECL(pinfo));
					if (PINFO_MATE(pinfo))
						assert(PINFO_MATE_BYTES_LEFT(pinfo) == record_count * PINFO_MATE_RECL(pinfo));

					if (IS_SEPARATE_TYPE(PINFO_TYPE(rech_pinfo)))
					{
						if (PINFO_IS_FILE(pinfo))
							PINFO_CURRENT_ARRAY_OFFSET(pinfo) += previous_input_array_size;
						else
							assert(PINFO_IS_FILE(pinfo));

						if (PINFO_IS_FILE(rech_pinfo))
							PINFO_CURRENT_ARRAY_OFFSET(rech_pinfo) += pinfo_file_recl(rech_pinfo);
						else
							assert(PINFO_IS_FILE(rech_pinfo));
					}
					else
					{
						/* set file offset for data format's next read based on file offset of last read
							record header and its length
						*/
						if (1 || (PINFO_IS_FILE(pinfo) && PINFO_IS_FILE(rech_pinfo)))
							PINFO_CURRENT_ARRAY_OFFSET(pinfo) = PINFO_CURRENT_ARRAY_OFFSET(rech_pinfo) + pinfo_file_recl(rech_pinfo);
						else
							assert(PINFO_IS_FILE(pinfo) && PINFO_IS_FILE(rech_pinfo));

						/* set file offset for record header's next read based on its old
							file offset, its record length, and the data block length
						*/
						if (1 || PINFO_IS_FILE(rech_pinfo))
							PINFO_CURRENT_ARRAY_OFFSET(rech_pinfo) += pinfo_file_recl(rech_pinfo) + PINFO_BYTES_LEFT(pinfo);
						else
							assert(PINFO_IS_FILE(rech_pinfo));
					}

					ff_destroy_process_info_list(rech_plist);
				} /* if rech_pinfo && IS_REC_TYPE(PINFO_TYPE(rech_pinfo)) && IS_HEADER_TYPE(PINFO_TYPE(rech_pinfo)) */
				else
				{
					error = EOF;
					goto read_formats_exit;
				}
			} /* if PINFO_ARRAY_DONE(pinfo) */

			bytes_to_read = min(PINFO_BYTES_LEFT(pinfo), bcapacity);
			count = bytes_to_read / PINFO_RECL(pinfo);
			bytes_to_read = (unsigned)(count * PINFO_RECL(pinfo));

			assert(bytes_to_read <= bcapacity);
			assert(bytes_to_read < PINFO_TOTAL_BYTES(pinfo));

			bytes_read = ndarr_reorient(PINFO_ARRAY_MAP(pinfo),
			                            PINFO_ID(pinfo),
			                            PINFO_IS_FILE(pinfo) ? PINFO_FNAME(pinfo) : PINFO_LOCUS_BUFFER(pinfo) + PINFO_CURRENT_ARRAY_OFFSET(pinfo),
			                            PINFO_IS_FILE(pinfo) ? PINFO_CURRENT_ARRAY_OFFSET(pinfo) : bytes_to_read,
			                            NDARRS_BUFFER,
			                            buffer,
			                            bytes_to_read,
			                            &PINFO_ARRAY_DONE(pinfo)
			                           );
			if ((long)bytes_read <= 0)
			{
				if (bytes_read)
					error = err_push(ERR_READ_FILE, "Unable to read from %s", PINFO_IS_FILE(pinfo) ? PINFO_FNAME(pinfo) : "buffer");
				else
					error = err_push(ERR_READ_FILE, "Unexpected end of file processing \"%s\"", PINFO_NAME(pinfo));

				goto read_formats_exit;
			}

			ff_unlock__(pinfo, &buffer, (long)bytes_read);

			PINFO_BYTES_LEFT(pinfo) -= min(bytes_read, bytes_to_read);

			error = db_do(dbin, DBDO_BYTE_SWAP, PINFO_FD(pinfo));
			if (error)
			{
				err_push(ERR_PROCESS_DATA, "Byte-swapping to native byte order");
				goto read_formats_exit;
			}

			error = calculate_input(pinfo);
			if (error)
				goto read_formats_exit;
	
			if (dbin->eqn_info)
			{
				error = db_do(dbin, DBDO_FILTER_ON_QUERY);
				if (error == EOF)
				{
					/* filtered out all data for this cache!  Try again... */
					/* do not go to next pinfo -- stay the course with the current one */
					error = 0;

					continue;
				}
				else if (error)
					goto read_formats_exit;
			}
		} /* else if is input data */
		else if (WAIT_FOR_NEXT_RECORD_HEADER(pinfo))
		{
			/* Do nothing.  Wait until input data format is done cacheing */
			error = ff_unlock__(pinfo, &buffer, 0);
			if (error)
				return(error);
		}
		else
			assert(0);

		plist = dll_next(plist);
		pinfo = FF_PI(plist);
	} /* End of format_list processing (while) */

	if (!error && !fd_get_header(dbin, FFF_REC))
		error = check_for_array_EOF(plist);

	ff_destroy_process_info_list(plist);

	return(error);

read_formats_exit:

	(void)ff_unlock__(pinfo, &buffer, 0);

	ff_destroy_process_info_list(plist);

	return(error);
}

#ifdef ROUTINE_NAME
#undef ROUTINE_NAME
#endif
#define ROUTINE_NAME "byte_swap"

/*****************************************************************************
 * NAME: byte_swap()
 *
 * PURPOSE:  To byte-swap various data types
 *
 * USAGE:  error = byte_swap(data_ptr, data_type);
 *
 * RETURNS:
 *
 * DESCRIPTION:  Data in a 1-2, 1-2-3-4, or 1-2-3-4-5-6-7-8 order become a 2-1, 4-3-2-1,
 * or 8-7-6-5-4-3-2-1 order, respectively.
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

static int byte_swap(char *dataptr, FF_TYPES_t var_type)
{
	int error = 0;
	size_t byte_size = ffv_type_size(var_type);

	switch ((int)byte_size)
	{
		char temp;
		
		case 1:
			/* do nothing */
		break;
		
		case 2:
			temp = *dataptr;
			*dataptr = *(dataptr + 1);
			*(dataptr + 1) = temp;
		break;
		
		case 4:
			temp = *dataptr;
			*dataptr = *(dataptr + 3);
			*(dataptr + 3) = temp;
			temp = *(dataptr + 1);
			*(dataptr + 1) = *(dataptr + 2);
			*(dataptr + 2) = temp;
		break;
		
		case 8:
			temp = *dataptr;
			*dataptr = *(dataptr + 7);
			*(dataptr + 7) = temp;
			temp = *(dataptr + 1);
			*(dataptr + 1) = *(dataptr + 6);
			*(dataptr + 6) = temp;
			temp = *(dataptr + 2);
			*(dataptr + 2) = *(dataptr + 5);
			*(dataptr + 5) = temp;
			temp = *(dataptr + 3);
			*(dataptr + 3) = *(dataptr + 4);
			*(dataptr + 4) = temp;
		break;
		
		default:
			assert(!ERR_SWITCH_DEFAULT);
			error = err_push(ERR_SWITCH_DEFAULT, "%d, %s:%d", (int)byte_size, os_path_return_name(__FILE__), __LINE__);
		break;
	}

	return(error);
}

static int dbdo_process_formats
	(
	 DATA_BIN_PTR dbin,
	 FF_TYPES_t format_type
	)
{
	int error = 0;
	BOOLEAN done_reading = FALSE;

	FF_VALIDATE(dbin);
	assert(format_type);

	error = db_do(dbin, DBDO_READ_FORMATS, format_type);
	if (error == EOF || error == ERR_GENERAL)
	{
		error = 0;
		done_reading = TRUE;
	}

	if (!error)
		error = db_do(dbin, DBDO_CONVERT_FORMATS, format_type);

	if (!error && done_reading)
		error = EOF;

	return(error);
}

static int dbdo_process_data
	(
	 DATA_BIN_PTR dbin,
	 FF_TYPES_t format_type
	)
{
	int error = 0;
	BOOLEAN done_processing = FALSE;

	FF_VALIDATE(dbin);

	format_type &= ~FFF_IO;

	while (!error)
	{
		error = db_do(dbin, DBDO_PROCESS_FORMATS, FFF_INPUT | format_type);
		if (error == EOF)
		{
			error = 0;
			done_processing = TRUE;
		}

		if (!error)
			error = db_do(dbin, DBDO_WRITE_FORMATS, FFF_OUTPUT | format_type);

		if (done_processing)
			error = EOF;
	}

	return error;
}

static int dbdo_byte_swap(FORMAT_DATA_PTR format_data)
{
	VARIABLE_PTR var	= NULL;
	VARIABLE_LIST	v_list = NULL;
	
	FF_VALIDATE(format_data);
	FF_VALIDATE(format_data->data);

	assert(format_data->data->bytes_used < format_data->data->total_bytes);
		
	/* Check to make sure the input format is binary */
	if (!IS_BINARY(format_data->format))
		return(0);

	/* check whether or not data is in the native format */
	if (FD_IS_NATIVE_BYTE_ORDER(format_data))
		return(0);

	/* Byte-swap one variable at a time, down through columns */

	v_list = FFV_FIRST_VARIABLE(format_data->format);
	var    = FF_VARIABLE(v_list);
	while (var)
	{
		int error;
		long offset;

		offset = var->start_pos - 1;

		/* test for valid variable type before loop on second through remaining */
		error = byte_swap(format_data->data->buffer + offset, FFV_DATA_TYPE(var));
		if (error)
			return(error);

		offset += FORMAT_LENGTH(format_data->format);
		while ((FF_BSS_t)offset < format_data->data->bytes_used)
		{
			(void)byte_swap(format_data->data->buffer + offset, FFV_DATA_TYPE(var));
			offset += FORMAT_LENGTH(format_data->format);
		}

		v_list = dll_next(v_list);
		var    = FF_VARIABLE(v_list);
	}

	return(0);
}

static int dbdo_filter_on_query(DATA_BIN_PTR dbin)
{
	int error = 0;
	FF_BSS_t count = 0;
	char HUGE *src_ptr = NULL;
	char HUGE *trg_ptr = NULL;

	PROCESS_INFO_LIST process_info_list = NULL;
	PROCESS_INFO_PTR pinfo = NULL;

	FF_VALIDATE(dbin);
	FF_VALIDATE(dbin->eqn_info);

	/* Check for preconditions */
	assert(dbin->eqn_info);

	error = db_ask(dbin, DBASK_PROCESS_INFO, FFF_INPUT | FFF_DATA, &process_info_list);
	if (!error)
	{
		pinfo = FF_PI(dll_first(process_info_list));

		if (ee_check_vars_exist(dbin->eqn_info, PINFO_FORMAT(pinfo)))
		{
			ff_destroy_process_info_list(process_info_list);
			return(err_push(ERR_EE_VAR_NFOUND, "In input format"));
		}

		error = 0;
		count = 0;
		src_ptr = trg_ptr = PINFO_BUFFER(pinfo);
		while ((char HUGE *)src_ptr < (char HUGE *)(PINFO_BUFFER(pinfo) + PINFO_BYTES_USED(pinfo)))
		{
			if (ee_set_var_values(dbin->eqn_info, src_ptr, PINFO_FORMAT(pinfo)))
			{
				ff_destroy_process_info_list(process_info_list);
				return(err_push(ERR_GEN_QUERY, "Seting equation variables"));
			}

			if (ee_evaluate_equation(dbin->eqn_info, &error))
			{
				count++;
				if ((char HUGE *)src_ptr != (char HUGE *)trg_ptr) 
				{
					memMemcpy(trg_ptr,
								 src_ptr,
								 PINFO_RECL(pinfo),
								 NO_TAG
								);
				}

				trg_ptr += PINFO_RECL(pinfo);
			}

			if (error)
			{
				error = err_push(ERR_GEN_QUERY, "Problem with ee_evaluate_equation");
				break;
			}
				
			src_ptr += PINFO_RECL(pinfo);
		}
			
		/* seting data_ptr to NULL allows VIEW_GET_DATA to once again
			start at the beginning of the cache (dview->first_pointer) */
		PINFO_BYTES_USED(pinfo) = (FF_BSS_t)count * PINFO_RECL(pinfo);

		if (!count)
		{
			PINFO_NEW_RECORD(pinfo) = 0;
			error = EOF;
		}

		ff_destroy_process_info_list(process_info_list);
	} /* if not error */

	return(error);
}

static int dbdo_read_stdin
	(
	 DATA_BIN_PTR dbin,
	 FF_STD_ARGS_PTR std_args,
	 BOOLEAN *done_processing
	)
{
	int error = 0;

	FF_VALIDATE(std_args);
	FF_VALIDATE(dbin);

	if (std_args->user.is_stdin_redirected)
	{
		size_t bytes_read = 0;
		size_t bytes_to_read = 0;
		PROCESS_INFO_LIST plist = NULL;

		if (feof(stdin))
		{
			*done_processing = TRUE;
			return(0);
		}

		error = db_ask(dbin, DBASK_PROCESS_INFO, FFF_INPUT, &plist);
		if (!error)
		{
			PROCESS_INFO_PTR pinfo = NULL;

			plist = dll_first(plist);
			pinfo = FF_PI(plist);
			while (pinfo)
			{
				size_t records_to_read = 0;

				records_to_read = PINFO_LOCUS_SIZE(pinfo) / PINFO_RECL(pinfo);
				bytes_to_read = records_to_read * PINFO_RECL(pinfo);

				bytes_read = fread(std_args->input_bufsize->buffer, 1, bytes_to_read, stdin); 
				if (bytes_read != bytes_to_read)
				{
					if (!feof(stdin))
						error = err_push(ERR_READ_FILE, "...from standard input");
				}
				else
					*done_processing = FALSE;

				std_args->input_bufsize->bytes_used = bytes_read;

				if (!error)
				{
					size_t records_read = 0;

					records_read = bytes_read / PINFO_RECL(pinfo);

					if (bytes_read % PINFO_RECL(pinfo))
						error = err_push(ERR_PARTIAL_RECORD, "...from standard input");
					else
					{
						error = make_tabular_format_array_mapping(pinfo, records_read, 1, records_read);
						if (!error && PINFO_MATE(pinfo))
							error = make_tabular_format_array_mapping(PINFO_MATE(pinfo), records_read, 1, records_read);
					}
				}

				if (error)
					break;

				plist = dll_next(plist);
				pinfo = FF_PI(plist);
			}

			ff_destroy_process_info_list(plist);
		}
	}

	return(error);
}

/* Check to see if any fseek's will be done on output.  This will happen if an array has
   separation or has an offset other than zero.  Basically, output formats with only a
	single array will not require any fseek's on output.

   Also check for any separate headers
*/

static BOOLEAN is_separation(PROCESS_INFO_PTR pinfo)
{
	FF_VALIDATE(pinfo);

	if (PINFO_ARRAY_MAP(pinfo)->subsep)
		return(TRUE);
	else
		return(FALSE);
}

static BOOLEAN is_offset(PROCESS_INFO_PTR pinfo)
{
	FF_VALIDATE(pinfo);

	if (PINFO_CURRENT_ARRAY_OFFSET(pinfo) != 0)
		return(TRUE);
	else
		return(FALSE);
}

static int dbdo_check_stdout(DATA_BIN_PTR dbin)
{
	PROCESS_INFO_LIST plist = NULL;
	PROCESS_INFO_PTR  pinfo = NULL;
	int error = 0;

	FF_VALIDATE(dbin);

	error = db_ask(dbin, DBASK_PROCESS_INFO, FFF_OUTPUT, &plist);
	if (!error)
	{
		plist = dll_first(plist);
		pinfo = FF_PI(plist);
		while (pinfo)
		{
			FF_VALIDATE(pinfo);

			if (is_separation(pinfo) || is_offset(pinfo) || (IS_HEADER(PINFO_FORMAT(pinfo)) && IS_SEPARATE(PINFO_FORMAT(pinfo))))
			{
				error = err_push(ERR_GENERAL, "You cannot write to standard out using this output format: %s", PINFO_NAME(pinfo));
				break;
			}

			plist = dll_next(plist);
			pinfo = FF_PI(plist);
		}

		ff_destroy_process_info_list(plist);
	}
	else
		error = err_push(ERR_GENERAL, "Nothing to redirect as no output formats have been specified");

	return(error);
}

#ifdef ROUTINE_NAME
#undef ROUTINE_NAME
#endif
#define ROUTINE_NAME "db_do"

/*****************************************************************************
 * NAME:
 *
 * PURPOSE:
 *
 * USAGE:
 *
 * RETURNS:
 *
 * DESCRIPTION:
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

int db_do(DATA_BIN_PTR dbin, int message, ...)
{
	va_list args;
	int error = 0;

	va_start(args, message);

	FF_VALIDATE(dbin);

	switch (message)
	{
		case DBDO_WRITE_FORMATS:       /* Argument: format type */
		{
			FF_TYPES_t format_type = va_arg(args, FF_TYPES_t);

			error = write_formats(dbin, format_type);
		}
		break;
			
		case DBDO_BYTE_SWAP:
		{
			FORMAT_DATA_PTR format_data = va_arg(args, FORMAT_DATA_PTR);

			error = dbdo_byte_swap(format_data);
		}
		break;

		case DBDO_CONVERT_DATA:
		{
			FORMAT_DATA_MAPPING_PTR format_data_mapping = NULL;

			format_data_mapping = va_arg(args, FORMAT_DATA_MAPPING_PTR);
				
			error = dbin_convert_data(format_data_mapping);
		}
		break;

		case DBDO_READ_FORMATS:       /* Argument: format type */
		{
			FF_TYPES_t format_type = va_arg(args, FF_TYPES_t);

			error = read_formats(dbin, format_type);
		}
		break;

		case DBDO_PROCESS_FORMATS:
		{
			FF_TYPES_t format_type = va_arg(args, FF_TYPES_t);

			error = dbdo_process_formats(dbin, format_type);
		}
		break;

		case DBDO_PROCESS_DATA:
		{
			FF_TYPES_t format_type = va_arg(args, FF_TYPES_t);

			error = dbdo_process_data(dbin, format_type);
		}
		break;
			
		case DBDO_CONVERT_FORMATS:
		{
			FF_TYPES_t format_type = va_arg(args, FF_TYPES_t);

			error = convert_formats(dbin, format_type);
		}
		break;

		case DBDO_FILTER_ON_QUERY:

			error = dbdo_filter_on_query(dbin);
			
		break;

		case DBDO_READ_STDIN:
		{
			FF_STD_ARGS_PTR std_args = va_arg(args, FF_STD_ARGS_PTR);
			BOOLEAN *done_processing = va_arg(args, BOOLEAN *);

			error = dbdo_read_stdin(dbin, std_args, done_processing);
		}
		break;

		case DBDO_CHECK_STDOUT:

			error = dbdo_check_stdout(dbin);

		break;

		case DBDO_WRITE_OUTPUT_FMT_FILE:
		{
			char *fname = va_arg(args, char *);

			error = write_output_fmt_file(dbin, fname);

		}
		break;

		default:
		{
			assert(!ERR_SWITCH_DEFAULT);
			error = err_push(ERR_SWITCH_DEFAULT, "%s, %s:%d",  ROUTINE_NAME, os_path_return_name(__FILE__), __LINE__);
		}
	}       /* End of Attribute Switch */

	va_end(args);
	return(error);
}

