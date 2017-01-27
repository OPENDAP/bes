/*
 * FILENAME:        menuindx.c
 *              
 * PURPOSE:     To handle all basic functions of menu indexes
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

#define MENU_EOL_CR 13
#define MENU_EOL_LF 10

/*
 * NAME:        mn_index_remove 
 *              
 * PURPOSE:     removes all MENU INDEX sections from the menu
 *
 * USAGE:       int mn_index_remove(char *inputfilename, char *outputfilename)
 *
 * RETURNS:     int, 0 on error, 1 if all is ok, but no index, 2 if index 
 *                            was found and removed.
 *
 * DESCRIPTION: rewrites the file TO A NEW FILE NAME without the menu index.
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
#define ROUTINE_NAME "mn_index_remove"
int mn_index_remove(char *filename, char *outfilename)
{
	FILE *infile;
	FILE *outfile;
	char *scratch;
	char datasec[40];
	char scratchtwo[500];
	char eat_section = 0;
	char menu_index_existed = 0;
	char *file_eol_str;
	long seclen;
	
	assert((filename) && (outfilename));
	
	if(!(scratch = (char *)mAlloc((size_t)(sizeof(char) * 500)))) {
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Creating scratch string");
		  return(0);
	}

	if(!(infile = fopen(filename, MENU_FOPEN_R))){
		sprintf(scratch, "Menu file '%s' not found.\n", filename);
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_GENERAL, scratch);
		return(0);
	}
	if(!(outfile = fopen(outfilename, MENU_FOPEN_W))){
		sprintf(scratch, "Can't open '%s' for writing.\n", outfilename);
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_GENERAL, scratch);
		fclose(infile); // jhrg 3/18/11
		return(0);
	}
	if(!(file_eol_str = mn_get_file_eol_str(filename))){
		sprintf(scratch, "Could not determine EOL characters for '%s'.\n", filename);
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_GENERAL, scratch);
		fclose(infile); // jhrg 3/18/11
		fclose(outfile);
		return(0);
	}
	
	strcpy(datasec, MENU_DATA_SECTION);
	strcat(datasec, file_eol_str);

	while(mn_binary_fgets(scratch, 490, infile, file_eol_str)) {
		if(scratch[0] == '*'){ /* section name */
			eat_section = 0;
			if(!(strncmp(scratch, MENU_INDEX_SECTION, MENU_INDEX_SECTION_LEN))){
				/* Is a MENU_INDEX section */
				eat_section = 1;
				menu_index_existed = 1;
			}
			if(strstr(scratch, datasec)) { 
				/* is a data section */
				
				fputs(scratch, outfile); /* Write out the title */
				
				if(!mn_binary_fgets(scratchtwo, 490, infile, file_eol_str)) {
					/* Corrupt data section */
					
					sprintf(scratchtwo, "%s: Data section corrupt.\n", scratch);
					MENU_ERR_PUSH(ROUTINE_NAME, ERR_MN_FILE_CORRUPT, scratchtwo);
					return(0);
				}
				
				seclen = atol(scratchtwo);
				
				fputs(scratchtwo, outfile); /* Write out the section length */
				
				/* Copy the data section over to the new file in chunks */
				for(; seclen > 490; seclen -= 490) {
					if((fread(scratchtwo, 490, 1, infile)) < 490) {
						sprintf(scratchtwo, "%s: Data section corrupt.\n", scratch);
						MENU_ERR_PUSH(ROUTINE_NAME, ERR_MN_FILE_CORRUPT, scratchtwo);
						return(0);
					}
					
					fwrite(scratchtwo, 490, 1, outfile);
				}
				
				if(seclen) {
					/* Finish copying it over */
					if((fread(scratchtwo, (size_t)seclen, 1, infile)) < 490) {
						sprintf(scratchtwo, "%s: Data section corrupt.\n", scratch);
						MENU_ERR_PUSH(ROUTINE_NAME, ERR_MN_FILE_CORRUPT, scratchtwo);
						return(0);
					}
				
					fwrite(scratchtwo, (size_t)seclen, 1, outfile);
				}
				
				/* We don't want to put out the section name again- 
				 * continue with loop */
				continue;				
			}
		}
		if(!eat_section) fputs(scratch, outfile);
	}
	fclose(infile);
	fclose(outfile);
	fRee(file_eol_str);

	return(1 + menu_index_existed);
}

/*
 * NAME:        mn_index_make
 *              
 * PURPOSE:     creates a MENU INDEX section at the front of a menu file
 *              or in memory only, and create and return a MENU_INDEX struct
 *
 * USAGE:       MENU_INDEX_PTR mn_index_make(char *inputfilename, ulong max_buf_size, char *outputfilename)
 *              If outputfilename is not NULL, the menu index is written to
 *    the front of outputfilename, followed by inputfilename (the
 *    menu itself).  The MENU_INDEX struct then reflects the new, 
 *    outputfilename menu.  max_buf_size is the maximum size of 
 *    the buffers to be used.  If outputfilename is NULL, the index
 *    is attempted to be read out of the file, or, if it cannot, will
 *    be created in memory only.
 *
 * RETURNS:     MENU_INDEX_PTR to the created structure, or NULL on error.
 *
 * DESCRIPTION: the MENU INDEX section created has the following structure:
 *              (line 1)      *MENU INDEX
 *              (line 2)      number_of_bytes_in_menu_index #_of_lines_in_menu_index
 *              (line 3)      number_of_bytes_in_an_EOL_when_index_was_created
 *              (even # line) #*section_title
 *              (odd # line)  number_of_bytes_after_menu_index #_of_lines_after_menu_index
 *    			(next to last line) END MENU INDEX
 *              (ending line) TOTALS: #bytes_in_menu #_lines_in_menu 
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none
 *
 * AUTHOR:      Kevin Frender kbf@ngdc.noaa.gov
 *
 * COMMENTS: Because the number of lines and the length of an EOL are recorded
 *           in the menu index, the menu file is completely portable when the
 *           get_offset function is used to determine a section offset.
 *
 *           This function, like all mn_index functions, assumes that no line
 *           in the menu will excede 490 characters in length, unless it is
 *			 inside a _DATASEC section, which is treated as binary data.
 *
 *           Most of the code in this function is handling the allowance of
 *           multiple buffers; actual code to deal with indexes is sparse.
 *          
 * KEYWORDS: menu index
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "mn_index_make" 
MENU_INDEX_PTR mn_index_make(char *filename, ulong max_buf_size, char *outfilename)
{
	FILE *infile = NULL;
	FILE *outfile = NULL;
	ulong offset = 0;
	ulong orig_offset = 0;
	long indlen = 0;
	long seclen = 0;
	ulong numlines = 0;
	long numindlines = 0;
	char *scratch;
	char *offsetstr;
	char *position;
	char *positiontwo;
	MENU_INDEX_PTR mindex = NULL;
	int i, j, curbuff;
	ulong numread;
	int num_allocated = 0;
	ROW_SIZES rowsize;
	char datasec[40];
	char data_sections_exist = 0;
	
	assert(filename);
	
	if(max_buf_size < 1000){
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_GENERAL, "Maximum buffer size too small");
		return(NULL);
	}

	if(!(scratch = (char *)mAlloc((size_t)(sizeof(char) * 500)))){
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "for scratch buffer"); 
		return(NULL);
	}

	if(!(offsetstr = (char *)mAlloc((size_t)(sizeof(char) * 500)))) {
		fRee(scratch);
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "for scratch buffer");
		return(NULL);
	}

	if(!(mindex = (MENU_INDEX_PTR)mAlloc(sizeof(MENU_INDEX)))){
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "For menu index structure");
		fRee(scratch);
		fRee(offsetstr);
		return(NULL);
	}
	
	if(!(mindex->menu_file = (char *)mAlloc((size_t)(strlen(filename) + 3)))) {
		fRee(scratch);
		fRee(offsetstr);
		fRee(mindex);
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "for menu file name");
		return(NULL);
	}


	mindex->check_address = (void *)mindex;
	strcpy(mindex->menu_file, filename);
	mindex->max_buffer_size = max_buf_size;
	mindex->index_corrupt = 0;
	mindex->file_eol_str = NULL;
	mindex->correct_eol_len = 0;
	mindex->num_buffers = 0;
	mindex->num_sections = 0;
	mindex->data_path_one = NULL;
	mindex->data_path_two = NULL;
	mindex->index_origion = 0;
	mindex->index_eol_len = 0;
	mindex->menu_in_mem = 0;
	mindex->lines_in_index = 0;
	mindex->bytes_in_index = 0;
	mindex->lines_in_menu = 0;
	mindex->bytes_in_menu = 0;
	mindex->file_eollen = 0;
	mindex->file_index_exists = 0;
	mindex->index_buffer = NULL;
	mindex->buffer_length = 0;
	mindex->menu_name = NULL;
	mindex->text_content = NULL;
    mindex->binary_menu = 0;

	if(!(infile = fopen(filename, MENU_FOPEN_R))){
		fRee(offsetstr);
		fRee(mindex);
		sprintf(scratch, "Menu file '%s' not found.\n", filename);
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_GENERAL, scratch);
		fRee(scratch);
		return(NULL);
	}
	
	if(!(mindex->file_eol_str = mn_get_file_eol_str(filename))){
		fRee(offsetstr);
		fRee(mindex);
		fclose(infile);
		sprintf(scratch, "Could not determine EOL characters for '%s'.\n", filename);
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_GENERAL, scratch);
		fRee(scratch);
		return(NULL);
	}
	
	/* Set up the data section title string */
	strcpy(datasec, MENU_DATA_SECTION);
	strcat(datasec, mindex->file_eol_str);
	
	mindex->file_eollen = strlen(mindex->file_eol_str);

	if(outfilename){ /* write to file */
		if(!(outfile = fopen(outfilename, MENU_FOPEN_W))){
			fRee(offsetstr);
			fRee(mindex);
			sprintf(scratch, "Can't open '%s' for writing.\n", outfilename);
			fclose(infile);
			MENU_ERR_PUSH(ROUTINE_NAME, ERR_GENERAL, scratch);
			fRee(scratch);
			return(NULL);
		}
	}

	if(!outfilename){ /* just reading/creating index into memory */
		
		if(!mn_binary_fgets(scratch, 490, infile, mindex->file_eol_str)) { /* file empty */
			MENU_ERR_PUSH(ROUTINE_NAME, ERR_MN_FILE_CORRUPT, "menu file empty");
			fRee(scratch);
			fRee(offsetstr);
			fRee(mindex);
			fclose(infile);
			return(NULL);
		}

		while(!strncmp(scratch, MENU_INDEX_SECTION, MENU_INDEX_SECTION_LEN)) { /* index exists in file */
			mindex->file_index_exists = 1;
			
			if(!mn_binary_fgets(scratch, 490, infile, mindex->file_eol_str)){ /* file empty */
				MENU_ERR_PUSH(ROUTINE_NAME, ERR_MN_FILE_CORRUPT, "menu file empty");
				fRee(scratch);
				fRee(offsetstr);
				fRee(mindex);
				fclose(infile);
				return(NULL);
			}

			mindex->bytes_in_index = (ulong)atol(scratch);
			position = strstr(scratch, " ");
			mindex->lines_in_index = (ulong)atol(position);
			
			mn_binary_fgets(scratch, 490, infile, mindex->file_eol_str);
			mindex->index_eol_len = (char)atol(scratch);
			if(mindex->index_eol_len != mindex->file_eollen){ /* correct for EOL */
				mindex->correct_eol_len = 1;
				mindex->bytes_in_index -= (mindex->index_eol_len - mindex->file_eollen) * mindex->lines_in_index;
			}

			/* determine probable number of buffers required */
			i = (int)(mindex->bytes_in_index / mindex->max_buffer_size + 10);
			
			/* ... and allocate pointers for them. */
			if(!(mindex->index_buffer = (char **)mAlloc((size_t)(sizeof(char *) * i)))){
				MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating index buffer array");
				fRee(scratch);
				fRee(offsetstr);
				fclose(infile);
				fRee(mindex);
				return(NULL);
			}

			if(!(mindex->buffer_length = (ulong *)mAlloc((size_t)(sizeof(ulong *) * i)))){
				MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating index buffer size array");
				fRee(scratch);
				fRee(offsetstr);
				fclose(infile);
				fRee(mindex);
				return(NULL);
			}

			for(j = 0; j < i; j++) mindex->index_buffer[j] = NULL; /* initialize */
            
            /* Allocate the first buffer */
			curbuff = 0;
			if(!(mindex->index_buffer[curbuff] = (char *)mAlloc((size_t)mindex->max_buffer_size))){
				MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Creating menu index buffer");
				fRee(mindex->index_buffer);
				fRee(mindex->buffer_length);
				fRee(mindex);
				fRee(offsetstr);
				fRee(scratch);
				fclose(infile);
				return(NULL);
			}
			numread = 0;
			max_buf_size = mindex->max_buffer_size - 105;
			strcpy(mindex->index_buffer[curbuff], "#####");
			
			while(mn_binary_fgets(scratch, 490, infile, mindex->file_eol_str)){ 
				/* read in index line */

				if(!mn_binary_fgets(offsetstr, 490, infile, mindex->file_eol_str)) { 
					/* Menu file bad */
					
					MENU_ERR_PUSH(ROUTINE_NAME, ERR_MN_FILE_CORRUPT, "Menu corrupt");
					for(i = 0; i <= curbuff; i++) fRee(mindex->index_buffer[i]);
					fRee(mindex->index_buffer);
					fRee(mindex->buffer_length);
					fRee(mindex);
					fRee(offsetstr);
					fRee(scratch);
					fclose(infile);
					return(NULL);
				}
				
				if(strstr(scratch, datasec))
					data_sections_exist = 1;

				if(scratch[0] != '#'){ 
					/* END_MENU_INDEX */
					
					mindex->num_buffers = curbuff + 1;
					break;
				}

				/* Check to see how full this buffer is */
				if(numread + strlen(scratch) > max_buf_size){ 
					/* We need a new buffer now */
					
					/* finish off current buffer */
					strcat(mindex->index_buffer[curbuff] + numread, "#* ");
					strcat(mindex->index_buffer[curbuff] + numread, mindex->file_eol_str);
					strcat(mindex->index_buffer[curbuff] + numread, offsetstr);
					strcat(mindex->index_buffer[curbuff] + numread, "#####");

					mindex->buffer_length[curbuff] = strlen(mindex->index_buffer[curbuff]);
					
					curbuff++;
					/* Allocate next buffer */
					if(!(mindex->index_buffer[curbuff] = (char *)mAlloc((size_t)mindex->max_buffer_size))){
						MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Creating menu index buffer");
						for(i = 0; i < curbuff; i++) fRee(mindex->index_buffer[i]);
						fRee(mindex->index_buffer);
						fRee(mindex->buffer_length);
						fRee(mindex);
						fRee(offsetstr);
						fRee(scratch);
						fclose(infile);
						return(NULL);
					}
					numread = 0;
					strcpy(mindex->index_buffer[curbuff], "#####");
				} /* end creation of new buffer */

				/* add newly read-in lines to buffer */
				mindex->num_sections++;
				strcat(mindex->index_buffer[curbuff] + numread, scratch);
				numread += strlen(scratch);
				strcat(mindex->index_buffer[curbuff] + numread, offsetstr);
				numread += strlen(offsetstr);
			} /* end of while(still reading in menu index) loop */

			if(!mindex->num_buffers){ /* menu index corrupt */
				mindex->index_corrupt = 1;
				break;
			}

			if(strncmp(offsetstr, "TOTALS:", 7)){ /* need to determine menu end */
				/* Backward compatibility for really old menu indexes */
				
				position = strrchr(mindex->index_buffer[curbuff], '*'); 
				
				position = strchr(position, mindex->file_eol_str[mindex->file_eollen - 1]);
				position++;
				offset = (ulong)atol(position);
				orig_offset = offset;
				position = strstr(position, " ");
				numlines = (ulong)atol(position);
				offset += mindex->bytes_in_index;

				if(mindex->correct_eol_len) { /* need to correct for EOL */
					offset -= (mindex->index_eol_len - mindex->file_eollen) * mindex->lines_in_index;
					i = mindex->index_eol_len - mindex->file_eollen;
				}

				if(fseek(infile, offset, SEEK_SET)) { /* can't find end */
					mindex->index_corrupt = 1; /* file index corrupt */
					break;
				}

				while(mn_binary_fgets(scratch, 490, infile, mindex->file_eol_str)) { /* while not the end... */
					orig_offset += strlen(scratch) + i;
					numlines++;
				}

				sprintf(offsetstr, "TOTALS: %ld %ld\n", orig_offset, numlines);

			} /* end of TOTALS nonexistant fixup */
			
			/********************************************************
			 * Done reading in menu index.  Check to see:
			 * if data sections exist, make sure that the 
			 * EOLs have not changed since we indexed.
			 ********************************************************/
			if(mindex->correct_eol_len && data_sections_exist) {
				MENU_ERR_PUSH(ROUTINE_NAME, ERR_MN_FILE_CORRUPT, 
						"Menu contains binary data, but End Of Line characters have changed.");
				for(i = 0; i < curbuff; i++) fRee(mindex->index_buffer[i]);
				fRee(mindex->index_buffer);
				fRee(mindex->buffer_length);
				fRee(mindex);
				fRee(offsetstr);
				fRee(scratch);
				fclose(infile);
				return(NULL);
			}
			 
			/********************************************************
			 * Check the last entry to see if the index is corrupt
			 * and needs to be rebuilt.
			 ********************************************************/
			position = strrchr(mindex->index_buffer[curbuff], '*');
			positiontwo = position;
			
			/* Go to next line of buffer */  
			position = strchr(position, mindex->file_eol_str[mindex->file_eollen - 1]);
			position++;
			
			/* Read in offset bytes */
			offset = (ulong)atol(position);
			position = strchr(position, ' ');
			
			/* Read in offset lines */
			numlines = (ulong)atol(position);
			
			if(mindex->correct_eol_len) { 
				/* need to correct for EOL */
				
				offset -= (mindex->index_eol_len - mindex->file_eollen) * numlines;
				i = mindex->index_eol_len - mindex->file_eollen;
			}
			
			/* Correct for index on front of file */
			offset += mindex->bytes_in_index;

			/* Seek to offset */
			if(fseek(infile, offset, SEEK_SET)) { 
				/* can't find end - file index corrupt */
				
				mindex->index_corrupt = 1; 
				break;
			}

			/* Read in line at offset */
			if(!(mn_binary_fgets(scratch, 490, infile, mindex->file_eol_str))){ 
				/* can't get line - file index corrupt */
				
				mindex->index_corrupt = 1;
				break;
			}

			if(scratch[0] != '*'){ 
				/* file index corrupt */
				
				mindex->index_corrupt = 1;
				break;
			}
			
			i = 0;
			
			for(i = 0; ; i++) {
				/* Check to see that the section title is what we expected it to be */
				
				if(positiontwo[i] != scratch[i]) {
					i = 0;
					break;
				}
				if(positiontwo[i] < ' ') /* We have hit a newline */
					break;
			}
			if(i == 0) {
				/* A difference was found- index is corrupt */
				
				mindex->index_corrupt = 1;
				break;
			}
			 
			/**********************************************************
			 * Done checking index - It is not corrupt.  Finish it off.
			 **********************************************************/
			strcat(mindex->index_buffer[curbuff] + numread, "#* ");
			strcat(mindex->index_buffer[curbuff] + numread, mindex->file_eol_str);
			position = strchr(offsetstr, ' ');
			strcat(mindex->index_buffer[curbuff] + numread, ++position);
			strcat(mindex->index_buffer[curbuff] + numread, "#####");

			/* Set the buffer length */			
			mindex->buffer_length[curbuff] = strlen(mindex->index_buffer[curbuff]);
			
			/* reAllocate current buffer to minimum possible size */
			if(!(mindex->index_buffer[curbuff] = (char *)reAlloc(mindex->index_buffer[curbuff], (size_t)(mindex->buffer_length[curbuff] + 5)))){
				MENU_ERR_PUSH(ROUTINE_NAME, ERR_GENERAL, "Reallocating index buffer");
				for(i = 0; i <= curbuff; i++) fRee(mindex->index_buffer[i]);
				fRee(scratch);
				fRee(offsetstr);
				fRee(mindex->index_buffer);
				fRee(mindex->buffer_length);
				fRee(mindex);
				fclose(infile);
				return(NULL);
			}
			
			mindex->index_origion = MENU_INDEX_FILE;
			mindex->bytes_in_menu = (ulong)atol(position);
			position = strchr(position, ' ');
			mindex->lines_in_menu = (ulong)atol(++position);

			/********************************************************
			 ********************************************************
			 * All was fine with index from file; chances of corruption are
			 * very low.  Now, return the MENU_INDEX pointer
			 ********************************************************
			 ********************************************************/
			fRee(scratch);
			fRee(offsetstr);
			fclose(infile);
			if(mindex){
				goto donemenuindexing; /* Clean up and return */            
			}

		} /* end of while(file index exists) */
		
		/*****************************************************************
		 * The file index was corrupt; Start over
		 *****************************************************************/
		if(mindex->index_corrupt){
			/* Reset and free the appropriate variables */
			
			for(i = 0; i < curbuff; i++) fRee(mindex->index_buffer[i]);
			fRee(mindex->index_buffer);
			fRee(mindex->buffer_length);
			mindex->check_address = (void *)mindex;
			mindex->index_corrupt = 1;
			mindex->correct_eol_len = 0;
			mindex->num_buffers = 0;
			mindex->num_sections = 0;
			mindex->data_path_one = NULL;
			mindex->data_path_two = NULL;
		} /* end of file index corruption cleanup */

		/* We read in first line of menu file to see if menu index existed;
		 * now reset file pointer to beginning of menu so that we can create
		 * one in memory alone. */
		if(fseek(infile, 0, SEEK_SET)) { /* error in fseek */
			fRee(scratch);
			fRee(mindex);
			fRee(offsetstr);
			MENU_ERR_PUSH(ROUTINE_NAME, ERR_GENERAL, "Can't seek beginning of menu file");
			return(NULL);
		}

	} /* end of if(!outfilename) */
     
    /***************************************************************************
	****************************************************************************
	*** At this point, one of the following is true:
	***
	*** 1) writing index out to file, regardless of previously existing index;
	*** 2) no index existed on file previously;
	*** 3) index existed in menu file, but was corrupt.
	****************************************************************************
	****************************************************************************/
	 
	
	/* set up a few variables */
	mindex->index_origion = MENU_INDEX_MEM;
	mindex->bytes_in_index = 0;
	mindex->lines_in_index = 0;

	/* allocate the buffers array, length array, and 1st buffer; initialize */

	num_allocated = 10;  /* start off with 10 buffers possible */
	
	if(!(mindex->index_buffer = (char **)mAlloc((size_t)(sizeof(char *) * num_allocated)))){
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating index buffer array");
		fRee(scratch);
		fRee(offsetstr);
		fclose(infile);
		fRee(mindex);
		if(outfilename) fclose(outfile);
		return(NULL);
	}

	if(!(mindex->buffer_length = (ulong *)mAlloc((size_t)(sizeof(ulong *) * num_allocated)))){
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating index buffer size array");
		fRee(scratch);
		fRee(offsetstr);
		fclose(infile);
		fRee(mindex);
		if(outfilename) fclose(outfile);
		return(NULL);
	}

	for(j = 0; j < num_allocated; j++) mindex->index_buffer[j] = NULL; /* initialize */

	curbuff = 0;
	if(!(mindex->index_buffer[curbuff] = (char *)mAlloc((size_t)mindex->max_buffer_size))){
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Creating menu index buffer");
		fRee(mindex->index_buffer);
		fRee(mindex->buffer_length);
		fRee(mindex);
		fRee(offsetstr);
		fRee(scratch);
		fclose(infile);
		if(outfilename) fclose(outfile);
		return(NULL);
	}
	
	offset = 0;
	numlines = 0;
	
	numread = 0;
	max_buf_size = mindex->max_buffer_size - 105;
	strcpy(mindex->index_buffer[curbuff], "#####");
	/* buffers array initialized */
	
	/* Check to see if we were given an output file name */
	if(outfilename){ 
		/* output 3 line header to menu index */
		
		strcpy(scratch, MENU_INDEX_SECTION);
		strcat(scratch, mindex->file_eol_str);
		fputs(scratch, outfile);
		indlen += strlen(scratch);
		numindlines++;
		
		strcpy(scratch, "                   *");
		/* the asterisk on the end is to prevent the menu stripping routines from stripping
		 * this line and changing the length of the menu */
		strcat(scratch, mindex->file_eol_str);
		fputs(scratch, outfile);
		indlen += strlen(scratch);
		numindlines++;
		
		sprintf(scratch, "%d", mindex->file_eollen);
		strcat(scratch, mindex->file_eol_str);
		fputs(scratch, outfile);
		indlen += strlen(scratch);
		numindlines++;
	}
	
	/* go through menu file, determining if new section */
	while(mn_binary_fgets(scratch, 490, infile, mindex->file_eol_str)) {
		if(scratch[0] == '*') { /* new section */
			/* Ignore MENU INDEX sections from corrupt file
			 * indexes */
			if(!strstr(scratch, MENU_INDEX_SECTION)){
				/* No, this section is not a MENU INDEX */
				
				for(i = strlen(scratch) - 1; i > 0; i--) /* trim trailing spaces */
					if(scratch[i] > ' ') break;
				i++;
				strcpy(scratch + i, mindex->file_eol_str);
			
				mindex->num_sections++;

				if(numread + strlen(scratch) > max_buf_size){ /* new buffer now */
					/* finish off old buffer */
					strcat(mindex->index_buffer[curbuff] + numread, "#* ");
					strcat(mindex->index_buffer[curbuff] + numread, mindex->file_eol_str);
				
					sprintf(offsetstr, "%ld %ld", offset, numlines);
					strcat(offsetstr, mindex->file_eol_str);
					strcat(mindex->index_buffer[curbuff] + numread, offsetstr);
					strcat(mindex->index_buffer[curbuff] + numread, "#####");
			
					mindex->buffer_length[curbuff] = strlen(mindex->index_buffer[curbuff]);
					
					curbuff++;
					if(curbuff == num_allocated){ /* need to make more buffer pointers */
						num_allocated += 10;  /* add another 10 buffers possible */
	
						if(!(mindex->index_buffer = (char **)reAlloc(mindex->index_buffer, (size_t)(sizeof(char *) * num_allocated)))){
							MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Reallocating index buffer array");
							fRee(mindex->buffer_length);
							fRee(scratch);
							fRee(offsetstr);
							fclose(infile);
							fRee(mindex);
							if(outfilename) fclose(outfile);
							return(NULL);
						}

						if(!(mindex->buffer_length = (ulong *)reAlloc(mindex->buffer_length, (size_t)(sizeof(ulong *) * num_allocated)))){
							MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating index buffer size array");
							for(i = 0; i < curbuff; i++) fRee(mindex->index_buffer[i]);
							fRee(scratch);
							fRee(offsetstr);
							fclose(infile);
							fRee(mindex);
							if(outfilename) fclose(outfile);
							return(NULL);
						}
					} /* end reAllocation of buffer pointers array */

					/* Allocate next buffer */
					if(!(mindex->index_buffer[curbuff] = (char *)mAlloc((size_t)mindex->max_buffer_size))){
						MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Creating menu index buffer");
						for(i = 0; i < curbuff; i++) fRee(mindex->index_buffer[i]);
						fRee(mindex->index_buffer);
						fRee(mindex->buffer_length);
						fRee(mindex);
						fRee(offsetstr);
						fRee(scratch);
						fclose(infile);
						if(outfilename) fclose(outfile);
						return(NULL);
					}
					numread = 0;
					strcpy(mindex->index_buffer[curbuff], "#####");
				} /* end creation of new buffer */

				/* Create MENU INDEX entry for this section */
				strcpy(offsetstr, "#");
				strcat(offsetstr, scratch);

				/* Put the entry onto the end of our memory buffer */
				strcat(mindex->index_buffer[curbuff] + numread, offsetstr);
				numread += strlen(offsetstr);

				/* If we are creating the index in a file, write out the entry
				 * now. */
				if(outfilename) 
					fputs(offsetstr, outfile);
					
				/* Adjust the index length variables */
				indlen += strlen(offsetstr);
				numindlines++;
				
				/* Create the second line of the entry */
				sprintf(offsetstr, "%ld %ld", offset, numlines);
				strcat(offsetstr, mindex->file_eol_str);
                
                /* Put it onto the end of our memory buffer */
				strcat(mindex->index_buffer[curbuff] + numread, offsetstr);
				numread += strlen(offsetstr);

				/* If we are creating the index in a file, write out the entry now. */
				if(outfilename) 
					fputs(offsetstr, outfile);
					
				/* Adjust the index length variables */	
				indlen += strlen(offsetstr);
				numindlines++;
				
				if(strstr(scratch, datasec)) { 
					/* is a data section */
				
					offset += strlen(scratch);
					numlines++;

					/* Retrieve the next line specifying the number of bytes
					 * in the section */				
					if(!mn_binary_fgets(offsetstr, 490, infile, mindex->file_eol_str)) {
						sprintf(offsetstr, "%s: Data section corrupt.\n", scratch);
						MENU_ERR_PUSH(ROUTINE_NAME, ERR_MN_FILE_CORRUPT, offsetstr);
						for(i = 0; i < curbuff; i++) fRee(mindex->index_buffer[i]);
						fRee(mindex->index_buffer);
						fRee(mindex->buffer_length);
						fRee(mindex);
						fRee(offsetstr);
						fRee(scratch);
						fclose(infile);
						if(outfilename) fclose(outfile);
						return(NULL);
					}
					
					seclen = atol(offsetstr);
					
					offset += strlen(offsetstr);
					numlines++;
					
					/* Seek past the data section */
					fseek(infile, seclen, SEEK_CUR);
					offset += seclen;
					numlines++;
					
					/* Go back to the top of the loop */
					continue;
				} /* end of if(data section) */
				
			} /* end of if(new section != MENU INDEX) */
			
		} /* end of new section handling */

		offset += strlen(scratch);
		numlines++;
	}
	
	strcpy(scratch, "END_MENU_INDEX");
	strcat(scratch, mindex->file_eol_str);
	if(outfilename) fputs(scratch, outfile);
	indlen += strlen(scratch);
	numindlines++;
	
	/* finish off index buffer */
	strcat(mindex->index_buffer[curbuff] + numread, "#* ");
	strcat(mindex->index_buffer[curbuff] + numread, mindex->file_eol_str);
				
	sprintf(offsetstr, "%ld %ld", offset, numlines);
	strcat(offsetstr, mindex->file_eol_str);
	strcat(mindex->index_buffer[curbuff] + numread, offsetstr);
	strcat(mindex->index_buffer[curbuff] + numread, "#####");
			
	mindex->buffer_length[curbuff] = strlen(mindex->index_buffer[curbuff]);
	
	/* reAllocate current buffer to minimum possible size */
	if(!(mindex->index_buffer[curbuff] = (char *)reAlloc(mindex->index_buffer[curbuff], (size_t)(mindex->buffer_length[curbuff] + 5)))){
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_GENERAL, "Reallocating index buffer");
		for(i = 0; i <= curbuff; i++) fRee(mindex->index_buffer[i]);
		fRee(scratch);
		fRee(offsetstr);
		fRee(mindex->index_buffer);
		fRee(mindex->buffer_length);
		fRee(mindex);
		fclose(infile);
		if(outfilename) fclose(outfile);
		return(NULL);
	}
	mindex->num_buffers = curbuff + 1;

	mindex->bytes_in_menu = offset;
	mindex->lines_in_menu = numlines;

	if(outfilename) {
		/* We are creating a file index on the front of outputfile-
		 * finish it off now */
		 
		sprintf(offsetstr, "TOTALS: %ld %ld", offset, numlines);
		strcat(offsetstr, mindex->file_eol_str);
		fputs(offsetstr, outfile);
		indlen += strlen(offsetstr);
		numindlines++;                         

		/* done with first pass, go in for second... */
		fclose(infile);
		
		/* Copy everything from the origional file to the output file */
		infile = fopen(filename, MENU_FOPEN_R);
		
		while(mn_binary_fgets(scratch, 490, infile, mindex->file_eol_str)) {
			if(scratch[0] == '*') {
				/* A new section was encountered */

				if(strstr(scratch, datasec)) { 
					/* is a data section */
				
					fputs(scratch, outfile); /* Write out the title */
				
					if(!mn_binary_fgets(offsetstr, 490, infile, mindex->file_eol_str)) {
						/* Corrupt data section */
						
						sprintf(offsetstr, "%s: Data section corrupt.\n", scratch);
						MENU_ERR_PUSH(ROUTINE_NAME, ERR_MN_FILE_CORRUPT, offsetstr);
						return(0);
					}
				
					seclen = atol(offsetstr);
				
					fputs(offsetstr, outfile); /* Write out the section length */
				
					/* Copy the data section over to the new file in chunks */
					for(; seclen > 490; seclen -= 490) {
						if((fread(offsetstr, 490, 1, infile)) < 490) {
							sprintf(offsetstr, "%s: Data section corrupt.\n", scratch);
							MENU_ERR_PUSH(ROUTINE_NAME, ERR_MN_FILE_CORRUPT, offsetstr);
							return(0);
						}
						
						fwrite(offsetstr, 490, 1, outfile);
					}
				
					if(seclen) {
						/* Finish copying it over */
						if((fread(offsetstr, (size_t)seclen, 1, infile)) < 490) {
							sprintf(offsetstr, "%s: Data section corrupt.\n", scratch);
							MENU_ERR_PUSH(ROUTINE_NAME, ERR_MN_FILE_CORRUPT, offsetstr);
							return(0);
						}
					
						fwrite(offsetstr, (size_t)seclen, 1, outfile);
					}
					
					/* We don't want to put out the section name again- 
					 * continue with loop */
					continue;				
				} /* End of (new section was a data section */
			} /* End of (new section encountered) */
			
			fputs(scratch, outfile);
		}
			
		fclose(outfile);
		/* Done copying */
		
		/* Fill in the necessary menu index byte length and line length */
		if(!(outfile = fopen(outfilename, MENU_FOPEN_U))) {
			sprintf(scratch, "Can't open '%s' for update.\n", outfilename);
			MENU_ERR_PUSH(ROUTINE_NAME, ERR_GENERAL, scratch);
			for(i = 0; i <= curbuff; i++) fRee(mindex->index_buffer[i]);
			fRee(scratch);
			fRee(offsetstr);
			fRee(mindex->index_buffer);
			fRee(mindex->buffer_length);
			fRee(mindex);
			fclose(infile);
			if(outfilename) fclose(outfile);
			return(NULL);
		}

		/* Seek to the second line in the file (right after *MENU INDEX) */
		if(fseek(outfile, 11 + mindex->file_eollen, SEEK_SET)){ 
			MENU_ERR_PUSH(ROUTINE_NAME, ERR_GENERAL, "Cannot seek location in file");
			for(i = 0; i <= curbuff; i++) fRee(mindex->index_buffer[i]);
			fRee(scratch);
			fRee(offsetstr);
			fRee(mindex->index_buffer);
			fRee(mindex->buffer_length);
			fRee(mindex);
			fclose(infile);
			if(outfilename) fclose(outfile);
			return(NULL);
		}
		
		/* Put out the byte length and line length */
		sprintf(scratch, "%ld %ld", indlen, numindlines);
		fputs(scratch, outfile);
		fclose(outfile);
		mindex->file_index_exists = 1;
	} /* end of if(outfilename) */
	
	/* The index was successfully created; return it */
	fclose(infile);
	fRee(scratch);
	fRee(offsetstr);
	
donemenuindexing:
	position = NULL;
	
	if(!mn_index_get_offset(mindex, "MENU_NAME", &rowsize))
		position = MENU_UNTITLED;
	else
		if(!mn_index_get_offset(mindex, "CD_MENU_NAME", &rowsize))
			position = MENU_UNTITLED;
			
	if(position){
		/* Found "MENU_NAME" section */
		if(mn_section_get(mindex, NULL, &rowsize, &scratch)){
			MENU_ERR_PUSH(ROUTINE_NAME, ERR_GENERAL, "Reading in MENU_NAME section");
			return(NULL);
		}
		if((position = strstr(scratch, mindex->file_eol_str)))
			position[0] = '\0';
		if(!(mindex->menu_name = (char *)mAlloc((size_t)(strlen(scratch) + 4)))){
			MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating menu name");
			fRee(scratch);
			return(NULL);
		}
		strcpy(mindex->menu_name, scratch);
		fRee(scratch);
	}
	else{
		if(!(mindex->menu_name = (char *)mAlloc((size_t)(strlen(MENU_UNTITLED) + 5)))){
			MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating menu name");
			return(NULL);
		}
		strcpy(mindex->menu_name, MENU_UNTITLED);
	}

	position = NULL;

	if(!mn_index_get_offset(mindex, MENU_TEXT_CONTENT_SEC, &rowsize)){
		/* Found "TEXT_CONTENT" section */
		if(mn_section_get(mindex, NULL, &rowsize, &scratch)){
			MENU_ERR_PUSH(ROUTINE_NAME, ERR_GENERAL, "Reading in TEXT_CONTENT section");
			return(NULL);
		}

		if((position = strstr(scratch, mindex->file_eol_str)))
			position[0] = '\0';
		
		for(i = strlen(scratch) - 1; (i >= 0) && (scratch[i] < 33); i--)
			scratch[i] = '\0';
			
		position = scratch;
		while((position[0]) && (position[0] < 33))
			position++;
			
		if(position[0]) {
			/* The TEXT_CONTENT section had a valid entry */
		
			if(!(mindex->text_content = (char *)mAlloc((size_t)(strlen(position) + 4)))){
				MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating menu text content");
				fRee(scratch);
				return(NULL);
			}
			strcpy(mindex->text_content, position);
			fRee(scratch);
		}
	}
	
	if(!(mindex->text_content)) {
		/* We didn't find a valid entry for TEXT_CONTENT- default value */
		
		if(!(mindex->text_content = (char *)mAlloc((size_t)(strlen(MENU_TEXT_CONTENT) + 5)))){
			MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating menu text content");
			return(NULL);
		}
		strcpy(mindex->text_content, MENU_TEXT_CONTENT);
	}

	return(mindex);
}


/*
 * NAME:        mn_index_get_offset
 *              
 * PURPOSE:     to determine the offset and size (in bytes) of a section 
 *              in a menu file previously indexed by the mn_index_make function
 *
 * USAGE:   int mn_index_get_offset(MENU_INDEX_PTR index_to_search, 
 *             char *section_title_to_find, ROW_SIZES_PTR rowsize)
 *
 * RETURNS:     0 if all is OK,
 *              >0 on error.  ERR_MN_SEC_NFOUND is the only error this function
 *              will return, and it WILL NOT push an error onto the stack.
 *              In the rowsize structure, the offset and size are stored.
 *
 * DESCRIPTION: Uses the MENU_INDEX structure to compute the offset and size
 *              of the requested section.  Returns ERR_MN_SEC_NFOUND if the
 *      requested section could not be found.
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none- completely portable
 *
 * AUTHOR:      Kevin Frender kbf@ngdc.noaa.gov
 *
 * COMMENTS: Requires a MENU_INDEX structure.  The rowsize structure MUST
 *           be allocated before the function is called.
 *           As with all mn_index routines, this function assumes that no
 *           section name will be greater than 490 characters in length.
 *          
 * KEYWORDS: menu index
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "mn_index_get_offset"
int mn_index_get_offset(MENU_INDEX_PTR mindex, char *section, ROW_SIZES_PTR rowsize)
{
	char scratch[500];
	char *position = NULL;
	int i = 0;
	ulong offset_bytes;
	ulong offset_lines;
	ulong offset;
	
	assert((mindex) && (mindex->check_address == (void *)mindex) && (section) && (rowsize));

	/* check to make sure that the section name request is complete
	 * (with "#*" at the front and an EOL at the end) */
	scratch[0] = '\0';

	if(section[i] != '#') 
		strcat(scratch, "#");
	else 
		i++;

	if(section[i] != '*') strcat(scratch, "*");
	strcat(scratch, section);

	for(i = strlen(scratch) - 1; i > 0; i--) /* trim trailing spaces */
		if(scratch[i] > ' ') break;
	i++;
	strcpy(scratch + i, mindex->file_eol_str);

	for(i = 0; i < mindex->num_buffers; i++){ /* loop through buffers */
		if(position = MN_STRNSTR(scratch, mindex->index_buffer[i], 
				(size_t)mindex->buffer_length[i])){ /* found section */
			position = strchr(position, mindex->file_eol_str[mindex->file_eollen - 1]);
			offset_bytes = (ulong)atol(++position);
			position = strchr(position, ' ');
			offset_lines = (ulong)atol(++position);

			if(mindex->correct_eol_len) /* need to correct for EOL length */
				offset = offset_bytes - (mindex->index_eol_len - mindex->file_eollen) * offset_lines;
			else 
				offset = offset_bytes;

			if(mindex->index_origion == MENU_INDEX_FILE)
				/* need to correct for menu index length */
				offset += mindex->bytes_in_index;

			rowsize->start = offset; /* set offset */
			/* now we have the offset of the section; get the size */

			position = strchr(position, '#'); /* go to next line */
			position = strchr(position, mindex->file_eol_str[mindex->file_eollen - 1]);
			offset_bytes = (ulong)atol(++position);
			position = strchr(position, ' ');
			offset_lines = (ulong)atol(++position);

			if(mindex->correct_eol_len) /* need to correct for EOL length */
				offset = offset_bytes - (mindex->index_eol_len - mindex->file_eollen) * offset_lines;
			else 
				offset = offset_bytes;

			if(mindex->index_origion == MENU_INDEX_FILE)
				/* need to correct for menu index length */
				offset += mindex->bytes_in_index;

			rowsize->num_bytes = offset - rowsize->start; /* set size */
			return(0); /* all is OK */
		}
	} /* end loop through buffers */
	
	/* no section matching the request was found */
	rowsize->start = 0;
	rowsize->num_bytes = 0;
	return(ERR_MN_SEC_NFOUND);
}

/*
 * NAME:        mn_index_set_paths
 *              
 * PURPOSE:     to set the paths in a MENU_INDEX structure where help is searched for
 *
 * USAGE:      int mn_index_set_paths(MENU_INDEX_PTR mindex, char *pathone, char *pathtwo)
 *              pathone MUST NOT be NULL;
 *              pathtwo may be NULL.
 *
 * RETURNS:     0 if all is OK, error code on error
 *
 * DESCRIPTION: Copies pathone into mindex->data_path_one and (if pathtwo is not NULL)
 *              copies pathtwo into mindex->data_path_two.  If the index paths
 *              have been previously set, they will be freed.
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none- completely portable
 *
 * AUTHOR:      Kevin Frender kbf@ngdc.noaa.gov
 *
 * COMMENTS:
 *          
 * KEYWORDS: menu index
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "mn_index_set_paths"
int mn_index_set_paths(MENU_INDEX_PTR mindex, char *pathone, char *pathtwo)
{
	assert((mindex) && (mindex->check_address == (void *)mindex));
	
	if(!pathone){
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_PTR_DEF, "Path one");
		return(ERR_PTR_DEF);
	}

	/* Free the old paths */    
	if(mindex->data_path_one)
		fRee(mindex->data_path_one);
	mindex->data_path_one = NULL;
	if(mindex->data_path_two)
		fRee(mindex->data_path_two);
	mindex->data_path_two = NULL;
	
	if(!(mindex->data_path_one = (char *)mAlloc((size_t)(strlen(pathone) + 3)))){
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating path");
		return(ERR_MEM_LACK);
	}
	strcpy(mindex->data_path_one, pathone);
	
	if(!pathtwo)
		return(0);

	if(!(mindex->data_path_two = (char *)mAlloc((size_t)(strlen(pathtwo) + 3)))){
		MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating path");
		return(ERR_MEM_LACK);
	}
	strcpy(mindex->data_path_two, pathtwo);
		
	return(0);
}

/*
 * NAME:        mn_index_free
 *              
 * PURPOSE:     to free a MENU_INDEX structure
 *
 * USAGE:      int mn_index_free(MENU_INDEX_PTR mindex)
 *
 * RETURNS:     0 if all is OK,
 *              1 on error.
 *
 * DESCRIPTION: frees the MENU_INDEX structure and all sub-elements
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none- completely portable
 *
 * AUTHOR:      Kevin Frender kbf@ngdc.noaa.gov
 *
 * COMMENTS:
 *          
 * KEYWORDS: menu index
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "mn_index_free"
int mn_index_free(MENU_INDEX_PTR mindex)
{
	int i;
	
	assert((mindex) && (mindex->check_address == (void *)mindex));
	
	if(mindex->check_address != (void *)mindex)
		return(1);

	if(mindex->menu_file)
		fRee(mindex->menu_file);
	if(mindex->buffer_length)
		fRee(mindex->buffer_length);
	if(mindex->index_buffer){
		for(i = 0; i < mindex->num_buffers; i++)
			if(mindex->index_buffer[i])
				fRee(mindex->index_buffer[i]);
		fRee(mindex->index_buffer);
	}
	if(mindex->file_eol_str)
		fRee(mindex->file_eol_str);
	if(mindex->data_path_one)
		fRee(mindex->data_path_one);
	if(mindex->data_path_two)
		fRee(mindex->data_path_two);

	if (mindex->menu_name)
		fRee(mindex->menu_name);

	if (mindex->text_content)
		fRee(mindex->text_content);

	fRee(mindex);
	return(0);
}

/*
 * NAME:        mn_get_file_eol_str
 *              
 * PURPOSE:     to determine the EOL sequence for a given file
 *
 * USAGE:      char *mn_get_file_eol_str(char *filename)
 *
 * RETURNS:     NULL on error, or a pointer to an allocated string containing
 *              the EOL sequence for the file.
 *
 * DESCRIPTION: Determines the EOL sequence for filename.  Returns a pointer
 *              to an allocated buffer containing the EOL sequence.
 *
 *				This function preforms rudimentary checking for corrupt EOL
 *				sequences.
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none- completely portable
 *
 * AUTHOR:      Kevin Frender kbf@ngdc.noaa.gov
 *
 * COMMENTS:	This function has sort-of kind-of the same functionality as
 *				ff_get_buffer_eol_str, but not quite.  The ff_get_buffer_eol_str
 *				function operates on a buffer; this function operates on a file.
 *				This function opens the file given, and returns an allocated
 *				buffer containing its EOL string.  ff_get_buffer_eol_str does not.
 *				This function is used by mn_ functions, which are not reliant on
 *				any freeform functions.  Several applications are built using only
 *				the menu library and not the freeform library; placing a call in
 *				this function to an ff_ function would ruin this independancy.
 *          
 * KEYWORDS: menu index
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "mn_get_file_eol_str"
char *mn_get_file_eol_str(char *filename)
{
	char *file_eol_str;
	FILE *infile;
	int c;
	
	if(!(infile = fopen(filename, "rb"))){
		return(NULL);
	}
	if(!(file_eol_str = (char *)mAlloc((size_t)3))){
	    fclose(infile); 	// jhrg 3/18/11
		return(NULL);
	}
	
	while((c = getc(infile)) != EOF){
		if(c == MENU_EOL_LF){ /* LF */
			/* Must be a unix file */
			file_eol_str[0] = (char)MENU_EOL_LF;
			file_eol_str[1] = '\0';
			
			/* Check for EOL corruption */
			while((c = getc(infile)) != EOF){
				if(c == MENU_EOL_CR) {
					/* Corrupt EOL encountered */
					fRee(file_eol_str);
					fclose(infile);
					return(NULL);
				}
				if(c != MENU_EOL_LF)
					break;
			}
			
			fclose(infile);
			return(file_eol_str);
		} /* End checking for unix EOL */
		
		if(c == MENU_EOL_CR){ /* CR */
			c = getc(infile);
			if(c == MENU_EOL_LF){ /* CR-LF */
				/* Must be a DOS file */
				file_eol_str[0] = (char)MENU_EOL_CR;
				file_eol_str[1] = (char)MENU_EOL_LF;
				file_eol_str[2] = '\0';
				
				/* Check for EOL corruption */
				while((c = getc(infile)) != EOF){
					if(c == MENU_EOL_LF) {
						/* Corrupt EOL encountered */
						fRee(file_eol_str);
						fclose(infile);
						return(NULL);
					}
					
					if(c == MENU_EOL_CR){
						c = getc(infile);
						if(c != MENU_EOL_LF) {
							/* Corrupt EOL encountered */
							fRee(file_eol_str);
							fclose(infile);
							return(NULL);
						}
					}
					else
						break;
				}
				
				fclose(infile);
				return(file_eol_str);                   
			}
			/* Must be a MAC file */
			file_eol_str[0] = (char)MENU_EOL_CR;
			file_eol_str[1] = '\0';
			
			/* Check for EOL corruption */
			while((c = getc(infile)) != EOF){
				if(c == MENU_EOL_LF) {
					/* Corrupt EOL encountered */
					fRee(file_eol_str);
					fclose(infile);
					return(NULL);
				}
				if(c != MENU_EOL_CR)
					break;
			}
			
			fclose(infile);
			return(file_eol_str);                   
		}
	}
	
	/* Couldn't find any EOL chars */
	fRee(file_eol_str);
	fclose(infile);
	return(NULL);
}

/*
 * NAME:        mn_binary_fgets
 *              
 * PURPOSE:     To read a single line from a text file OPENED IN BINARY MODE.
 *
 * USAGE:      char *mn_binary_fgets(char *string, int n, FILE *stream,
 *                   char *file_eol_str)
 *
 * RETURNS:     NULL on error, string if all is OK
 *
 * DESCRIPTION: Reads a line from stream until:
 *              -the EOL sequence stored in file_eol_str is hit;
 *              -n characters have been read;
 *              -an EOF has been encountered.
 *
 *              This function assumes that the EOL sequences will be uniform
 *              throughout the file.  The stream MUST BE OPENED IN BINARY MODE.
 *              This function (like fgets) will retain the EOL
 *              sequence on the end of the string.
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none- completely portable
 *
 * AUTHOR:      Kevin Frender kbf@ngdc.noaa.gov
 *
 * COMMENTS:    stream MUST BE OPENED IN BINARY MODE.
 *          
 * KEYWORDS: menu index
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "mn_binary_fgets"
char *mn_binary_fgets(char *string, int n, FILE *stream, char *file_eol_str)
{
	int c;
	int d;
	int numc = 0;
	
	d = (int)file_eol_str[0];
	
	while(numc < n){
		c = getc(stream);
		if(c == EOF){
			if(numc)
				return(string);
			else
				return(NULL);
		}

		string[numc++] = (char)c;
		
		if(c == d){
			if(file_eol_str[1]){
				c = getc(stream);
				if(c != (int)file_eol_str[1]){
					/* ERROR- file has inconsistent EOL sequences */
					return(NULL);
				}
				string[numc++] = (char)c;
			}
			string[numc++] = '\0';
			return(string);
		}
	}
}

/*
 * NAME:        mn_index_find_title
 *              
 * PURPOSE:     to find all titles matching a given postfix
 *
 * USAGE:       int mn_index_find_title(MENU_INDEX_PTR index_to_search, 
 *                  char *postfix_to_find, ROW_SIZES_PTR rowsize, char **buffer)
 *
 * RETURNS:     0 if all is OK, >0 on error
 *
 * DESCRIPTION: Uses the MENU_INDEX structure to find all occurances of a title
 *              in the menu file ending in postfix_to_find.  Each call to this 
 *              function returns one title.  The rowsize structure is used as a
 *              state holder only; the information in it is meaningless outside
 *              this function.  To start searching the menu file for a postfix,
 *              set rowsize->num_bytes = 0 and call.  When no more titles are 
 *              found, returns ERR_MN_SEC_NFOUND (But no error is pushed onto
 *              the stack).  The section title found is stored in the dereferenced
 *              buffer pointer; if the dereferenced buffer pointer is NULL, a
 *              string of 500 bytes is allocated for it.
 *
 * SYSTEM DEPENDENT FUNCTIONS:  none- completely portable
 *
 * AUTHOR:      Kevin Frender kbf@ngdc.noaa.gov
 *
 * COMMENTS: Requires a MENU_INDEX structure.  
 *           As with all mn_index routines, this function assumes that no
 *           section name will be greater than 490 characters in length.
 *
 *           If given a numeric prefix, no garuntee can be made that a bogus
 *           title will not be returned
 *
 *           Rowsize structure must be allocated outside of this function-
 *           important that the rowsize->num_bytes get set to 0 to start.
 *
 * KEYWORDS: menu index
 *
 */
#undef FUNCT
#define FUNCT "mn_index_find_title"
int mn_index_find_title(MENU_INDEX_PTR mindex, char *postfix, ROW_SIZES_PTR rowsize, char **buffer)
{
	char *position = NULL;
	char *buff;
	int i = 0;
	
	assert((mindex) && (mindex->check_address == (void *)mindex) && (postfix) && (rowsize));
	
	buff = *(buffer);
	if(!buff){
		if(!(buff = (char *)mAlloc((size_t)500))){
			MENU_ERR_PUSH(ROUTINE_NAME, ERR_MEM_LACK, "Allocating buffer");
			return(ERR_MEM_LACK);
		}
		*(buffer) = buff;
	}
	
	/* check to make sure that the section name request is complete
	 * (with an EOL at the end) */
	strcpy(buff, postfix);
	for(i = strlen(buff) - 1; i > 0; i--) /* trim trailing spaces */
		if(buff[i] > ' ') break;
	i++;
	strcpy(buff + i, mindex->file_eol_str);
	
	if(rowsize->num_bytes == 0){
		rowsize->start = 0;
	}
	
	for(i = rowsize->start; i < mindex->num_buffers; i++){ /* loop through buffers */
		if(position = MN_STRNSTR(buff, (mindex->index_buffer[i] + rowsize->num_bytes), 
				(size_t)(mindex->buffer_length[i] - rowsize->num_bytes))){ /* found section */
			/* Store away current position */
			rowsize->start = i;
			rowsize->num_bytes = (long)((position + strlen(postfix)) - (mindex->index_buffer[i]));
				
			while(position[-1] != '*')
				position--;
				
			for(i = 0; position[i] != mindex->file_eol_str[0]; i++){
				buff[i] = position[i];
			}
			buff[i] = '\0';
			return(0);
		}               
	} /* end loop through buffers */
	
	/* no section matching the request was found */
	rowsize->start = 0;
	rowsize->num_bytes = 0;
	return(ERR_MN_SEC_NFOUND);
}

#ifdef MEMTRAP
void *mn_malloc(size_t memsize, int linenum, char *routine)
{
	FILE *infile;
	void *vptr;
	
	if(!(infile = fopen(MEMTRAPFILE, "a")))
		return(NULL);
	vptr = malloc(memsize);
	fprintf(infile, "malloc  ptr=%p size=%d line %d, %s\n", vptr, (int)memsize, linenum, routine);
	fclose(infile);
	return(vptr);
}

void *mn_realloc(void *memblk, size_t memsize, int linenum, char *routine)
{
	FILE *infile;
	void *vptr;
	
	if(!(infile = fopen(MEMTRAPFILE, "a")))
		return(NULL);
	vptr = realloc(memblk, memsize);
	fprintf(infile, "realloc ptr=%p size=%d line %d, %s (%p)\n", vptr, (int)memsize, linenum, routine, memblk);
	fclose(infile);
	return(vptr);
}

void mn_free(void *memblk, int linenum, char *routine)
{
	FILE *infile;
	
	if(!(infile = fopen(MEMTRAPFILE, "a")))
		return;
	fprintf(infile, "free    ptr=%p line %d, %s\n", memblk, linenum, routine);
	free(memblk);
	fclose(infile);
	return;
}
#endif

#ifdef NO_FF

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

#undef ROUTINE_NAME
#define ROUTINE_NAME "Strnstr"

#define AlphabetSize 256

/*
 * NAME:	mn_strnstr
 *		
 * PURPOSE:
 *			-- see above --
 * AUTHOR:
 *
 * USAGE:	mn_strnstr(
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

char *mn_strnstr(char *pcPattern, char *pcText, size_t uTextLen)
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
         mAlloc(2 * (sizeof(unsigned) * (uPatLen + 1)));

    if (!upMatchJump) {
		MENU_ERR_PUSH(ROUTINE_NAME,ERR_MEM_LACK,"upMatchJump");
		return(NULL);
    }

	upBackUp = upMatchJump + uPatLen + 1;

    /* Heuristic #1 -- simple char mis-match jumps ... */
    memset((void *)uCharJump, 0, AlphabetSize*sizeof(unsigned));
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
    fRee(upMatchJump);
    if (uPat == 0)
        return(pcText + (uText + 1)); /* have a match */
    else
        return (NULL); /* no match */
}

#endif
