/*
 * FILENAME:        menuobj.c
 *              
 * PURPOSE:     To handle all basic functions of menus
 *
 * USAGE:       
 *
 * RETURNS:    
 *
 * DESCRIPTION: 
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none
 *
 * AUTHOR:      Kevin Frender kbf@ngdc.noaa.gov
 *
 * COMMENTS: 
 *          
 * KEYWORDS: menu index
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef NO_FF
#include <freeform.h>
#endif

#include <menuindx.h>

/* Local function prototypes:
 *
 * THESE FUNCTIONS ARE MEANT TO BE USED LOCALY ONLY!!!!
 * Unexpected or erratic behavior may result in using these
 * functions outside of mn_ land.  Do not move these prototypes!! */
long mn_get_filelength(FILE *infile);

/*
 * NAME:        mn_help_sec_find 
 *              
 * PURPOSE:     To find the offset of the help section associated with
 *              lookupstring.
 *
 * USAGE:       int mn_help_sec_find(MENU_INDEX_PTR mindex, char *lookupstring,
 *                  ROW_SIZES_PTR rowsize, char *section_name)
 *
 * RETURNS:     int, ERR_MN_SEC_NFOUND if no help sections were found,
 *              0 if help section was found, >0 on error.
 *
 * DESCRIPTION: Searches the menu for a help section associated with the 
 *              lookupstring.  Returns the offset of this section (in bytes)
 *              if it is found in the rowsize->start, setting it to 0 if no help
 *              sections were found.  The size of the found help section is
 *              stored in rowsize->num_bytes.  The title of the help section
 *              is returned in section_name.  If *section_name is NULL,
 *              the name of the found help section is not returned.  The 
 *				section_name argument must be mAlloced BEFORE calling this
 *              function, and at most 200 bytes of section title will be
 *              returned.
 *
 *              If no help sections are found, ERR_MN_SEC_NFOUND is returned,
 *              BUT NO ERRORS ARE PUSHED ONTO THE ERROR STACK.
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none
 *
 * AUTHOR:      Kevin Frender kbf@ngdc.noaa.gov
 *
 * COMMENTS:  Like all the mn_index functions, this function assumes that no
 *            menu line will exceede 490 bytes.
 *          
 * KEYWORDS: menu index help
 *
 */
#undef ROUTINE_NAME 
#define ROUTINE_NAME "mn_help_sec_find"
int mn_help_sec_find(MENU_INDEX_PTR mindex, char *lookup, ROW_SIZES_PTR rowsize, char *section_name)
{
	char *position;
	char *dirsep;
	char *extension;
	char *addinfo = NULL;
	char scratch[500];
	char pound_existed = 0;
	int i;
	
	assert((mindex) && (mindex->check_address == (void *)mindex) && (lookup) && (rowsize));

	position = strchr(lookup, '#');
	
	if(position){ /* there was a # directive in the string we got, fix it. */
		position++;
		pound_existed = 1;
	}
	else
		position = lookup;

	switch(position[0]){
		case '*': /* section directive */
			strcpy(scratch, ++position);
			for(i = strlen(scratch); i > 0; i--) /* trim off EOL chars */
				if(scratch[i] > ' ') 
					break;
				else
					scratch[i] = '\0';
			strcat(scratch, "_help");
			i = mn_index_get_offset(mindex, scratch, rowsize);
			if(section_name)
				if(!i) /* Section found */
					strcpy(section_name, scratch);
				else
					section_name[0] = '\0';
			return(i);
		case 'H': /* term section directive */
			if(pound_existed){
				/* No help can exist for a term section */
				rowsize->start = 0;
				rowsize->num_bytes = 0;
				return(ERR_MN_SEC_NFOUND);
			}
			position--; /* NO BREAK ON PURPOSE */
		case '#': /* another menu file or term file directive */
			position++;
			/* NO BREAK ON PURPOSE */
		default: /* file name */
			if(position[0] == '&')
				position++;
			
			if(position[0] == '$')
				position++;

			/* The search for help sections for files procedes as follows:
			 *
			 * 1. Search for FULLPATH\BASEFILENAME.EXTENSION+ADDINFO_help
			 * 3. Search for FULLPATH\BASEFILENAME.EXTENSION_help
			 * 4. Search for FULLPATH\BASEFILENAME_help
			 * 5. Search for BASEFILENAME.EXTENSION_help
			 * 6. Search for BASEFILENAME_help
			 * 7. Search for FULLPATH\.EXTENSION_help
			 * 8. Search for .EXTENSION_help
			 */
			strcpy(scratch, position);
			for(i = strlen(scratch); i > 0; i--) /* trim off EOL chars */
				if(scratch[i] > ' ') 
					break;
				else
					scratch[i] = '\0';
			strcat(scratch, "_help");
			
			/* Step 1 */
			i = mn_index_get_offset(mindex, scratch, rowsize);
			if((i == 0) || (i != ERR_MN_SEC_NFOUND)){ /* Section found or error */
				if(section_name)
					strcpy(section_name, scratch);
				return(i);
			}
			
			if((addinfo = strchr(scratch, '+'))){
				/* Trim off extra information */
				addinfo[0] = '\0';
				addinfo = strchr(position, '+');
				addinfo[0] = '\0';

				/* Step 3 */                
				strcat(scratch, "_help");
				i = mn_index_get_offset(mindex, scratch, rowsize);
				if((i == 0) || (i != ERR_MN_SEC_NFOUND)){ /* Section found or error */
					if(section_name)
						strcpy(section_name, scratch);
					if(addinfo)
						addinfo[0] = '+';
					return(i);
				}
			}

			/* Search for next possible help section (step 4) */
			extension = strrchr(scratch, '.');
			if(extension){ /* don't bother to search if there is no extension */
				strcpy(extension, "_help");
				i = mn_index_get_offset(mindex, scratch, rowsize);
				if((i == 0) || (i != ERR_MN_SEC_NFOUND)){ /* section found or error */
					if(section_name)
						strcpy(section_name, scratch);
					if(addinfo)
						addinfo[0] = '+';
					return(i);
				}
			}

			/* Search for next possible help section (step 5) */
			dirsep = strrchr(position, MENU_DIR_SEP);

			if(dirsep) { /* if no directory seperator exists; no help sections
								  matching steps 3, 4, or 6 will exist */

				strcpy(scratch, ++dirsep);
				for(i = strlen(scratch); i > 0; i--) /* trim off EOL chars */
					if(scratch[i] > ' ') 
						break;
					else
						scratch[i] = '\0';

				strcat(scratch, "_help");
				i = mn_index_get_offset(mindex, scratch, rowsize);
				if((i == 0) || (i != ERR_MN_SEC_NFOUND)){ /* section found or error */
					if(section_name)
						strcpy(section_name, scratch);
					if(addinfo)
						addinfo[0] = '+';
					return(i);
				}

				/* Search for next possible help section (step 6) */
				extension = strrchr(scratch, '.');
				if(extension){ /* don't bother to search if there is no extension */
					strcpy(extension, "_help");
					i = mn_index_get_offset(mindex, scratch, rowsize);
					if((i == 0) || (i != ERR_MN_SEC_NFOUND)){ /* section found or error */
						if(section_name)
							strcpy(section_name, scratch);
						if(addinfo)
							addinfo[0] = '+';
						return(i);
					}
				}
			} /* end of steps 5 and 6 */

			/* Search for FULLPATH\.EXTENSION_help (step 7) */
			strcpy(scratch, position);

			extension = strrchr(scratch, '.');
			
			if(extension){ /* If no extension exists, steps 7 and 8 are passed */
				dirsep = strrchr(scratch, MENU_DIR_SEP);
				if(dirsep){ /* a directory seperator does exist */
					strcpy(dirsep + 1, extension);
				}
				else{
					strcpy(scratch, extension);
				}
				
				for(i = strlen(scratch); i > 0; i--) /* trim off EOL chars */
					if(scratch[i] > ' ') 
						break;
					else
						scratch[i] = '\0';

				strcat(scratch, "_help");
				i = mn_index_get_offset(mindex, scratch, rowsize);
				if((i == 0) || (i != ERR_MN_SEC_NFOUND)){ /* section found or error */
					if(section_name)
						strcpy(section_name, scratch);
					if(addinfo)
						addinfo[0] = '+';
					return(i);
				}
				
				/* Search for .EXTENSION_help (Step 8) */
				if(dirsep){ /* this is pointless if no dirseperator exists */
					extension = strrchr(scratch, '.');
					strcpy(scratch, extension);
  
					i = mn_index_get_offset(mindex, scratch, rowsize);
					if((i == 0) || (i != ERR_MN_SEC_NFOUND)){ /* section found or error */
						if(section_name)
							strcpy(section_name, scratch);
						if(addinfo)
							addinfo[0] = '+';
						return(i);
					}
				}
			}

			/* No help sections were found. */
			rowsize->start = 0;
			rowsize->num_bytes = 0;
			if(section_name)
				section_name[0] = '\0';

			if(addinfo)
				addinfo[0] = '+';
			return(ERR_MN_SEC_NFOUND);
	} /* end of switch */
}

/*
 * NAME:        mn_datasec_len_get
 *              
 * PURPOSE:     To report the length (in bytes) of a data section.
 *
 * USAGE:       int mn_datasec_len_get(MENU_INDEX_PTR mindex, 
 *						ROW_SIZES_PTR rowsize, long *length)
 *
 * RETURNS:     0 if all is OK, (setting *length to the length of the data in
 *				the data section) or >0 on error.
 *
 * DESCRIPTION:  Uses the MENU_INDEX struct to get the menu file name,
 *				 retrieves the section described by rowsize, and retrieves
 *				 the second line of that section (the first line contains
 *				 the section title, the second contains the length of data)
 *				 to determine the data length.  This number is then placed
 *				 in *(length).
 *
 *				 If the section title does not end in _DATASEC, an error
 *				 is returned.
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none
 *
 * AUTHOR:      Kevin Frender kbf@ngdc.noaa.gov
 *
 * COMMENTS:  
 *          
 * KEYWORDS: menu index 
 *
 */                  
#undef ROUTINE_NAME 
#define ROUTINE_NAME "mn_datasec_len_get"
int mn_datasec_len_get(MENU_INDEX_PTR mindex, ROW_SIZES_PTR rowsize, long *length)
{
	FILE *infile;
	char scratch[500];
	
	assert((mindex) && (mindex->check_address == (void *)mindex) && (rowsize) && (length));
	
	/* open the menu file */
	if(!(infile = fopen(mindex->menu_file, MENU_FOPEN_R))) { /* couldn't open file */
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_OPEN_FILE, mindex->menu_file);
		return(ERR_OPEN_FILE);
	}
    
    /* Seek to offset */
	if(fseek(infile, rowsize->start, SEEK_SET)){ /* error in fseek */
		fclose(infile);
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_READ_FILE, "Can't seek section offset");
		return(ERR_READ_FILE);
	}

	/* Get the title */	
	if(!(mn_binary_fgets(scratch, 490, infile, mindex->file_eol_str))){
		fclose(infile);
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_READ_FILE, "Can't read section title");
		return(ERR_READ_FILE);
	}
	
	/* Check the title */
	if (!strstr(scratch, MENU_DATA_SECTION)) {
		fclose(infile);
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_GENERAL, "Not a data section");
		return(ERR_GENERAL);
	}
	
	/* Get the length */	
	if(!(mn_binary_fgets(scratch, 490, infile, mindex->file_eol_str))){
		fclose(infile);
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_READ_FILE, "Can't read section title");
		return(ERR_READ_FILE);
	}
	
	fclose(infile);
	*(length) = atol(scratch);	
	
	return(0);
}


/*
 * NAME:        mn_section_get 
 *              
 * PURPOSE:     To read the menu section described by rowsize into a buffer.
 *
 * USAGE:       int mn_section_get(MENU_INDEX_PTR mindex, char *buf_to_use,
 *                    ROW_SIZES_PTR rowsize, char **buf_filled)
 *
 * RETURNS:     0 if all is OK, (setting *buf_filled to the buffer with the
 *              section read in) or >0 on error.
 *              The buffer returned in *buf_filled is null-terminated, and
 *              does not contain the title. (Unless the rowsize->num_bytes
 *              is less than 0, in which case the title is included)
 *              Returns ERR_MN_BUFFER_TRUNCATED if the buffer was not large
 *              enough to hold the section.
 *
 * DESCRIPTION:  Uses the MENU_INDEX struct to get the menu file name,
 *               create a buffer (of maximum size mindex->max_buffer_size)
 *               large enough to hold the menu section to be read in, and
 *               fills the buffer with this section from the menu file.
 *               The section desired is specified by the rowsize pointer,
 *               which can be obtained by a previous call to
 *               mn_index_get_offset or mn_help_sec_find.  The buf_to_use
 *               argument should usually be NULL; it is here to provide
 *               backward compatibility with a nasty habit started with
 *               the string database (sdb_) functions of reading a section
 *               into a buffer of UNKNOWN SIZE.  Providing a value other
 *               than NULL for buf_to_use will force mn_section_get
 *               to fill the buffer at that location.  THIS CAUSES
 *               PROBLEMS, because the section can be longer than the
 *               buffer, and there is no way for mn_section_get to determine
 *               if this nasty memory corruption is occuring.
 *
 *				 If the section title ends in _DATASEC (the section is a data 
 *				 section) the data is read into the buffer and returned.
 *				 This DOES NOT INCLUDE the line specifying the number of bytes
 *				 of data.
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none
 *
 * AUTHOR:      Kevin Frender kbf@ngdc.noaa.gov
 *
 * COMMENTS:  Do not hold this function responsible for core dumps if the
 *            buf_to_use argument is not NULL.
 *          
 * KEYWORDS: menu index 
 *
 */                  
#undef ROUTINE_NAME 
#define ROUTINE_NAME "mn_section_get"
int mn_section_get(MENU_INDEX_PTR mindex, char *buf_to_use, ROW_SIZES_PTR rowsize, char **buf_filled)
{
	char *scratch;
	char datasec[40];
	FILE *infile;
	ulong buf_size;
	ulong bytes_to_read;
	ulong dataseclen = 0;
	char inc_title = 0;
	int retval = 0;
	
	assert((mindex) && (mindex->check_address == (void *)mindex) && (rowsize));
	
	/* Special case to include the title in the section */
	if(rowsize->num_bytes < 0){
		rowsize->num_bytes *= -1;
		inc_title = 1;
	}

	if(buf_to_use){ /* YUCK! We are being forced to use a buffer pre-mAllocated
							 to a size we don't know. */
		scratch = buf_to_use;
		bytes_to_read = rowsize->num_bytes;

		if(bytes_to_read >= mindex->max_buffer_size){
			retval = ERR_MN_BUFFER_TRUNCATED;
			bytes_to_read = mindex->max_buffer_size - 2;
		}
	}
	else { /* need to allocat a buffer the right size */
		buf_size = rowsize->num_bytes;

		if(buf_size >= mindex->max_buffer_size){
			retval = ERR_MN_BUFFER_TRUNCATED;
			buf_size = mindex->max_buffer_size - 5;
		}

		bytes_to_read = buf_size;
		buf_size += 2; /* add a little breating room onto the buffer */

		if(!(scratch = (char *)mAlloc((size_t)buf_size))){ /* out of memory */
			MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating buffer for section");
			if(!buf_to_use)
				fRee(scratch);
			if(buf_filled)
				*(buf_filled) = NULL;
			return(ERR_MEM_LACK);
		}
	}

	/* open the menu file */
	if(!(infile = fopen(mindex->menu_file, MENU_FOPEN_R))) { /* couldn't open file */
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_OPEN_FILE, mindex->menu_file);
		if(!buf_to_use)
			fRee(scratch);
		if(buf_filled)
			*(buf_filled) = NULL;
		return(ERR_OPEN_FILE);
	}

	if(fseek(infile, rowsize->start, SEEK_SET)){ /* error in fseek */
		fclose(infile);
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_READ_FILE, "Can't seek section offset");
		if(!buf_to_use)
			fRee(scratch);
		if(buf_filled)
			*(buf_filled) = NULL;
		return(ERR_READ_FILE);
	}

	if(!inc_title){ 
		if(!(mn_binary_fgets(scratch, 490, infile, mindex->file_eol_str))){
			fclose(infile);
			MENU_ERR_PUSH(ROUTINE_NAME, ERR_READ_FILE, "Can't read section title");
			if(!buf_to_use)
				fRee(scratch);
			if(buf_filled)
				*(buf_filled) = NULL;
			return(ERR_READ_FILE);
		}
	
		bytes_to_read -= strlen(scratch); /* Get rid of title from length to read */
		
		strcpy(datasec, MENU_DATA_SECTION);
		strcat(datasec, mindex->file_eol_str);
		
		if (strstr(scratch, datasec)) {
			/* It is a data section, retrieve only "data length" bytes */
			
			if(!(mn_binary_fgets(scratch, 490, infile, mindex->file_eol_str))){
				fclose(infile);
				MENU_ERR_PUSH(ROUTINE_NAME, ERR_READ_FILE, "Can't read data section length");
				if(!buf_to_use)
					fRee(scratch);
				if(buf_filled)
					*(buf_filled) = NULL;
				return(ERR_READ_FILE);
			}
			
			bytes_to_read -= strlen(scratch); /* Get rid of line from length to read */
		}
	}
	if(fread(scratch, (size_t)1, (size_t)bytes_to_read, infile) != (size_t)bytes_to_read){ 
		fclose(infile);
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_READ_FILE, "Can't read in section");
		if(!buf_to_use)
			fRee(scratch);
		if(buf_filled)
			*(buf_filled) = NULL;
		return(ERR_READ_FILE);
	}
	
	fclose(infile);

	scratch[bytes_to_read] = '\0';

	if(buf_filled){ /* Don't write over a null pointer... */
		*(buf_filled) = scratch;
	}

	return(retval);
}

/*
 * NAME:        mn_sec_titles_to_buf 
 *              
 * PURPOSE:     To create a doubly linked list containing only strings which
 *              are the section titles of a menu file.
 *
 * USAGE:       int mn_sec_titles_to_buf(MENU_INDEX_PTR mindex, 
 *                  char *buf_to_use, int *num_sections, char **buf_filled)
 *
 * RETURNS:     int, 0 on success, >0 on error.
 *
 * DESCRIPTION:  Uses the MENU_INDEX struct to get the menu file name,
 *               create a buffer (of maximum size mindex->max_buffer_size)
 *               large enough to hold the list of menu sections, and
 *               fills the buffer with the list of sections in the menu.
 *               The buf_to_use
 *               argument should usually be NULL; it is here to provide
 *               backward compatibility with a nasty habit started with
 *               the string database (sdb_) functions of writing 
 *               into a buffer of UNKNOWN SIZE.  Providing a value other
 *               than NULL for buf_to_use will force mn_sec_titles_to_buf
 *               to fill the buffer at that location.  THIS CAUSES
 *               PROBLEMS, because the list of section titles can be longer
 *               than the buffer, and there is no way for mn_sec_titles_to_buf
 *               to determine if this nasty memory corruption is occuring.
 *
 *    If the list in the buffer had to be truncated for size
 *              reasons, ERR_MN_BUFFER_TRUNCATED is returned.
 *
 *    This function replaces any EOL characters with "\n" in the
 *              created buffer.
 *
 *    If the num_sections or buf_filled pointers are NULL, they
 *              are ignored and not filled.
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none
 *
 * AUTHOR:      Kevin Frender kbf@ngdc.noaa.gov
 *
 * COMMENTS:  Do not hold this function responsible for core dumps if the
 *            buf_to_use argument is not NULL.
 *
 *            This function directly accesses the MENU_INDEX buffers,
 *            without calls to MN_INDEX functions.  
 *          
 * KEYWORDS: menu index 
 *
 */
#undef ROUTINE_NAME 
#define ROUTINE_NAME "mn_sec_titles_to_buf"
int mn_sec_titles_to_buf(MENU_INDEX_PTR mindex, char *buf_to_use, int *num_sections, char **buf_filled)
{
	char *scratch;
	char *position;
	char *pos;
	char *mindexbuf;
	ulong buf_size;
	int i;
	long numread;
	long numonline;
	
	assert((mindex) && (mindex->check_address == (void *)mindex));

	if(num_sections)
		*(num_sections) = mindex->num_sections;

	if(buf_to_use){ /* YUCK! we are forced to use a buffer whose size we don't know */
		scratch = buf_to_use;
		buf_size = mindex->max_buffer_size; /* hope this is the size of the buf */
	}
	else{ /* allocate our own */
		buf_size = mindex->num_sections * 100; /* get a rough estimate */

		if(buf_size > mindex->max_buffer_size)
			buf_size = mindex->max_buffer_size;

		if(!(scratch = (char *)mAlloc((size_t)buf_size))){
			MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating buffer");
			return(ERR_MEM_LACK);
		}
	}
	
	scratch[0] = '\0';
	position = scratch;
	numread = 5;

	for(i = 0; i < mindex->num_buffers; i++){
		/* cycle through all the MENU INDEX buffers */
		mindexbuf = mindex->index_buffer[i];

		while((mindexbuf = strstr(mindexbuf, "#*"))){
			mindexbuf += 2;

			if(mindexbuf[0] > ' '){ /* make sure we aren't hitting an
											 * end-of-buffer phantom section */
				pos = strstr(mindexbuf, mindex->file_eol_str);

				numonline = pos - mindexbuf;
				if(numread + numonline >= (long)buf_size){
					/* see if we can enlarge the buffer */
					if(!buf_to_use){ /* WE are allocating the buffer */
						if(buf_size < mindex->max_buffer_size) {
							buf_size = mindex->max_buffer_size;
							
							if(!(position = (char *)reAlloc(scratch, (size_t)buf_size))){
								/* Could not reAlloc buffer; send back partial */
								if(buf_filled){
									*(buf_filled) = scratch;
								}
								return(ERR_MN_BUFFER_TRUNCATED);
							}
							scratch = position;
							position = scratch + strlen(scratch);
						}
						else{
							/* Could not reAlloc buffer; send back partial */
							if(buf_filled){
								*(buf_filled) = scratch;
							}
							return(ERR_MN_BUFFER_TRUNCATED);
						}
					}
					else{
						/* Could not reAlloc buffer; send back partial */
						if(buf_filled){
							*(buf_filled) = scratch;
						}
						return(ERR_MN_BUFFER_TRUNCATED);
					}
				}  

				strncpy(position, mindexbuf, (size_t)numonline);
				position[numonline] = '\0';
				position += numonline;
				numread += numonline;

				strcat(position, "\n");
				position++;
				numread++;

				if(position[0] != '\0'){
					position++;
					numread++;
				}
			}
		}
	}

	if(buf_filled)
		*(buf_filled) = scratch;
	return(0);
}


/*
 * NAME:  mn_help_sec_get
 *
 * PURPOSE:     To locate help text associated with lookup and load it into
 *              a buffer.
 *
 * USAGE:       int mn_help_sec_get(MENU_INDEX_PTR mindex, char *lookup,
 *                    ROW_SIZES_PTR rowsize, char **buf_filled)
 *
 * RETURNS:     0 if successful; >0 on error.  Will return (BUT NOT PUSH ONTO
 *              THE ERROR STACK) ERR_MN_SEC_NFOUND if a help section associated
 *              with lookup cannot be found.  If a help section is found, but
 *              references another section which cannot be found, 
 *              ERR_MN_FILE_CORRUPT will be returned.  If a help section 
 *              references a file which cannot be found,
 *              ERR_MN_REF_FILE_NFOUND will be returned.  If the help text
 *              to be retrieved is larger than mindex->max_buffer_size,
 *              ERR_MN_SEC_TRUNCATED will be returned, but the buffer will
 *              be filled with mindex->max_buffer_size bytes of help text.
 *
 *              In the buf_filled argument is stored the pointer to the buffer
 *              created and filled if successful; it is NULL on error.
 *              The buffer returned is null-terminated.
 *
 * DESCRIPTION:  Uses the MENU_INDEX struct to get the menu file name,
 *               then searches for the help section to read.  This is
 *               done as follows:  If the rowsize pointer is not NULL,
 *               it is assumed that the rowsize struct is pointing to the
 *               help section in the menu file.  If the lookup pointer is
 *               not NULL, the string is checked for an appended _help,
 *               and (if lookup ends in _help) this section is searched for.
 *               If lookup is not NULL and not terminated by _help, it is
 *               sent to mn_help_sec_find to find the appropriate associated
 *               help section.  If the lookup pointer and the rowsize pointer
 *               are both defined, the function will use the rowsize pointer.
 *
 *               Once the help section is found, it is looked at to determine
 *               if it is merely a reference to antother section or to a
 *               documentation file.  All references are followed until the
 *               actual help text is found.  This help text is then read into
 *               a buffer (allocated in mn_help_sec_get to assure proper
 *               size) up to mindex->max_buffer_size bytes.
 *
 *               Even if an error occurs, the buffer containing the help
 *               section may be returned in buf_filled.
 *
 *               The title line of the help section is not returned in the buffer
 *               unless an error occurs.
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none
 *
 * AUTHOR:      Kevin Frender kbf@ngdc.noaa.gov
 *
 * COMMENTS:
 *
 * KEYWORDS: menu index help
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "mn_help_sec_get"
int mn_help_sec_get(MENU_INDEX_PTR mindex, char *lookup, ROW_SIZES_PTR rowsize, char **buf_filled)
{
	FILE *infile;
	int retval = 0;
	int i, j;
	char *scratch = NULL;
	char *path = NULL;
	char *pos = NULL;
	char *position;
	char *helpappend = "_help";
	char sec_found = 0;
	long length;
	ROW_SIZES lrowsize;


	assert((mindex) && (mindex->check_address == (void *)mindex) && ((lookup) || (rowsize)) && (buf_filled));

	/* Before we can look for references to other help sections or files, we
	 * must first get the initial help section. */

	if(rowsize){ /* rowsize pointer was specified */
		retval = mn_section_get(mindex, NULL, rowsize, &scratch);
		if(retval){ /* error occured */
			MENU_ERR_PUSH(ROUTINE_NAME, ERR_UNKNOWN, "Retrieving menu section");
			*(buf_filled) = scratch;
			return(retval);
		}
		sec_found = 1;
	}

	if((!sec_found) && lookup){ /* No rowsize, so look for help section */
		/* Check for an appended _help on lookup */
		j = strlen(lookup) - 1;
		for(i = 4; i >= 0; i--)
			if(lookup[j--] != helpappend[i])
				break;
				
		if(i < 0) { /* Did have _help appended */
			retval = mn_index_get_offset(mindex, lookup, &lrowsize);
			if(!retval)
				retval = mn_section_get(mindex, NULL, &lrowsize, &scratch);
			if(retval){ /* error occured */
				MENU_ERR_PUSH(ROUTINE_NAME, ERR_UNKNOWN, "Retrieving menu section");
				*(buf_filled) = scratch;
				return(retval);
			}
			sec_found = 1;
		}
		else { /* Did not have _help appended */
			retval = mn_help_sec_find(mindex, lookup, &lrowsize, NULL);

			if(retval){ /* section not found or error */
				*(buf_filled) = scratch;
				return(retval);
			}

			retval = mn_section_get(mindex, NULL, &lrowsize, &scratch);
			if(retval){ /* error occured */
				MENU_ERR_PUSH(ROUTINE_NAME, ERR_UNKNOWN, "Retrieving menu section");
				*(buf_filled) = scratch;
				return(retval);
			}
			sec_found = 1;
		}
	}      
	
	if(!sec_found){ /* No lookup or rowsize pointer went in */
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_PTR_DEF, "For rowsize and lookup arguments");
		*(buf_filled) = scratch;
		return(ERR_PTR_DEF);
	}

	/* Now that we have the section in memory, follow any references
	 * to any other sections or files */

	while(sec_found){
		position = scratch;

		if(position[0] != '>'){ /* NOT a reference */
			*(buf_filled) = scratch;
			return(0); /* all is OK */
		}

		sec_found = 0;
		position++;

		if(position[0] == '*'){ /* reference to another help section */
			pos = strstr(position, mindex->file_eol_str);
			if(pos)
				pos[0] = '\0';
			retval = mn_index_get_offset(mindex, position, &lrowsize);
			if (pos)	// jhrg 3/18/11
			    pos[0] = mindex->file_eol_str[0];
			if(retval){ /* section not found or error */
				*(buf_filled) = scratch;
				return(retval);
			}
			
			fRee(scratch);
			
			retval = mn_section_get(mindex, NULL, &lrowsize, &scratch);
			if(retval){ /* error occured */
				MENU_ERR_PUSH(ROUTINE_NAME, ERR_UNKNOWN, "Retrieving menu section");
				*(buf_filled) = scratch;
				return(retval);
			}
			sec_found = 1;
		}
		else{ /* file reference */
			pos = strstr(position, mindex->file_eol_str);
			if(!(path = (char *)mAlloc((size_t)(500 * sizeof(char))))){
				MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating path");
				*(buf_filled) = scratch;
				return(ERR_MEM_LACK);
			}
			
			if(position[0] == '&')
				position++;
				
			if(pos)
				pos[0] = '\0';
			
			if(!mindex->data_path_one){
				*(buf_filled) = scratch;
				fRee(path);
				MENU_ERR_PUSH(ROUTINE_NAME, ERR_MN_REF_FILE_NFOUND, "No paths defined");
				return(ERR_MN_REF_FILE_NFOUND);
			}
			os_path_put_parts(path, mindex->data_path_one, position, NULL);
			os_path_make_native(path, path);
			if(infile = fopen(path, MENU_FOPEN_R)){
				fRee(scratch);
				length = mn_get_filelength(infile);
				if(length < 1){
					MENU_ERR_PUSH(ROUTINE_NAME, ERR_UNKNOWN, "Determining file length");
					fRee(path);
					return(ERR_UNKNOWN);
				}
				if(length > mindex->max_buffer_size - 5){
					length = mindex->max_buffer_size - 5;
				}
				if(!(scratch = (char *)mAlloc((size_t)(length + 5)))){
					MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "for help section");
					fRee(path);
					return(ERR_MEM_LACK);
				}
				if (fread(scratch, 1, length, infile) != length)
				{
					fRee(path);
					fclose(infile);
					MENU_ERR_PUSH(ROUTINE_NAME, ERR_READ_FILE, path);
					return(ERR_READ_FILE);
				}

				scratch[length] = '\0';
				*(buf_filled) = scratch;
				fRee(path);
				fclose(infile);
				return(0);              
			}
			else{
				if(mindex->data_path_two){
					os_path_put_parts(path, mindex->data_path_two, position, NULL);
					os_path_make_native(path, path);
					if(!(infile = fopen(path, MENU_FOPEN_R))){
						*(buf_filled) = scratch;
						fRee(path);
						MENU_ERR_PUSH(ROUTINE_NAME, ERR_MN_REF_FILE_NFOUND, "Retrieving help text");
						return(ERR_MN_REF_FILE_NFOUND);
					}
					fRee(scratch);
					length = mn_get_filelength(infile);
					if(length < 1){
						MENU_ERR_PUSH(ROUTINE_NAME, ERR_UNKNOWN, "Determining file length");
						fRee(path);
						fcloase(infile);	// jhrg 3/18/11
						return(ERR_UNKNOWN);
					}
					if(length > mindex->max_buffer_size - 5){
						length = mindex->max_buffer_size - 5;
					}
					if(!(scratch = (char *)mAlloc((size_t)(length + 5)))){
						MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "for help section");
						fcloase(infile);	// jhrg 3/18/11
						fRee(path);
						return(ERR_MEM_LACK);
					}
					if (fread(scratch, 1, length, infile) != length)
					{
						fRee(path);
						fclose(infile);
						MENU_ERR_PUSH(ROUTINE_NAME, ERR_READ_FILE, path);
						return(ERR_READ_FILE);
					}
 					
					scratch[length] = '\0';
					*(buf_filled) = scratch;
					fRee(path);
					fclose(infile);
					return(0);              
				}
				else{ /* No second data path defined */
					*(buf_filled) = scratch;
					fRee(path);
					MENU_ERR_PUSH(ROUTINE_NAME, ERR_MN_REF_FILE_NFOUND, position);
					return(ERR_MN_REF_FILE_NFOUND);
				}
			}
		}
	}
}

/*
 * NAME:  mn_sec_process
 *
 * PURPOSE:     To process a menu section, determine its type, and return
 *              a MENU_SECTION structure.
 *
 * USAGE:       int mn_sec_process(MENU_INDEX_PTR mindex, char *section,
 *                    ROW_SIZES_PTR rowsize, char *parent_menu,
 *                    MENU_SECTION_PTR *menu_sec)
 *
 * RETURNS:     0 if successful; >0 on error.  
 *              If the specified section is not found, ERR_MN_SEC_NFOUND
 *              is returned (But no error is pushed onto the error stack).
 *
 * DESCRIPTION:  Finds the menu section specified and processes it into a
 *               MENU_SECTION structure.  The section is specified by the
 *               section variable or by the rowsize structure.  If neither
 *               argument is NULL, the rowsize will be used to retrieve the
 *               section.  If both are NULL, an INTRODUCTION section is 
 *               searched for (and if existant) displayed.  If no
 *               INTRODUCTION section is found, MAIN MENU is searched for.
 *               If no MAIN MENU is found, one is dynamically created from
 *               all of the section titles in the menu file.
 *
 *               The MENU_SECTION structure is dynamically allocated
 *               (use mn_proc_section_free to free it).  If the section is
 *               no more than a block of text needing to be displayed
 *               (i.e. INTRODUCTION, *_help, term section, etc) the
 *               menu_sec->section_type will be set to 
 *               MENU_SECTION_TYPE_HELP, and a pointer to a buffer filled
 *               with the text to be displayed will be placed in 
 *               menu_sec->display.  The menu_sec->parent and
 *               menu_sec->next will both be set to the previous calling
 *               section.  (i.e. for INTRODUCTION, parent = MAIN MENU).
 *
 *               If the section is a selection section, 
 *               menu_sec->section_type will be set to 
 *               MENU_SECTION_TYPE_SELECTION.  The menu_sec->parent will
 *               be set to the parent menu, menu_sec->next will be NULL.
 *               Help will be searched for for the section, and if found
 *               the name of the section will be returned in 
 *               menu_sec->help_title and a rowsize struct will be placesd
 *               in menu_sec->help_sec.  For all selection type sections,
 *               menu_sec->display will be set to NULL. 
 *               menu_sec->num_choices is set to the number of items the
 *               user has to choose from, and MENU_SELECTION structures are
 *               allocated and filled, with the pointer to the first being
 *               placed in menu_sec->selection.
 *
 *               PLEASE NOTE THAT WHILE THE MENU_SELECTION STRUCT IS
 *               DESIGNED AS A DLL, THIS STRUCT IS NOT DESIGNED FOR USE
 *               WITH FREEFORM'S MACROS FOR DLL NAVIGATION.
 *               The macros MENU_NEXT_SELECTION and MENU_PREV_SELECTION
 *               should be used instead.  The MENU_SELECTION DLL is not
 *               circular; the next and previous pointers on both ends are
 *               set to NULL.
 *
 *               The parent_menu string in the arguments to this function
 *               is used to override the natural search for the parent menu.
 *               If the section requested to process is a help section in
 *               a file (ending in _help) the parent menu is not capabile
 *               of being found by this function.  This is the only time
 *               when this argument is necessary.
 *
 *               Any peice of information which cannot be determined or is
 *               innapropriate or irrelevant to the section being processed
 *               will be returned as NULL in the MENU_SECTION and any
 *               MENU_SELECTION structures.
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none
 *
 * AUTHOR:      Kevin Frender kbf@ngdc.noaa.gov
 *
 * COMMENTS:    DO NOT USE FREEFORM DLL MACROS WITH THE 
 *              MENU_SELECTION STRUCT!
 *
 * KEYWORDS: menu index help
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "mn_sec_process"
int mn_sec_process(MENU_INDEX_PTR mindex, char *section_title, ROW_SIZES_PTR rowsize, char *parent_menu, MENU_SECTION_PTR *menu_sec)
{
	MENU_SECTION_PTR section = NULL;
	MENU_SELECTION_PTR selec = NULL;
	MENU_SELECTION_PTR prev_selec = NULL;
	ROW_SIZES rowsize_1;
	int i;
	int num_sel;
	int sel_num;
	ulong l;
	char *scratch;
	char *buffer;
	char *title;
	char *position;
	char *posinbuffer;
	char *eolpos;
	char main_menu_exists = 0;

	assert((mindex) && (mindex->check_address = (void *)mindex) && (menu_sec));

	*(menu_sec) = NULL;

	if(!(scratch = (char *)mAlloc((size_t)500))){
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "for scratch");
		return(ERR_MEM_LACK);
	}
	
	if(!(section = (MENU_SECTION_PTR)mAlloc(sizeof(MENU_SECTION)))){
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "for menu section");
		fRee(scratch);
		return(ERR_MEM_LACK);
	}
	
	section->check_address = (void *)section;
	section->mindex = mindex;
	section->extra_info = NULL;
	section->title = NULL;
	section->section = NULL;
	section->parent = NULL;
	section->next = NULL;
	section->help_exists = 0;
	section->dynamic_section = 0;
	section->help_title = NULL;
	section->help_sec = NULL;
	section->display = NULL;
	section->num_choices = 0;
	section->selection = NULL;
	section->section_type = MENU_SECTION_TYPE_SELECTION;

	if(rowsize){
		rowsize_1.start = rowsize->start;
		rowsize_1.num_bytes = rowsize->num_bytes;
	}
	else{
		if(section_title){
			i = mn_index_get_offset(mindex, section_title, &rowsize_1);
			if((i != 0) && (i != ERR_MN_SEC_NFOUND)){
				MENU_ERR_PUSH(ROUTINE_NAME, ERR_UNKNOWN, "Looking for specified section");
				fRee(scratch);
				mn_proc_section_free(section);
				return(ERR_UNKNOWN);
			}


			if(i != 0){ /* section not found */
				if(!strcmp(section_title, "INTRODUCTION")){
					/* Search for MAIN MENU later */
					section_title = NULL;
				}
				if(section_title)
					if(!strcmp(section_title, "MAIN MENU")){
						/* Dynamically create MAIN MENU */
						section_title = NULL;
					}

				if(section_title){
					sprintf(scratch, "section name: %s", section_title);
					MENU_ERR_PUSH(ROUTINE_NAME, ERR_MN_SEC_NFOUND, scratch);
					fRee(scratch);
					mn_proc_section_free(section);
					return(ERR_MN_SEC_NFOUND);
				}
			}
		}
	}
	
	if((!section_title) && (!rowsize)){ /* search for INTRODUCTION */
		i = mn_index_get_offset(mindex, "INTRODUCTION", &rowsize_1);
		if((i != 0) && (i != ERR_MN_SEC_NFOUND)){
			MENU_ERR_PUSH(ROUTINE_NAME, ERR_UNKNOWN, "Looking for INTRODUCTION");
			fRee(scratch);
			mn_proc_section_free(section);
			return(ERR_UNKNOWN);
		}
		
		if(i == ERR_MN_SEC_NFOUND){ /* search for MAIN MENU */
			i = mn_index_get_offset(mindex, "MAIN MENU", &rowsize_1);
			if((i != 0) && (i != ERR_MN_SEC_NFOUND)){
				MENU_ERR_PUSH(ROUTINE_NAME, ERR_UNKNOWN, "Looking for MAIN MENU");
				fRee(scratch);
				mn_proc_section_free(section);
				return(ERR_UNKNOWN);
			}

			if(i == ERR_MN_SEC_NFOUND){ /* create a dynamic MAIN MENU */
				if(mn_sec_titles_to_buf(mindex, NULL, &num_sel, &buffer)){
					MENU_ERR_PUSH(ROUTINE_NAME, ERR_UNKNOWN,"Retrieving section titles");
					fRee(scratch);
					mn_proc_section_free(section);
					return(ERR_UNKNOWN);
				}
				if(!(section->section = (char *)mAlloc((size_t)20))){
					MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating section title");
					fRee(scratch);
					mn_proc_section_free(section);
					return(ERR_MEM_LACK);
				}

				strcpy(section->section, "MAIN MENU");

				if(!(section->title = (char *)mAlloc((size_t)20))){
					MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating section title");
					fRee(scratch);
					mn_proc_section_free(section);
					return(ERR_MEM_LACK);
				}

				strcpy(section->title, "Main Menu");
				section->dynamic_section = 1;
				section->num_choices = num_sel;
				sel_num = 0;

				posinbuffer = buffer;
				prev_selec = NULL;
				/* Process the buffer to make a MAIN MENU */
				while((position = strstr(posinbuffer, "\n"))){
					/* Fill scratch with next line in buffer */
					strncpy(scratch, posinbuffer, (size_t)(position - posinbuffer));
					scratch[position - posinbuffer] = '\0';

					/* Advance posinbuffer to next line */
					posinbuffer = position;
					while((posinbuffer[0] < ' ') && (posinbuffer[0] != '\0'))
						posinbuffer++;

					 /* Allocate new selection */
					if(!(selec = (MENU_SELECTION_PTR)mAlloc(sizeof(MENU_SELECTION)))){
						MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating selection");
						fRee(scratch);
						mn_proc_section_free(section);
						fRee(buffer);
						return(ERR_MEM_LACK);
					}

					selec->previous_selection = prev_selec;
					if(prev_selec)
						prev_selec->next_selection = selec;
					else /* first struct */
						section->selection = selec;

					/* set up the selection node */
					selec->check_address = (void *)selec;
					selec->next_selection = NULL;
					selec->help_exists = 0;
					selec->help_title = NULL;
					selec->help_sec = NULL;
					selec->additional_info = NULL;
					selec->selection_type = MENU_DIRECTIVE_TERM_SECTION;
					selec->selection_num = sel_num++;
					selec->ascii_data_file = 0;
					selec->extra_info = NULL;

					if(!(selec->user_display = (char *)mAlloc((size_t)(strlen(scratch) + 4)))){
						MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating user_display");
						fRee(scratch);
						mn_proc_section_free(section);
						fRee(buffer);
						return(ERR_MEM_LACK);
					}

					if(!(selec->next_event = (char *)mAlloc((size_t)(strlen(scratch) + 4)))){
						MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating next_event");
						fRee(scratch);
						mn_proc_section_free(section);
						fRee(buffer);
						return(ERR_MEM_LACK);
					}

					prev_selec = selec;
					strcpy(selec->user_display, scratch);
					strcpy(selec->next_event, scratch);
				} /* End processing of buffer */

				fRee(buffer);
				*(menu_sec) = section;
				fRee(scratch);
				section->num_choices = sel_num;
				return(0);
			} /* End creation of dynamic MAIN MENU */
		} /* End search for MAIN MENU */
	} /* End search for INTRODUCTION */

	/* Read the section into a buffer.  If the section is larger
	 * than mindex->max_buffer_size, then it is processed until the
	 * end of the buffer is reached. */
	rowsize_1.num_bytes *= -1; /* Specify we want the title */
	i = mn_section_get(mindex, NULL, &rowsize_1, &buffer);

	if((i) && (i != ERR_MN_BUFFER_TRUNCATED)){
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_UNKNOWN, "Retrieving section");
		fRee(scratch);
		mn_proc_section_free(section);
		return(ERR_MEM_LACK);
	}

	/* OK, so now we have the section in a buffer.  If it was
	 * more than mindex->max_buffer_size, it was truncated. */

	/* See if MAIN MENU exists. */
	i = mn_index_get_offset(mindex, "MAIN MENU", &rowsize_1);
	if(!i)
		main_menu_exists = 1;
	else
		section->section_type = MENU_SECTION_TYPE_HELP;
		/* If MAIN MENU does not exist in the file, one was created
		 * dynamically and therefore this is a help-type section
		 * (one to be displayed as text) */

	/* Get the title */
	position = strstr(buffer, mindex->file_eol_str);
	if(!position){
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_GENERAL, "Retrieving section title");
		fRee(scratch);
		mn_proc_section_free(section);
		fRee(buffer);
		return(ERR_GENERAL);
	}

	position[0] = '\0';
	if(!(posinbuffer = ++position)){
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_GENERAL, "Empty section");
		fRee(scratch);
		mn_proc_section_free(section);
		fRee(buffer);
		return(ERR_GENERAL);
	}

	while(posinbuffer[0] < ' ')
		posinbuffer++;

	strcpy(scratch, buffer + 1);
	/* See if we are in a help section */
	position = strrchr(scratch, '_');
	if(position){
		if(!strncmp(position, "_help", (size_t)5)){
			section->section_type = MENU_SECTION_TYPE_HELP;
		}
	}
	
	/* INTRODUCTION is also considered a help section */
	if(!strncmp(scratch, "INTRODUCTION", (size_t)12)){
		section->section_type = MENU_SECTION_TYPE_HELP;
		if(!(section->next = (char *)mAlloc((size_t)20))){
			MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating next title");
			fRee(scratch);
			fRee(buffer);
			mn_proc_section_free(section);
			return(ERR_MEM_LACK);
		}
		/* Set the next section to view to be MAIN MENU */
		strcpy(section->next, "MAIN MENU");
	}

	if(parent_menu){ /* We were sent a parent menu */
		if(!(section->parent = (char *)mAlloc((size_t)(strlen(parent_menu) + 4)))){
			MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating parent title");
			fRee(scratch);
			fRee(buffer);
			mn_proc_section_free(section);
			return(ERR_MEM_LACK);
		}
		strcpy(section->parent, parent_menu);
	}
	
	/* Store the title */
	if(section->section_type != MENU_SECTION_TYPE_HELP){
		if(!strcmp(scratch, "MAIN MENU")){ /* section is MAIN MENU */
			if(strcmp(mindex->menu_name, MENU_UNTITLED)){
				if(!(section->title = (char *)mAlloc((size_t)(strlen(mindex->menu_name) + 15)))){
					MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating section name");
					fRee(scratch);
					mn_proc_section_free(section);
					fRee(buffer);
					return(ERR_MEM_LACK);
				}
				strcpy(section->title, mindex->menu_name);
				strcat(section->title, ": Main Menu");
			}
			else{
				if(!(section->title = (char *)mAlloc((size_t)20))){
					MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating section name");
					fRee(scratch);
					mn_proc_section_free(section);
					fRee(buffer);
					return(ERR_MEM_LACK);
				}
				strcpy(section->title, "Main Menu");
			}           
		}
		if(!section->title){
			if(!(section->title = (char *)mAlloc((size_t)(strlen(scratch) + 4)))){
				MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating section title");
				fRee(scratch);
				fRee(buffer);
				mn_proc_section_free(section);
				return(ERR_MEM_LACK);
			}
			strcpy(section->title, scratch);
		}
		if(!(section->section = (char *)mAlloc((size_t)(strlen(scratch) + 4)))){
			MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating section title");
			fRee(scratch);
			fRee(buffer);
			mn_proc_section_free(section);
			return(ERR_MEM_LACK);
		}
		strcpy(section->section, scratch);
	}
	else{ /* Is a help section */
		if(!strcmp(buffer + 1, "INTRODUCTION"))
			i = 20 + strlen(mindex->menu_name);
		else
			i = 4;
			
		if(!(section->title = (char *)mAlloc((size_t)(strlen(buffer + 1) + i)))){
			MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating section title");
			fRee(scratch);
			fRee(buffer);
			mn_proc_section_free(section);
			return(ERR_MEM_LACK);
		}
		
		if(!strcmp(buffer + 1, "INTRODUCTION")){
			if(strcmp(mindex->menu_name, MENU_UNTITLED)){
				strcpy(section->title, mindex->menu_name);
				strcat(section->title, ": Introduction");
			}
			else
				strcpy(section->title, "Introduction");
		}
		else { /* An _help section */
			strcpy(section->title, buffer + 1);
			position = strstr(section->title, "_help");
			if(position)
				position[0] = '\0';
		}
			
		if(!(section->section = (char *)mAlloc((size_t)(strlen(buffer) + 2)))){
			MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating section name");
			fRee(scratch);
			fRee(buffer);
			mn_proc_section_free(section);
			return(ERR_MEM_LACK);
		}
			
		strcpy(section->section, buffer + 1);
		
		/* Check for other section/file reference */
		if(posinbuffer[0] == '>') {
			/* This is a reference */
			
			fRee(buffer);
			if(mn_help_sec_get(mindex, section->section, NULL, &buffer)) {
				MENU_ERR_PUSH(ROUTINE_NAME, ERR_UNKNOWN, "Retrieving help section");
				fRee(scratch);
				mn_proc_section_free(section);
				return(ERR_MEM_LACK);
			}
		}
		else {
			/* Copy the help past the title to the beginning of the buffer */
			for(l = 0; posinbuffer[l]; l++)
				buffer[l] = posinbuffer[l];
				
			buffer[l] = '\0';
		}
			
		section->display = buffer;
		
		/* we are done processing a help section */
		*(menu_sec) = section;
		fRee(scratch);
		return(0);
	}

	/* So it is a selection section we are processing. */
	/* Search for help */
	i = mn_help_sec_find(mindex, section->section, &rowsize_1, scratch);
	if((i != 0) && (i != ERR_MN_SEC_NFOUND)){
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_UNKNOWN, "Searching for help");
		fRee(scratch);
		mn_proc_section_free(section);
		fRee(buffer);
		return(ERR_MEM_LACK);
	}

	if(!i){ /* Help found */
		section->help_exists = 1;
		/* Store the title */
		if(!(section->help_title = (char *)mAlloc((size_t)(strlen(scratch) + 4)))){
			MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating section help title");
			fRee(scratch);
			mn_proc_section_free(section);
			fRee(buffer);
			return(ERR_MEM_LACK);
		}
		strcpy(section->help_title, scratch);

		/* Store the rowsize struct */
		if(!(section->help_sec = (ROW_SIZES_PTR)mAlloc(sizeof(ROW_SIZES)))){
			MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating section help_sec");
			fRee(scratch);
			mn_proc_section_free(section);
			fRee(buffer);
			return(ERR_MEM_LACK);
		}
		section->help_sec->start = rowsize_1.start;
		section->help_sec->num_bytes = rowsize_1.num_bytes;
	} /* End of (help found) */

	/* Now get the parent menu */
	position = strstr(posinbuffer, mindex->file_eol_str);
	if(!position){
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_GENERAL, "Retrieving section parent");
		fRee(scratch);
		mn_proc_section_free(section);
		fRee(buffer);
		return(ERR_GENERAL);
	}
	position[0] = '\0';

	if(strchr(posinbuffer, '#')){
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_MN_FILE_CORRUPT, "Parent menu not found");
		fRee(scratch);
		fRee(buffer);
		mn_proc_section_free(section);
		return(ERR_MN_FILE_CORRUPT);
	}

	if(!section->parent){ /* If we were not told to override parent menu */
		if(!(section->parent = (char *)mAlloc((size_t)(strlen(posinbuffer) + 15)))){
			MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating section parent");
			fRee(scratch);
			fRee(buffer);
			mn_proc_section_free(section);
			return(ERR_MEM_LACK);
		}
		if(strcmp(posinbuffer, "NULL")){ /* Skip NULL parent */
			strcpy(section->parent, posinbuffer);
		}
		else{ /* If parent was NULL, make it INTRODUCTION */
			i = mn_index_get_offset(mindex, "INTRODUCTION", &rowsize_1);
			if(!i) /* INTRODUCTION exists */
				strcpy(section->parent, "INTRODUCTION");
			else
				section->parent = NULL;
		}
	}

	eolpos = position;
	prev_selec = NULL;
	sel_num = 0;

	/* Fill a selection DLL */
	while((posinbuffer = ++eolpos)){
		while(posinbuffer[0] < ' '){
			if(posinbuffer[0] == '\0'){
				break; /* End of buffer; done processing */
			}
			posinbuffer++;
		}
		if(posinbuffer[0] == '\0') break; /* Done processing */

		eolpos = strstr(posinbuffer, mindex->file_eol_str);
		if(!eolpos){
			MENU_ERR_PUSH(ROUTINE_NAME, ERR_GENERAL, "Parsing section directives");
			fRee(scratch);
			mn_proc_section_free(section);
			fRee(buffer);
			return(ERR_GENERAL);
		}
		eolpos[0] = '\0';

		/* Allocate new selection */
		if(!(selec = (MENU_SELECTION_PTR)mAlloc(sizeof(MENU_SELECTION)))){
			MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating selection");
			fRee(scratch);
			mn_proc_section_free(section);
			fRee(buffer);
			return(ERR_MEM_LACK);
		}

		selec->previous_selection = prev_selec;
		if(prev_selec)
			prev_selec->next_selection = selec;
		else /* first struct */
			section->selection = selec;

		prev_selec = selec;

		/* set up the selection node */
		selec->check_address = (void *)selec;
		selec->next_selection = NULL;
		selec->help_exists = 0;
		selec->help_title = NULL;
		selec->additional_info = NULL;
		selec->help_sec = NULL;
		selec->selection_num = sel_num++;
		selec->ascii_data_file = 0;
		selec->extra_info = NULL;

		/* Search for help */
		i = mn_help_sec_find(mindex, posinbuffer, &rowsize_1, scratch);
		if((i != 0) && (i != ERR_MN_SEC_NFOUND)){
			MENU_ERR_PUSH(ROUTINE_NAME, ERR_UNKNOWN, "Searching for help");
			fRee(scratch);
			mn_proc_section_free(section);
			fRee(buffer);
			return(ERR_MEM_LACK);
		}

		if(!i){ /* Help found */
			selec->help_exists = 1;
			/* Store the title */
			if(!(selec->help_title = (char *)mAlloc((size_t)(strlen(scratch) + 4)))){
				MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating selection help title");
				fRee(scratch);
				mn_proc_section_free(section);
				fRee(buffer);
				return(ERR_MEM_LACK);
			}
			strcpy(selec->help_title, scratch);

			/* Store the rowsize struct */
			if(!(selec->help_sec = (ROW_SIZES_PTR)mAlloc(sizeof(ROW_SIZES)))){
				MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating selection help_sec");
				fRee(scratch);
				fRee(buffer);
				return(ERR_MEM_LACK);
			}
			selec->help_sec->start = rowsize_1.start;
			selec->help_sec->num_bytes = rowsize_1.num_bytes;
		} /* End of (help found) */

		position = strchr(posinbuffer, '#');
		if(!position){
			/* Must not be a hierarchy-type section */
			MENU_ERR_PUSH(ROUTINE_NAME, ERR_GENERAL, "Unknown section type");
			fRee(scratch);
			fRee(buffer);
			return(ERR_GENERAL);
		}
		position[0] = '\0';
		title = position - 1;
		position++;

		while(title[0] < ' '){
			title[0] = '\0';
			title--;
		}

		title = posinbuffer;

		if(!(selec->user_display = (char *)mAlloc((size_t)(strlen(title) + 4)))){
			MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating user_display");
			fRee(scratch);
			mn_proc_section_free(section);
			fRee(buffer);
			return(ERR_MEM_LACK);
		}
		strcpy(selec->user_display, title);

		switch(position[0]){ /* Determine directive type */
			case '#':
				selec->selection_type = MENU_DIRECTIVE_NEW_MENU_FILE;
				position++;
				if(position[0] == '&')
					position++;
				break;
			case '*':
				selec->selection_type = MENU_DIRECTIVE_NEW_SECTION;
				position++;
				break;
			case 'H':
				selec->selection_type = MENU_DIRECTIVE_TERM_SECTION;
				position += 2; /* Skip the star too */
				break;
			case '$':
				selec->ascii_data_file = 1;
				/* NO BREAK ON PURPOSE */
			case '&':
				position++;
				/* NO BREAK ON PURPOSE */
			case '\\':
				selec->selection_type = MENU_DIRECTIVE_DATA_FILE;
				break;
			default:
				MENU_ERR_PUSH(ROUTINE_NAME, ERR_GENERAL, "Unknown directive");
				fRee(scratch);
				mn_proc_section_free(section);
				fRee(buffer);
				return(ERR_MEM_LACK);
		}

		if(!(selec->next_event = (char *)mAlloc((size_t)(strlen(position) + 4)))){
			MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating next_event");
			fRee(scratch);
			mn_proc_section_free(section);
			fRee(buffer);
			return(ERR_MEM_LACK);
		}
		strcpy(selec->next_event, position);
		if(selec->selection_type == MENU_DIRECTIVE_DATA_FILE){ /* additional info? */
			/* If additional information exists, seperate it from the file
			 * name and store it away */
			if((position = strchr(selec->next_event, '+'))){
				if(!(selec->additional_info = (char *)mAlloc((size_t)(strlen(position) + 4)))){
					MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating additional_info");
					fRee(scratch);
					mn_proc_section_free(section);
					fRee(buffer);
					return(ERR_MEM_LACK);
				}
				strcpy(selec->additional_info, position + 1);
				
				position[0] = '\0';
			}
		}
	}
	

	/* Done processing */
	fRee(buffer);
	*(menu_sec) = section;
	fRee(scratch);
	section->num_choices = sel_num;
	return(0);
}


/*
 * NAME:  mn_proc_section_free
 *
 * PURPOSE:     To free a MENU_SECTION struct and associated members.
 *
 * USAGE:       int mn_proc_section_free(MENU_SECTION_PTR menu_sec)
 *
 * RETURNS:     0 if successful; >0 on error.
 *
 * DESCRIPTION:  See PURPOSE.
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none
 *
 * AUTHOR:      Kevin Frender kbf@ngdc.noaa.gov
 *
 * COMMENTS:
 *
 * KEYWORDS: menu
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "mn_proc_section_free"
int mn_proc_section_free(MENU_SECTION_PTR menu_sec)
{
	MENU_SELECTION_PTR sel;

	assert((menu_sec) && (menu_sec->check_address = (void *)menu_sec));

	if(menu_sec->display)
		fRee(menu_sec->display);

	if(menu_sec->section) 
		fRee(menu_sec->section);
		
	if(menu_sec->title)
		fRee(menu_sec->title);
		
	/* Parent and next can sometimes be the same buffer */
	if(menu_sec->parent){
		if(menu_sec->parent == menu_sec->next){
			fRee(menu_sec->parent);
		}
		else{
			fRee(menu_sec->parent);
			if(menu_sec->next){
				fRee(menu_sec->next);
			}
		}
	}
	else{
		if(menu_sec->next){
			fRee(menu_sec->next);
		}
	}
	if(menu_sec->help_title) fRee(menu_sec->help_title);
	if(menu_sec->help_sec) fRee(menu_sec->help_sec);

	if(menu_sec->selection){
		MENU_REWIND_SELECTION_DLL(menu_sec->selection);
		while(menu_sec->selection){
		
            sel = menu_sec->selection;
            
            menu_sec->selection = menu_sec->selection->next_selection;

			mn_selection_free(sel);
		}  
	}
	fRee(menu_sec);
	
	return(0);
}


/*
 * NAME:        mn_get_filelength
 *
 * PURPOSE:     To determine the length, in bytes, of a file
 *
 * USAGE:       long mn_get_filelength(FILE *infile)
 *
 * RETURNS:     Number of bytes in the file, else -1 on error
 *
 * DESCRIPTION: This function determines the length of a file, in bytes.
 *              it is different from the os_filelength function in that:
 *
 *                  -This function works on the Mac,
 *                  -This function uses ANSI file handles instead of UNIX,
 *                  -This function assumes the file is opened for MENU_FOPEN_R
 *                      access.
 *
 *              This function is meant only to
 *              provide the approximate size a buffer must be allocated to to hold
 *              a description section.
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none
 *
 * AUTHOR:      Kevin Frender kbf@ngdc.noaa.gov
 *
 * COMMENTS:    This function is meant only to be used by other mn_ functions, 
 *              and hence its prototype is not in any include file, nor should it
 *              be.
 *
 * KEYWORDS: menu
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "mn_get_filelength"
long mn_get_filelength(FILE *infile)
{
	long total;
	
	if(fseek(infile, 0, SEEK_END)){
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_UNKNOWN, "Seeking end of file");
		return(-1);
	}
	
	total = ftell(infile);
	
	/* Reset file pointer to beginning */   
	if(fseek(infile, 0, SEEK_SET)){
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_UNKNOWN, "Seeking beginning of file");
		return(-1);
	}
	
	return(total);
}

/*
 * NAME:        mn_selection_free
 *
 * PURPOSE:     To free a MENU_SELECTION struct
 *
 * USAGE:       int mn_selection_free(MENU_SELECTION_PTR selection)
 *
 * RETURNS:     0 if all is OK, >< 0 on error
 *
 * DESCRIPTION: This function frees a MENU_SELECTION struct and all associated members.
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none
 *
 * AUTHOR:      Kevin Frender kbf@ngdc.noaa.gov
 *
 * COMMENTS:    
 *
 * KEYWORDS: menu
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "mn_selection_free"
int mn_selection_free(MENU_SELECTION_PTR selection)
{
	assert(selection && (selection->check_address == (void *)selection));

	if(selection->additional_info) fRee(selection->additional_info);
	if(selection->user_display) fRee(selection->user_display);
	if(selection->next_event) fRee(selection->next_event);
	if(selection->help_title) fRee(selection->help_title);
	if(selection->help_sec) fRee(selection->help_sec);
	
	/* Set all the pointers to NULL as a debugging precaution */
	selection->check_address = NULL;
	selection->next_selection = NULL;
	selection->previous_selection = NULL;
	selection->user_display = NULL;
	selection->next_event = NULL;
	selection->additional_info = NULL;
	selection->help_title = NULL;
	selection->help_sec = NULL;
	selection->extra_info = NULL;
	
	fRee(selection);

	return(0);
}

/*
 * NAME:        mn_selection_copy
 *
 * PURPOSE:     To copy a MENU_SELECTION struct
 *
 * USAGE:       MENU_SELECTION_PTR mn_selection_copy(MENU_SELECTION_PTR selection)
 *
 * RETURNS:     MENU_SELECTION_PTR to the newly copied MENU_SELECTION_PTR, or
 *                  NULL on error.
 *
 * DESCRIPTION: This function copies a MENU_SELECTION struct and all associated members.
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none
 *
 * AUTHOR:      Kevin Frender kbf@ngdc.noaa.gov
 *
 * COMMENTS:    
 *
 * KEYWORDS: menu
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "mn_selection_copy"
MENU_SELECTION_PTR mn_selection_copy(MENU_SELECTION_PTR selection)
{
	MENU_SELECTION_PTR newselec;

	assert(selection && (selection->check_address == (void *)selection));

	if(!(newselec = (MENU_SELECTION_PTR)mAlloc(sizeof(MENU_SELECTION)))){
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Copying selection");
		return(NULL);
	}

	newselec->check_address = newselec;
	newselec->next_selection = NULL;
	newselec->previous_selection = NULL;
	newselec->help_exists = selection->help_exists;
	newselec->selection_type = selection->selection_type;
	newselec->ascii_data_file = selection->ascii_data_file;
	newselec->selection_num = selection->selection_num;
	newselec->extra_info = selection->extra_info;
	newselec->user_display = NULL;
	newselec->next_event = NULL;
	newselec->additional_info = NULL;
	newselec->help_title = NULL;
	newselec->help_sec = NULL;

	if(!(newselec->user_display = (char *)mAlloc((size_t)(strlen(selection->user_display) + 2)))){
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Copying selection");
		mn_selection_free(newselec);
		return(NULL);
	}
	strcpy(newselec->user_display, selection->user_display);
	
	if(selection->additional_info){
		if(!(newselec->additional_info = (char *)mAlloc((size_t)(strlen(selection->additional_info) + 2)))){
			MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Copying selection");
			mn_selection_free(newselec);
			return(NULL);
		}
		strcpy(newselec->additional_info, selection->additional_info);
	}
	
	if(selection->next_event){
		if(!(newselec->next_event = (char *)mAlloc((size_t)(strlen(selection->next_event) + 2)))){
			MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Copying selection");
			mn_selection_free(newselec);
			return(NULL);
		}
		strcpy(newselec->next_event, selection->next_event);
	}

	if(selection->help_title){
		if(!(newselec->help_title = (char *)mAlloc((size_t)(strlen(selection->help_title) + 2)))){
			MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Copying selection");
			mn_selection_free(newselec);
			return(NULL);
		}
		strcpy(newselec->help_title, selection->help_title);
	}

	if(selection->help_sec){
		if(!(newselec->help_sec = (ROW_SIZES_PTR)mAlloc(sizeof(ROW_SIZES)))){
			MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Copying selection");
			mn_selection_free(newselec);
			return(NULL);
		}
		newselec->help_sec->start = selection->help_sec->start;
		newselec->help_sec->num_bytes = selection->help_sec->num_bytes;
	}

	return(newselec);
}

/*
 * NAME:        mn_section_rebuild
 *
 * PURPOSE:     To rebuild a menu section (TEXT) from a MENU_SECTION struct
 *
 * USAGE:       int mn_section_rebuild(MENU_SECTION_PTR msection, char **buffer)
 *
 * RETURNS:     int, 0 if all is OK, >< 0 on error.  Pointer to allocated and filled
 *					buffer returned in *(buffer).
 *
 * DESCRIPTION: This function allocates a buffer and "prints" the section represented
 *					in MENU_SELECTION_PTR msection to it.
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none
 *
 * AUTHOR:      Kevin Frender kbf@ngdc.noaa.gov
 *
 * COMMENTS:    
 *
 * KEYWORDS: menu
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "mn_section_rebuild"
int mn_section_rebuild(MENU_SECTION_PTR msection, char **buffer)
{
	long bufflen = 1; /* Account for the * on the front */
	char *buff = NULL;

	assert(msection && (msection->check_address = (void *)msection->check_address));
	
	/* Determine the size we need to make this buffer */
	bufflen += strlen(msection->section);
	bufflen += msection->mindex->file_eollen;
	
	if(msection->section_type == MENU_SECTION_TYPE_SELECTION) {
		if(msection->parent)
			bufflen += strlen(msection->parent);
		else
			bufflen += 5; /* NULL */
			
		bufflen += msection->mindex->file_eollen;
		
		MENU_REWIND_SELECTION_DLL(msection->selection);
		
		while(1) {
			bufflen += strlen(msection->selection->user_display);
			bufflen += strlen(msection->selection->next_event);
			if(msection->selection->additional_info)
				bufflen += strlen(msection->selection->additional_info);
			bufflen += 4; /* Worst case ##&\ type seperator */
			bufflen += msection->mindex->file_eollen;
			
			if(!MENU_NEXT_SELECTION(msection->selection))
				break;
			msection->selection = MENU_NEXT_SELECTION(msection->selection);
		}
	}
	else { /* HELP type section */
		bufflen += strlen(msection->display);
	}
	
	bufflen += msection->mindex->file_eollen;
	

	/* Time to allocate the buffer */	
	if(!(buff = (char *)mAlloc((size_t)bufflen))) {
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating reconstructive buffer");
		return(ERR_MEM_LACK);
	}

	/* Put out the section title */		
	strcpy(buff, "*");
	strcat(buff, msection->section);
	strcat(buff, msection->mindex->file_eol_str);

	/* Put out the section body */
	if(msection->section_type == MENU_SECTION_TYPE_SELECTION) {
		if(msection->parent)
			strcat(buff, msection->parent);
		else
			strcat(buff, "NULL");
	
		strcat(buff, msection->mindex->file_eol_str);

		MENU_REWIND_SELECTION_DLL(msection->selection);
		
		while(1) {
			strcat(buff, msection->selection->user_display);
			strcat(buff, "#");
			
			switch(msection->selection->selection_type) {
				case MENU_DIRECTIVE_NEW_SECTION:
					strcat(buff, "*");
					break;
				case MENU_DIRECTIVE_TERM_SECTION:
					strcat(buff, "H*");
					break;
				case MENU_DIRECTIVE_NEW_MENU_FILE:
					strcat(buff, "#");
					break;
				case MENU_DIRECTIVE_DATA_FILE:
					if(msection->selection->ascii_data_file)
						strcat(buff, "$");
					break;
				default:
					MENU_ERR_PUSH(ROUTINE_NAME, ERR_UNKNOWN, "Unknown menu item type");
					return(ERR_UNKNOWN);
			}
			
			strcat(buff, msection->selection->next_event);

			if(msection->selection->additional_info){
				strcat(buff, "+");
				strcat(buff, msection->selection->additional_info);
			}
			
			strcat(buff, msection->mindex->file_eol_str);
						
			if(!MENU_NEXT_SELECTION(msection->selection))
				break;
			msection->selection = MENU_NEXT_SELECTION(msection->selection);
		}
		strcat(buff, msection->mindex->file_eol_str);
	}
	else { /* HELP type section */
		strcat(buff, msection->display);
	}
	
	*(buffer) = buff;
	
	return(0);
}
