/*
 * FILENAME: file2buf.c
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
 * NAME:	ff_file_to_buffer
 *		
 * PURPOSE:	To read a file into a buffer
 *
 * USAGE:	unsigned int ff_file_to_buffer(char *file_name, char *buffer)
 *
 * RETURNS:	The number of bytes read into the buffer.
 *
 * DESCRIPTION:	
 *
 * SYSTEM DEPENDENT FUNCTIONS:	
 *
 * ERRORS:
 *		System File Error,file_name
 *		System File Error,file_name
 *		Problem reading file,"Input File To Buffer"
 *
 * AUTHOR:	T. Habermann, NGDC, (303) 497 - 6472, haber@ngdc.noaa.gov
 *
 * COMMENTS: SIDE EFFECT: If buffer is smaller then file size than
 *							memory beyond the buffer will be corrupted
 *						  this should be checked in the objects
 *	
 * RETURNS: returns 0 on error
 *
 * KEYWORDS:	
 *
 */
/*
 * HISTORY:
 *	r fozzard	4/21/95		-rf01 
 *		comment out struct stat (now part of stat.h included by unix.h)
*/

#include <freeform.h>

#undef ROUTINE_NAME
#define ROUTINE_NAME "ff_file_to_buffer"

static unsigned int ff_file_to_buffer(char *file_name, char *buffer)
{
	unsigned int file_length = 0;
	char *cp = NULL;

	FILE *input_file;
	size_t num_read;
	size_t num_to_read;

	/* Error checking on NULL parameters */
	assert(file_name && buffer);

	input_file = fopen(file_name, "rb");
	if (input_file == NULL)
	{
		err_push(ERR_OPEN_FILE, file_name);
		return(0);
	}

	/* Turn off system buffering on input file */
/* Profile this */
	setvbuf(input_file, NULL, _IONBF, (size_t)0);
	
	file_length = os_filelength(file_name);
	if (file_length >= UINT_MAX)
		err_push(ERR_GENERAL, "%s is too big! (exceeds %lu bytes)", file_name, (unsigned long)(UINT_MAX - 1));
	
	/* Cannot read more than size_t bytes from file */
	num_to_read = (size_t)min(file_length, UINT_MAX);

	num_read = fread(buffer, sizeof(char), num_to_read, input_file);
	fclose(input_file);

	if (num_read != num_to_read)
	{
		err_push(ERR_READ_FILE,"Input File To Buffer");
		return(0);
	}

	*(buffer + num_read) = STR_END;

	/* Remove any EOFs */
	cp = strchr(buffer, '\x1a');
	while (cp) {
		*cp = ' ';
		cp = strchr(buffer, '\x1a');
	}

	return(num_read);
}

int ff_file_to_bufsize(char *file_name, FF_BUFSIZE_HANDLE hbufsize)
{
	unsigned long filelength = os_filelength(file_name);
	assert(file_name);
	assert(hbufsize);
	
	if (!os_file_exist(file_name))
		return(err_push(ERR_OPEN_FILE, "%s", file_name));
	
	if (*hbufsize)
	{
		int error;

		FF_VALIDATE(*hbufsize);

		if (filelength + 1 > (*hbufsize)->total_bytes)
		{
			error = ff_resize_bufsize(filelength + 1, hbufsize);
			if (error)
				return(error);
		}
	}
	else
	{
		*hbufsize = ff_create_bufsize(filelength + 1);
		if (!*hbufsize)
			return(ERR_MEM_LACK);
	}
			
	(*hbufsize)->bytes_used = ff_file_to_buffer(file_name, (*hbufsize)->buffer);
	
	if ((*hbufsize)->bytes_used)
		return(0);
	else
		return(err_push(ERR_READ_FILE, "%s", file_name));
}

static int ff_bufsize_to_textfile(char *file_name, FF_BUFSIZE_PTR bufsize, char *mode)
{
	int error = 0;
	FILE *output_file = NULL;
	
	assert(file_name);
	FF_VALIDATE(bufsize);
	
	output_file = fopen(file_name, mode);
	if (!output_file)
		return(ERR_CREATE_FILE);
	
	if (fwrite(bufsize->buffer,
	           sizeof(char),
	           bufsize->bytes_used,
	           output_file
	          ) != bufsize->bytes_used
	   )
	{
		error = ERR_WRITE_FILE;
	}

	fclose(output_file);
	
	return(error);
}

int ff_bufsize_to_textfile_overwrite(char *file_name, FF_BUFSIZE_PTR bufsize)
{
	return(ff_bufsize_to_textfile(file_name, bufsize, "wt"));
}

int ff_bufsize_to_textfile_append(char *file_name, FF_BUFSIZE_PTR bufsize)
{
	return(ff_bufsize_to_textfile(file_name, bufsize, "at"));
}

