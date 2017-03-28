/*
 * FILENAME: memtrack.c
 * MEM.H -- ** Copyright (c) 1990, Cornerstone Systems Group, Inc. 
 * Dr. Dobbs Journal, August 1990, p. 180
 *
 * Modified significantly by Mark A. Ohrenschall, mao@ngdc.noaa.gov
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

#define MEM_OPEN_FILE 1
#define MEM_PTR_DEF 2
#define MEM_GENERAL 3

#define ERR_H__ /* force not including err.h */
#include <freeform.h>

#if FF_CC == FF_CC_MSVC1 || FF_CC == FF_CC_MSVC4
#include <io.h>
#endif

#if FF_CC == FF_CC_MSVC1
#define HUGE huge
#define FAR __far
#else
#define HUGE
#define FAR
#endif

#ifdef SUNCC /* Let this break */
#include <unistd.h>
#endif

#include <string.h>

#ifdef FIND_UNWRAPPED_MEM
#undef FIND_UNWRAPPED_MEM
#endif
#include <memtrack.h>

#define FF_DIE {abort();}

#if FF_OS == FF_OS_DOS

typedef long POST_TYPE_t;

#define PREPOST  "\xFE\xED\xDE\xAD"
#define POSTPOST "\xDE\xAD\xBE\xEF"

#else

typedef double POST_TYPE_t;

#define PREPOST  "\xFE\xED\xDE\xAD\xFE\xED\xDE\xAD"
#define POSTPOST "\xDE\xAD\xBE\xEF\xDE\xAD\xBE\xEF"

#endif

#define ONE_POST_SIZE sizeof(POST_TYPE_t)
#define BOTH_POSTS_SIZE (2 * ONE_POST_SIZE)

/* the next functions are only called by the other mem functions */

static void set_posts(FF_MEM_ENTRY_PTR);
static int check_posts(FF_MEM_ENTRY_PTR);

static int MEMTrack_fp(FF_MEM_LOG_PTR);

static int heap_check(FF_MEM_LOG_PTR);
static int leak_test(FF_MEM_LOG_PTR);

static void MEMTrack_alloc(FF_MEM_LOG_PTR);
static int MEMTrack_free(FF_MEM_LOG_PTR);

#define  ALLOC          'A'
#define  FREE           'F'
#define  MESSAGE_LETTER 'M'
#define  FILL_CHAR      '\xCC'
#define  MAX_LTH        255

#ifndef BOOLEAN
#ifdef TRUE
#undef TRUE
#endif
#ifdef FALSE
#undef FALSE
#endif
#define BOOLEAN unsigned char
#define TRUE 1
#define FALSE 0
#endif /* BOOLEAN */

#ifndef MEMTRACK_B
/*
 * NAME:    MEMTrack_msg(char *msg)
 *      
 * PURPOSE: Write a message to the debugging file.
 *
 * USAGE:   static int MEMTrack_msg(char *msg, char *routine_name, char *cfile_name, int line_number)
 *
 * RETURNS: non-zero if an error or failure to write to file
 *
 * DESCRIPTION: This function is called by the memory functions
 *              which are active if MEMTRACK is defined.
 *              It writes msg to the memory tracking log file
 * Will write to stderr if fseek() on file fails
 *              
 * AUTHOR:  Adapted from an article in the August 1990 issue of Dr. Dobbs
 *          Journal (Copyright (c) 1990, Cornerstone Systems Group, Inc.)
 *          by T. Habermann, NGDC, (303) 497 - 6472, haber@ngdc.noaa.gov
 *
 * COMMENTS:This function is only called if MEMTRACK is defined.  If environmental
 * variable MEMTRACK is not defined then no action is taken.
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * KEYWORDS:
 *
 */

static int MEMTrack_msg(char *msg, char *routine_name, char *cfile_name, int line_number)
{
	int error;

	FF_MEM_ENTRY entry;
	FF_MEM_LOG   log;

	entry.addr = 0;
	entry.size = 0;

	entry.tag = msg;
	entry.routine = routine_name;
	entry.file = cfile_name;
	entry.line = line_number;

	log.entry = &entry;
	error = MEMTrack_fp(&log);
	if (error == MEM_OPEN_FILE)
	{
		fprintf(stderr,"%c %s %s %d %s\n", MESSAGE_LETTER, routine_name, cfile_name, line_number, msg);
		fflush(stderr);
		return(error);
	}
	else if (error)
		return(error);
		
	if (fseek(log.file,0L,SEEK_END) != 0)
	{
		fprintf(stderr, "Cannot perform fseek operation on memory log file\n");
		fprintf(stderr, "Called from %s %s %d (%s)\n", routine_name, os_path_return_name(cfile_name), line_number, msg);
		exit(EXIT_FAILURE);
	}

	if (fprintf(log.file,"%c %s %s %d %s\n", MESSAGE_LETTER, routine_name, os_path_return_name(cfile_name), line_number, msg) <= 0)
	{
		fclose(log.file);
		return(1);
	}

	fclose(log.file);
	return(0);
}
#endif

#ifdef MEMTRACK_B
void MEMTrace(void)
#else
void MEMTrace(char *msg, char *routine_name, char *cfile_name, int line_number)
#endif
{
	FF_MEM_ENTRY entry;
	FF_MEM_LOG   log;

	entry.addr = 0;
	entry.size = 0;

#ifndef MEMTRACK_B
	entry.tag = msg;
	entry.routine = routine_name;
	entry.file = cfile_name;
	entry.line = line_number;
#endif

	log.entry = &entry;

	heap_check(&log);
#ifndef MEMTRACK_B
	MEMTrack_msg(msg, routine_name, cfile_name, line_number);
#endif
}

static void MEMTrack_append_entry
	(
	 FF_MEM_LOG_PTR log
	)
{
#ifdef MEMTRACK_B
	size_t num_written;
#else
	int num_written;
#endif

	fseek(log->file,0L,SEEK_END);
#ifdef MEMTRACK_B
	num_written = fwrite(log->entry, sizeof(*log->entry), 1, log->file);
	if (num_written != 1)
		FF_DIE
#else
	num_written = fprintf(log->file,"%c %lx %lu %s %s %d %s\n", (char)log->entry->state, (long)(char HUGE *)log->entry->addr, (unsigned long)log->entry->size, log->entry->routine, os_path_return_name(log->entry->file), (int)log->entry->line, log->entry->tag);
	if (num_written <= 0)
		FF_DIE
#endif
	fclose(log->file);
}

/*
 * NAME:    MEMTrack_alloc
 *      
 * PURPOSE: Track an allocation. Write it in the memory log file
 *          in the format A memlocation bytesallocated tag, where memlocation is a number
 *          in %lx format (long int, hex notation)
 *
 * USAGE:   void MEMTrack_alloc(void *allocated, char *tag, char *routine_name, char *cfile_name, int line_number)
 *
 * RETURNS: None
 *
 * DESCRIPTION: This function is called by the memory allocation functions
 *              which are active if MEMTRACK is defined.
 *              MEMTrack_alloc records the allocation of memory in the
 *              memory tracking log file. The name of this file is given by the
 *              environment variable MEMTRACK.
 *              
 * AUTHOR:  Adapted from an article in the August 1990 issue of Dr. Dobbs
 *          Journal (Copyright (c) 1990, Cornerstone Systems Group, Inc.)
 *          by T. Habermann, NGDC, (303) 497 - 6472, haber@ngdc.noaa.gov
 *
 * COMMENTS:This function is only called if MEMTRACK is defined.  If environmental
 * variable MEMTRACK is not defined then memory log tracking is disabled.
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * KEYWORDS:
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "MEMTrack_alloc" 

static void MEMTrack_alloc(FF_MEM_LOG_PTR log)
{
	int error;

	error = MEMTrack_fp(log);
	if (!error && log->file)
	{
		log->entry->state = ALLOC;
		MEMTrack_append_entry(log);
	}
	else if (!log->file)
	{/* MEMTRACK is not defined */
	}
}

static BOOLEAN MEMTrack_find_entry(FF_MEM_LOG_PTR log)
{
#ifndef MEMTRACK_B
	char  line[MAX_LTH];
#endif
	BOOLEAN found = FALSE;

	FF_MEM_ENTRY file_entry;

	file_entry.state = log->entry->state;

#ifdef MEMTRACK_B
	for (log->pos = 0; fread(&file_entry, sizeof(file_entry), 1, log->file); log->pos += sizeof(file_entry))
#else
	for (log->pos=0L; fgets(line,sizeof(line),log->file); log->pos += strlen(line))
#endif
	{
#ifndef MEMTRACK_B
		if (line[0] != log->entry->state)
			continue;

		sscanf(line,"%*c %lx %lu", &file_entry.addr, &file_entry.size);
#endif
		/* Is addr in file the one we want?  */
		if ((file_entry.state == log->entry->state) && ((char HUGE *)file_entry.addr - (char HUGE *)log->entry->addr == 0))
		{
			found = TRUE;
			break;
		}
	}

	return(found);
}

/*
 * NAME:    MEMTrack_free
 *      
 * PURPOSE: Track and perform a memory free. Check the memory tracking log
 * file to make sure memory being freed was allocated.
 *
 * USAGE:   void MEMTrack_free(void *to_free, char *tag, char *routine_name, char *cfile_name, int line_number)
 *
 * RETURNS: Zero on success, or MEM_OPEN_FILE if MEMTRACK_LOG cannot be opened
 * or MEM_PTR_DEF if address cannot be found.
 *
 * DESCRIPTION: This function is called by the memory functions
 *              which are active if MEMTRACK is defined.
 *              MEMTrack_free records the freeing of memory in the
 *              memory log file. The name of this file is given by the
 *              environment variable MEMTRACK.
 *
 * Returns the allocation size of the block to be free()'d as recorded in the
 * log file.  If no record, or is a NULL pointer, returns zero.
 *
 * Logs a message if:
 * 1) address was not previously recorded as an allocation
 * 2) address is NULL
 *              
 * AUTHOR:  Adapted from an article in the August 1990 issue of Dr. Dobbs
 *          Journal (Copyright (c) 1990, Cornerstone Systems Group, Inc.)
 *          by T. Habermann, NGDC, (303) 497 - 6472, haber@ngdc.noaa.gov
 *
 * COMMENTS:This function is only called if MEMTRACK is defined.  If environmental
 * variable MEMTRACK is not defined then memory log tracking is disabled.
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * KEYWORDS:
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "MEMTrack_free" 

static int MEMTrack_free(FF_MEM_LOG_PTR log)
{
	char  found = 0;
#ifndef MEMTRACK_B
	char  msg[MAX_LTH];
#endif
	int error = 0;

	error = MEMTrack_fp(log);
	if (error == MEM_OPEN_FILE)
	{/* MEMTRACK is not defined */
		/* do special check on freeing NULL */
		if ((void *)log->entry->addr == NULL)
		{
#ifndef MEMTRACK_B
			sprintf(msg, "BAD free(), NULL (%s)", log->entry->tag);
			MEMTrack_msg(msg, log->entry->routine, log->entry->file, log->entry->line);
#endif
			FF_DIE
		}

		return(error);
	}
	else if (error && error != MEM_OPEN_FILE)
		return(error);
	else if (log->file)
	{
		log->entry->state = ALLOC;
		found = MEMTrack_find_entry(log);
		if (found)
		{
			fseek(log->file, log->pos, SEEK_SET);
#ifdef MEMTRACK_B
			log->entry->state = FREE;
			fwrite(log->entry, sizeof(*log->entry), 1, log->file);
#else
			fputc(FREE, log->file); /* Over-write the ALLOC tag */
#endif

			fclose(log->file);

			return(0);
		}
		else
		{
			fclose(log->file);

#ifndef MEMTRACK_B
			sprintf(msg, "BAD free(), unallocated:%lx (%s)", log->entry->addr, log->entry->tag);
			MEMTrack_msg(msg, log->entry->routine, log->entry->file, log->entry->line);
#endif
			FF_DIE
		}
	}

	return error;
}

/*
 * NAME:    MEMTrack_fp
 *      
 * PURPOSE: Return FILE pointer for memory tracking log file.
 *
 * USAGE:   static FILE  *MEMTrack_fp()
 *
 * RETURNS: Zero on success or MEM_OPEN_FILE if MEMTRACK_LOG cannot be opened
 *
 * DESCRIPTION: This function is called by the memory functions
 *              which are active if MEMTRACK is defined.
 *              The name of the memory log file is given by the
 *              environment variable MEMTRACK.
 *              
 * AUTHOR:  Adapted from an article in the August 1990 issue of Dr. Dobbs
 *          Journal (Copyright (c) 1990, Cornerstone Systems Group, Inc.)
 *          by T. Habermann, NGDC, (303) 497 - 6472, haber@ngdc.noaa.gov
 *
 * COMMENTS:This function is only called if MEMTRACK is defined.  If environmental
 * variable MEMTRACK is not defined then no action is taken.
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * KEYWORDS:
 *
 * ERRORS:  
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "MEMTrack_fp" 

static int MEMTrack_fp(FF_MEM_LOG_PTR log)
{
	static enum {first_time, enabled, disabled} mem_state = first_time;

	if (mem_state == first_time)
	{
		mem_state = enabled;

#if FF_CC == FF_CC_MSVC1 || FF_CC == FF_CC_MSVC4 || FF_CC == FF_CC_MACCW
		_fmode = O_BINARY;
#endif
		log->file = fopen(MEMTRACK_LOG,"r");
		if (log->file)
		{
			fclose(log->file);

#if FF_CC == FF_CC_MSVC1 || FF_CC == FF_CC_MSVC4 || FF_CC == FF_CC_MACCW
			_fmode = O_BINARY;
#endif
			log->file = fopen(MEMTRACK_LOG, "w");
			if (log->file)
			{
				fclose(log->file);
#ifdef MEMTRACK_B
				fprintf(stderr, "Binary ");
#else
				fprintf(stderr, "ASCII ");
#endif
				fprintf(stderr, "MEMTrack logging file \"%s\" enabled\n", MEMTRACK_LOG);
			}
			else
			{
				fprintf(stderr, "Cannot write to Memtrack logging file \"%s\"\n", MEMTRACK_LOG);
				exit(EXIT_FAILURE);
			}
		}
		else
		{
			mem_state = disabled;
			
			fprintf(stderr, "Memtrack logging file \"%s\" does not exist\n", MEMTRACK_LOG);
			fprintf(stderr, "Memory tracking and error detection will not be enabled\n");

			return(MEM_OPEN_FILE);
		}
	} /* if (first_time) */

	if (mem_state == enabled)
	{
#if FF_CC == FF_CC_MSVC1 || FF_CC == FF_CC_MSVC4 || FF_CC == FF_CC_MACCW
		_fmode = O_BINARY;
#endif
		log->file = fopen(MEMTRACK_LOG,"r+"); /* Open debugging file for append access. */
		if (!log->file)
		{
			fprintf(stderr, "Cannot read/append to file \"%s\"\n", MEMTRACK_LOG);
#ifndef MEMTRACK_B
			fprintf(stderr, "Called from %s %s %d (%s)\n", log->entry->routine, log->entry->file, (int)log->entry->line, log->entry->tag);
#endif
			exit(EXIT_FAILURE);
		}
	
		return(0);
	}
	else if (mem_state == disabled)
	{
		log->file = NULL;
		return(MEM_OPEN_FILE);
	}

	return 0;
}

/*
 * NAME:    MEMCalloc
 *      
 * PURPOSE: Same as calloc(), but registers activity using MEMTrack().
 *
 * USAGE:   void *MEMCalloc(size_t num_elems, size_t bytes_per_elem, char *tag, char *routine_name, char *cfile_name, int line_number)
 *
 * RETURNS: same as calloc()
 *
 * DESCRIPTION: This function provides a wrapper around calloc which is
 *              called only if MEMTRACK is defined. The wrapper calls
 *              MEMTrack_alloc which writes out the location and size
 *              of the allocated block of memory along with the tag
 *              given to this function.
 *
 * <PLEASE NOTE THAT THE FOLLOWING STATEMENT IS NO LONGER TRUE>
 *              The fill of allocated memory with the value 0XCC is
 *              used to clearly indicate allocated memory while in
 *              the debugger. This was suggested by Steve Maguire in
 *              his book "Writing Solid Code".
 *
 * <ADD'L NOTE>
 * To perform a memset() with the fill char on the allocated block is to render
 * MEMCalloc() no different than MEMMalloc(), which was causing  problems with
 * newform.
 *              
 * AUTHOR:  Adapted from an article in the August 1990 issue of Dr. Dobbs
 *          Journal (Copyright (c) 1990, Cornerstone Systems Group, Inc.)
 *          by T. Habermann, NGDC, (303) 497 - 6472, haber@ngdc.noaa.gov
 *
 * COMMENTS:This function is only called if MEMTRACK is defined.  If environmental
 * variable MEMTRACK is not defined then memory log tracking is disabled.
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * KEYWORDS:
 *
 */
#ifdef MEMTRACK_B
void *MEMCalloc(size_t num_elems, size_t bytes_per_elem)
#else
void *MEMCalloc(size_t num_elems, size_t bytes_per_elem, char *tag, char *routine_name, char *cfile_name, int line_number)
#endif
{
	void *allocated;
	
	FF_MEM_ENTRY entry;
	FF_MEM_LOG   log;

	entry.addr = 0;
	entry.size = 0;

#ifndef MEMTRACK_B
	entry.tag = tag;
	entry.routine = routine_name;
	entry.file = cfile_name;
	entry.line = line_number;
#endif

	log.entry = &entry;
	
	heap_check(&log);

	allocated = calloc(num_elems + BOTH_POSTS_SIZE, bytes_per_elem);
	if (allocated)
	{
		log.entry->addr = (long)((char HUGE *)allocated + ONE_POST_SIZE);
		log.entry->size = num_elems * bytes_per_elem;

		MEMTrack_alloc(&log);
		set_posts(log.entry);
	}
	
	return((void *)log.entry->addr);
}

/*
 * NAME:    MEMFree
 *      
 * PURPOSE: Same as free(), but registers activity using MEMTrack().
 *
 * USAGE:   void MEMFree(void *to_free, char *tag, char *routine_name, char *cfile_name, int line_number)
 *
 * RETURNS: None
 *
 * DESCRIPTION: This function provides a wrapper around free which is
 *              called only if MEMTRACK is defined. The wrapper calls
 *              MEMTrack_free which checks to insure that the memory
 *              being freed was allocated and modifies the line for
 *              that block in the memory tracking log file. 
 *
 * Pointers deemed invalid are not free()'d.  Memory blocks that are deemed
 * valid are filled with the FILL_CHAR character before being free()'d. 
 *              
 * AUTHOR:  Adapted from an article in the August 1990 issue of Dr. Dobbs
 *          Journal (Copyright (c) 1990, Cornerstone Systems Group, Inc.)
 *          by T. Habermann, NGDC, (303) 497 - 6472, haber@ngdc.noaa.gov
 *
 * COMMENTS:This function is only called if MEMTRACK is defined.  If environmental
 * variable MEMTRACK is not defined then memory log tracking is disabled.
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * KEYWORDS:
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "MEMFree" 

#ifdef MEMTRACK_B
void MEMFree(void *user_block)
#else
void MEMFree(void *user_block, char *tag, char *routine_name, char *cfile_name, int line_number)
#endif
{
	size_t size = 0;
	int error;

	FF_MEM_ENTRY entry;
	FF_MEM_LOG   log;

	entry.addr = (long)user_block;
	entry.size = 0;

#ifndef MEMTRACK_B
	entry.tag = tag;
	entry.routine = routine_name;
	entry.file = cfile_name;
	entry.line = line_number;
#endif

	log.entry = &entry;

	heap_check(&log);

	error = MEMTrack_free(&log);
	if (error != MEM_PTR_DEF)
	{
		char *allocated = (char *)user_block - ONE_POST_SIZE;

		/* garbage shredding */
		if (size)
			memset(allocated, FILL_CHAR, (size_t)size + BOTH_POSTS_SIZE);

		free(allocated);
	}
}

/*
 * NAME:    MEMMalloc
 *      
 * PURPOSE: Same as malloc(), but registers activity using MEMTrack().
 *
 * USAGE:   void *MEMMalloc(size_t number_of_bytes, char *tag, char *routine_name, char *cfile_name, int line_number)
 *
 * RETURNS: same as malloc() (note that memory area is filled with FILL_CHAR value)
 *
 * DESCRIPTION: This function provides a wrapper around malloc which is
 *              called only if MEMTRACK is defined. The wrapper calls
 *              MEMTrack_alloc which writes out the location and size
 *              of the allocated block of memory along with the tag
 *              given to this function.
 *              The fill of allocated memory with the value 0XCC is
 *              used to clearly indicate allocated memory while in
 *              the debugger. This was suggested by Steve Maguire in
 *              his book "Writing Solid Code".
 *              
 * AUTHOR:  Adapted from an article in the August 1990 issue of Dr. Dobbs
 *          Journal (Copyright (c) 1990, Cornerstone Systems Group, Inc.)
 *          by T. Habermann, NGDC, (303) 497 - 6472, haber@ngdc.noaa.gov
 *
 * COMMENTS:This function is only called if MEMTRACK is defined.  If environmental
 * variable MEMTRACK is not defined then memory log tracking is disabled.
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * KEYWORDS:
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "MEMMalloc" 

#ifdef MEMTRACK_B
void *MEMMalloc(size_t bytes)
#else
void *MEMMalloc(size_t bytes, char *tag, char *routine_name, char *cfile_name, int line_number)
#endif
{
	void *allocated;

	FF_MEM_ENTRY entry;
	FF_MEM_LOG   log;

	entry.addr = 0;
	entry.size = bytes;

#ifndef MEMTRACK_B
	entry.tag = tag;
	entry.routine = routine_name;
	entry.file = cfile_name;
	entry.line = line_number;
#endif

	log.entry = &entry;

	heap_check(&log);

	allocated = malloc(bytes + BOTH_POSTS_SIZE);
	if (allocated != NULL)
	{
		log.entry->addr = (long)((char *)allocated + ONE_POST_SIZE);
		log.entry->size = bytes;
		
		MEMTrack_alloc(&log);

		memset((void *)log.entry->addr, FILL_CHAR, bytes);
		set_posts(log.entry);
	}
	
	return((void *)log.entry->addr);
}

/*
 * NAME:    MEMRealloc
 *      
 * PURPOSE: Same as realloc(), but registers activity using MEMTrack().
 *
 * USAGE:   void *MEMRealloc(void *allocated, size_t bytes, char *tag, char *routine_name, char *cfile_name, int line_number)
 *
 * RETURNS: same as realloc()
 *
 * DESCRIPTION: This function provides a wrapper around realloc which is
 *              called only if MEMTRACK is defined. The wrapper calls
 *              MEMTrack_alloc which writes out the location and size
 *              of the allocated block of memory along with the tag
 *              given to this function.
 *
 * If the memory block is being expanded, then the expanded tail is filled
 * with FILL_CHAR.  If the memory block is being shrinked, then
 *
 * Logs the reallocation as first a free of the address then a new allocation
 * (regardless if the memory area is simply resized or moved).
 *              
 * AUTHOR:  Adapted from an article in the August 1990 issue of Dr. Dobbs
 *          Journal (Copyright (c) 1990, Cornerstone Systems Group, Inc.)
 *          by T. Habermann, NGDC, (303) 497 - 6472, haber@ngdc.noaa.gov
 *
 * COMMENTS:This function is only called if MEMTRACK is defined.  If environmental
 * variable MEMTRACK is not defined then memory log tracking is disabled.
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * KEYWORDS:         
 *
 * ERRORS:  
 *
 */
#ifdef MEMTRACK_B
void *MEMRealloc(void *user_block, size_t bytes)
#else
void *MEMRealloc(void *user_block, size_t bytes, char *tag, char *routine_name, char *cfile_name, int line_number)
#endif
{
	char *allocated;
	int error;

	FF_MEM_ENTRY entry;
	FF_MEM_LOG   log;

	entry.addr = (long)user_block;
	entry.size = 0;

#ifndef MEMTRACK_B
	entry.tag = tag;
	entry.routine = routine_name;
	entry.file = cfile_name;
	entry.line = line_number;
#endif

	log.entry = &entry;

	heap_check(&log);

	if (user_block)
	{
		allocated = (char *)user_block - ONE_POST_SIZE;

		error = MEMTrack_free(&log);

		/* garbage shredding */
		if (!error && log.entry->size)
		{
			memset(allocated, FILL_CHAR, ONE_POST_SIZE);
			memset((char *)log.entry->addr + log.entry->size, FILL_CHAR, ONE_POST_SIZE);

			if (bytes < (size_t)log.entry->size)
				memset((char *)log.entry->addr + bytes, FILL_CHAR, (size_t)(log.entry->size - bytes));
		}
	}
	else
		allocated = NULL;

	allocated = realloc(allocated, bytes + BOTH_POSTS_SIZE);
	if (allocated)
	{
		log.entry->addr = (long)((char *)allocated + ONE_POST_SIZE);
		log.entry->size = bytes;
		
		MEMTrack_alloc(&log);
		
		set_posts(log.entry);
	}

	return((void *)log.entry->addr);
}

/*
 * NAME:    MEMStrdup
 *      
 * PURPOSE: Same as strdup(), but registers activity using MEMTrack().
 *
 * USAGE:   char *MEMStrdup(void *allocated, char *tag, char *routine_name, char *cfile_name, int line_number)
 *
 * RETURNS: same as strdup()
 *
 * DESCRIPTION: This function provides a wrapper around strdup which is
 *              called only if MEMTRACK is defined. The wrapper calls
 *              MEMTrack_alloc which writes out the location and size
 *              of the allocated block of memory along with the tag
 *              given to this function.
 *              The fill of allocated memory with the value 0XCC is
 *              used to clearly indicate allocated memory while in
 *              the debugger. This was suggested by Steve Maguire in
 *              his book "Writing Solid Code".
 *
 * Logs a warning if:
 * 1) the memory allocation of the string to be duplicated was not logged
 *              
 * AUTHOR:  Adapted from an article in the August 1990 issue of Dr. Dobbs
 *          Journal (Copyright (c) 1990, Cornerstone Systems Group, Inc.)
 *          by T. Habermann, NGDC, (303) 497 - 6472, haber@ngdc.noaa.gov
 *
 * COMMENTS:This function is only called if MEMTRACK is defined.  If environmental
 * variable MEMTRACK is not defined then memory log tracking is disabled.
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * KEYWORDS:
 *
 */
#ifdef MEMTRACK_B
char *MEMStrdup(const char *string)
#else
char *MEMStrdup(const char *string, char *tag, char *routine_name, char *cfile_name, int line_number)
#endif
{
	char *user_block;

#if MEMTRACK == MEMTRACK_ULTRA && !defined(MEMTRACK_B)
	int error;
#endif
	
	FF_MEM_ENTRY entry;
	FF_MEM_LOG   log;

	entry.state = ALLOC;

	entry.addr = (long)string;
	entry.size = 0;

#ifndef MEMTRACK_B
	entry.tag = tag;
	entry.routine = routine_name;
	entry.file = cfile_name;
	entry.line = line_number;
#endif

	log.entry = &entry;

	heap_check(&log);

#if MEMTRACK == MEMTRACK_ULTRA && !defined(MEMTRACK_B)
	error = MEMTrack_find_entry(&log);
	if (!error && !log.entry->size)
	{
		char msg[MAX_LTH];

		sprintf(msg, "Duplicating untracked string (%s)", tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
#endif

#ifdef MEMTRACK_B
	user_block = MEMMalloc(strlen(string) + 1);
#else
	user_block = MEMMalloc(strlen(string) + 1, tag, routine_name, cfile_name, line_number);
#endif
	if (user_block)
		strcpy(user_block, string);

	return(user_block);
}

/*
 * NAME:    MEMExit
 *      
 * PURPOSE: Same as exit(), but registers activity using MEMTrack().
 *
 * USAGE:   void MEMExit(int status, char *tag, char *routine_name, char *cfile_name, int line_number)
 *
 * RETURNS: same as exit()
 *
 * DESCRIPTION: This function provides a wrapper around exit which is
 * called only if MEMTRACK is defined. The wrapper calls heap_check, then
 * exit.
 *
 * AUTHOR:  Adapted from an article in the August 1990 issue of Dr. Dobbs
 *          Journal (Copyright (c) 1990, Cornerstone Systems Group, Inc.)
 *          by T. Habermann, NGDC, (303) 497 - 6472, haber@ngdc.noaa.gov
 *
 * COMMENTS:This function is only called if MEMTRACK is defined.  If environmental
 * variable MEMTRACK is not defined then memory log tracking is disabled.
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * KEYWORDS:
 *
 */
#ifdef MEMTRACK_B
void MEMExit(int status)
#else
void MEMExit(int status, char *tag, char *routine_name, char *cfile_name, int line_number)
#endif
{
	FF_MEM_ENTRY entry;
	FF_MEM_LOG   log;

	entry.state = ALLOC;

	entry.addr = 0;
	entry.size = 0;

#ifndef MEMTRACK_B
	entry.tag = tag;
	entry.routine = routine_name;
	entry.file = cfile_name;
	entry.line = line_number;
#endif

	log.entry = &entry;

	leak_test(&log);

	exit(status);
}

/*
 * NAME:    MEMStrcpy
 *      
 * PURPOSE: Same as strcpy(), but warns for some memory errors.
 *
 * USAGE:   MEMStrcpy(char *s, const char *ct, char *tag, char *routine_name, char *cfile_name, int line_number);
 *
 * RETURNS: same as strcpy()
 * 
 *
 * DESCRIPTION:	This function provides a wrapper around strcpy which is called only if
 * MEMTRACK is defined.  Furthermore, checks memtrack file for previous allocation and
 * checks sizes for fit.  Warns if FILL_CHAR is first character of ct, if address of
 * cs cannot be found in memory log, or if given allocated size is less than size of ct.
 *              
 * Logs a warning if:
 * 1) first character of ct is FILL_CHAR
 * 2) s is NULL
 * 3) memory allocation of s is not tracked in log file
 * 4) strlen(ct) >= allocation size of s (when tracked)
 *  
 * AUTHOR:  Adapted from an article in the August 1990 issue of Dr. Dobbs
 *          Journal (Copyright (c) 1990, Cornerstone Systems Group, Inc.)
 *          by Mark A. Ohrenschall, NGDC, (303) 497 - 6124, mao@ngdc.noaa.gov
 *
 * COMMENTS:This function is only called if MEMTRACK is defined.  If environmental
 * variable MEMTRACK is not defined then memory log tracking is disabled.
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * KEYWORDS:
 *
 * ERRORS:  strlen(ct) may dump core if ct is not properly NULL-terminated
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "MEMStrcpy" 

#ifdef MEMTRACK_B
char *MEMStrcpy(char *s, const char *ct)
#else
char *MEMStrcpy(char *s, const char *ct, char *tag, char *routine_name, char *cfile_name, int line_number)
#endif
{
#ifndef MEMTRACK_B
	char msg[MAX_LTH];
	int error;
	
	FF_MEM_ENTRY entry;
	FF_MEM_LOG   log;

	entry.state = ALLOC;

	entry.addr = (long)s;
	entry.size = 0;

	entry.tag = tag;
	entry.routine = routine_name;
	entry.file = cfile_name;
	entry.line = line_number;

	log.entry = &entry;

	if (*ct == FILL_CHAR)
	{
		sprintf(msg,"copying FILL_CHAR string (%s)", tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
		
	if (s == NULL)
	{
		sprintf(msg,"copying into NULL string (%s)", tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
	
	error = MEMTrack_find_entry(&log);
	if (!error && !log.entry->size)
	{
		sprintf(msg, "copying %lu bytes into untracked string:%lx (%s)", (unsigned long)strlen(ct), (long)s, tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}


	if (!error && log.entry->size && log.entry->size <= strlen(ct))
	{
		sprintf(msg,"copying %lu bytes into %lu byte buffer (%s)", (unsigned long)strlen(ct), (unsigned long)log.entry->size, tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
#endif
	return(strcpy(s, ct));
}

/*
 * NAME:    MEMStrncpy
 *      
 * PURPOSE: Same as strncpy(), but warns for some memory errors.
 *
 * USAGE:   MEMStrncpy(char *s, const char *ct, size_t n, char *tag, char *routine_name, char *cfile_name, int line_number);
 *
 * RETURNS: same as strncpy()
 * 
 *
 * DESCRIPTION:	This function provides a wrapper around strncpy which is called only if
 * MEMTRACK is defined.  Furthermore, checks memtrack file for previous allocation and
 * checks sizes for fit.  Warns if FILL_CHAR is first character of ct, if address of
 * cs cannot be found in memory log, or if given allocated size is less than size of ct.
 *              
 * Logs a warning if:
 * 1) first character of ct is FILL_CHAR
 * 2) s is NULL
 * 3) memory allocation of s is not tracked in log file
 * 4) strlen(ct) or n (which ever is less) >= allocation size of s (when tracked)
 *  
 * AUTHOR:  Adapted from an article in the August 1990 issue of Dr. Dobbs
 *          Journal (Copyright (c) 1990, Cornerstone Systems Group, Inc.)
 *          by Mark A. Ohrenschall, NGDC, (303) 497 - 6124, mao@ngdc.noaa.gov
 *
 * COMMENTS:This function is only called if MEMTRACK is defined.  If environmental
 * variable MEMTRACK is not defined then memory log tracking is disabled.
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * KEYWORDS:
 *
 * ERRORS:  strlen(ct) may dump core if ct is not a properly NULL-terminated string
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "MEMStrncpy" 

#ifdef MEMTRACK_B
char *MEMStrncpy(char *s, const char *ct, size_t n)
#else
char *MEMStrncpy(char *s, const char *ct, size_t n, char *tag, char *routine_name, char *cfile_name, int line_number)
#endif
{
#ifndef MEMTRACK_B
	char msg[MAX_LTH];
	int error;
	
	FF_MEM_ENTRY entry;
	FF_MEM_LOG   log;

	entry.state = ALLOC;

	entry.addr = (long)s;
	entry.size = 0;

	entry.tag = tag;
	entry.routine = routine_name;
	entry.file = cfile_name;
	entry.line = line_number;

	log.entry = &entry;

	if (*ct == FILL_CHAR) {
		sprintf(msg,"copying FILL_CHAR string (%s)", tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
		
	if(s == NULL) {
		sprintf(msg,"copying into NULL string (%s)", tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
	
	error = MEMTrack_find_entry(&log);
	if (!error && !log.entry->size)
	{
		sprintf(msg,"copying %lu bytes into untracked string:%lx (%s)", (unsigned long)(n < strlen(ct) ? n : strlen(ct)), (long)s, tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}

	if (!error && log.entry->size && log.entry->size <= (n < strlen(ct) ? n : strlen(ct))) {
		sprintf(msg,"copying up to %lu bytes into %lu byte buffer (%s)", (unsigned long)(n < strlen(ct) ? n : strlen(ct)), (unsigned long)log.entry->size, tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
#endif

	return(strncpy(s, ct, n));
}

/*
 * NAME:    MEMStrcat
 *      
 * PURPOSE: Same as strcat(), but warns for some memory errors.
 *
 * USAGE:   MEMStrcat(char *s, const char *ct, char *tag, char *routine_name, char *cfile_name, int line_number);
 *
 * RETURNS: same as strcat()
 *
 * DESCRIPTION:	This function provides a wrapper around strcat which is called only if
 * MEMTRACK is defined.  Furthermore, checks memtrack file for previous allocation and
 * checks sizes for fit.  Warns if FILL_CHAR is first character of ct, if address of
 * cs cannot be found in memory log, or if given allocated size is less than size of
 * ct plus s.
 *              
 * Logs a warning if:
 * 1) first character of ct is FILL_CHAR
 * 2) s is NULL
 * 3) memory allocation of s is not tracked in log file
 * 4) strlen(ct) >= allocation size of s minus strlen(s) (when tracked)
 *  
 * AUTHOR:  Adapted from an article in the August 1990 issue of Dr. Dobbs
 *          Journal (Copyright (c) 1990, Cornerstone Systems Group, Inc.)
 *          by Mark A. Ohrenschall, NGDC, (303) 497 - 6124, mao@ngdc.noaa.gov
 *
 * COMMENTS:This function is only called if MEMTRACK is defined.  If environmental
 * variable MEMTRACK is not defined then memory log tracking is disabled.
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * KEYWORDS:
 *
 * ERRORS:  strlen({ct,s}) may dump core if ct or s are not properly NULL-terminated
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "MEMStrcat"

#ifdef MEMTRACK_B
char *MEMStrcat(char *s, const char *ct)
#else
char *MEMStrcat(char *s, const char *ct, char *tag, char *routine_name, char *cfile_name, int line_number)
#endif
{
#ifndef MEMTRACK_B
	char msg[MAX_LTH];
	int error;
	
	FF_MEM_ENTRY entry;
	FF_MEM_LOG   log;

	entry.state = ALLOC;

	entry.addr = (long)s;
	entry.size = 0;

	entry.tag = tag;
	entry.routine = routine_name;
	entry.file = cfile_name;
	entry.line = line_number;

	log.entry = &entry;

	if (*ct == FILL_CHAR) {
		sprintf(msg,"concatenating FILL_CHAR string (%s)", tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
		
	if(s == NULL) {
		sprintf(msg,"concatentating onto NULL string (%s)", tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
	
	error = MEMTrack_find_entry(&log);
	if (!error && !log.entry->size)
	{
		sprintf(msg,"concatenating %lu bytes onto untracked string:%lx (%s)", (unsigned long)strlen(ct), (long)s, tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}

	if (!error && log.entry->size && log.entry->size <= strlen(ct) + strlen(s)) {
		sprintf(msg,"concatenating %lu bytes onto %lu byte buffer from offset %lu (%s)", (unsigned long)strlen(ct), (unsigned long)log.entry->size, (unsigned long)strlen(s), tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
#endif

	return(strcat(s, ct));
}

/*
 * NAME:    MEMStrncat
 *      
 * PURPOSE: Same as strncat(), but checks for some memory errors.
 *
 * USAGE:   MEMStrncat(char *s, const char *ct, size_t n, char *tag, char *routine_name, char *cfile_name, int line_number);
 *
 * RETURNS: same as strncat()
 *
 * DESCRIPTION:	This function provides a wrapper around strncat which is called only if
 * MEMTRACK is defined.  Furthermore, checks memtrack file for previous allocation and
 * checks sizes for fit.  Warns if FILL_CHAR is first character of ct, if address of
 * cs cannot be found in memory log, or if given allocated size is less than size of
 * ct plus s.
 *              
 * Logs a warning if:
 * 1) first character of ct is FILL_CHAR
 * 2) s is NULL
 * 3) memory allocation of s is not tracked in log file
 * 4) strlen(ct) or n (which ever is less) >= allocation size of s minus strlen(s) (when tracked)
 *  
 * AUTHOR:  Adapted from an article in the August 1990 issue of Dr. Dobbs
 *          Journal (Copyright (c) 1990, Cornerstone Systems Group, Inc.)
 *          by Mark A. Ohrenschall, NGDC, (303) 497 - 6124, mao@ngdc.noaa.gov
 *
 * COMMENTS:This function is only called if MEMTRACK is defined.  If environmental
 * variable MEMTRACK is not defined then memory log tracking is disabled.
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * KEYWORDS:
 *
 * ERRORS:  strlen({ct,s}) may dump core if ct or s are not properly NULL-terminated
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "MEMStrncat"

#ifdef MEMTRACK_B
char *MEMStrncat(char *s, const char *ct, size_t n)
#else
char *MEMStrncat(char *s, const char *ct, size_t n, char *tag, char *routine_name, char *cfile_name, int line_number)
#endif
{
#ifndef MEMTRACK_B
	char msg[MAX_LTH];
	int error;
	
	FF_MEM_ENTRY entry;
	FF_MEM_LOG   log;

	entry.state = ALLOC;

	entry.addr = (long)s;
	entry.size = 0;

	entry.tag = tag;
	entry.routine = routine_name;
	entry.file = cfile_name;
	entry.line = line_number;

	log.entry = &entry;

	if (*ct == FILL_CHAR) {
		sprintf(msg,"concatenating FILL_CHAR string (%s)", tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
		
	if(s == NULL) {
		sprintf(msg,"concatentating onto NULL string (%s)", tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
	
	error = MEMTrack_find_entry(&log);
	if (!error && !log.entry->size)
	{
		sprintf(msg,"concatenating %lu bytes onto untracked string:%lx (%s)", (unsigned long)(n < strlen(ct) ? n : strlen(ct)), (long)s, tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}

	if (!error && log.entry->size && log.entry->size <= (n < strlen(ct) ? n : strlen(ct)) + strlen(s)) {
		sprintf(msg,"concatenating %lu bytes onto %lu byte buffer from offset %lu (%s)", (unsigned long)(n < strlen(ct) ? n : strlen(ct)), (unsigned long)log.entry->size, (unsigned long)strlen(s), tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
#endif

	return(strncat(s, ct, n));
}

/*
 * NAME:    MEMStrcmp
 *      
 * PURPOSE: Same as strcmp(), but checks for some memory errors.
 *
 * USAGE:   int MEMStrcmp(const char *cs, const char *ct, char *tag, char *routine_name, char *cfile_name, int line_number);
 *
 * RETURNS: Same as strcmp().
 *
 * DESCRIPTION:	This function provides a wrapper around strcmp which
 *						is called only if MEMTRACK is defined. The wrapper checks to
 *						make sure that the strings being compared are allocated and filled.
 *              
 * Logs a warning if:
 * 1) cs is NULL
 * 2) first character of cs is FILL_CHAR
 * 3) ct is NULL
 * 4) first character of ct is FILL_CHAR
 *  
 * AUTHOR:  Adapted from an article in the August 1990 issue of Dr. Dobbs
 *          Journal (Copyright (c) 1990, Cornerstone Systems Group, Inc.)
 *          by T. Habermann, NGDC, (303) 497 - 6472, haber@ngdc.noaa.gov
 *
 * COMMENTS:This function is only called if MEMTRACK is defined.  If environmental
 * variable MEMTRACK is not defined then memory log tracking is disabled.
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * KEYWORDS:
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "MEMStrcmp" 

#ifdef MEMTRACK_B
int MEMStrcmp(const char *cs, const char *ct)
#else
int MEMStrcmp(const char *cs, const char *ct, char *tag, char *routine_name, char *cfile_name, int line_number)
#endif
{
#ifndef MEMTRACK_B
	char msg[MAX_LTH];
	
	if(cs == NULL){
		sprintf(msg,"comparing NULL string (cs) (%s)", tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
	if(*cs == FILL_CHAR){
		sprintf(msg,"comparing FILL_CHAR string (cs) (%s)", tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
	if(ct == NULL){
		sprintf(msg,"searching NULL string (ct) (%s)", tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
	if(*ct == FILL_CHAR){
		sprintf(msg,"searching FILL_CHAR string (ct) (%s)", tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
#endif

	return(strcmp(cs, ct));
}

/*
 * NAME:    MEMStrncmp
 *      
 * PURPOSE: Same as strncmp(), but checks for some memory errors.
 *
 * USAGE:   int MEMStrncmp(const char *cs, const char *ct, size_t bytes, char *tag, char *routine_name, char *cfile_name, int line_number);
 *
 * RETURNS: Same as strncmp().
 *
 * DESCRIPTION:	This function provides a wrapper around strncmp which
 *						is called only if MEMTRACK is defined. The wrapper checks to
 *						make sure that the strings being compared are allocated and filled.
 *              
 * Logs a warning if:
 * 1) cs is NULL
 * 2) first character of cs is FILL_CHAR
 * 3) ct is NULL
 * 4) first character of ct is FILL_CHAR
 *  
 * AUTHOR:  Adapted from an article in the August 1990 issue of Dr. Dobbs
 *          Journal (Copyright (c) 1990, Cornerstone Systems Group, Inc.)
 *          by T. Habermann, NGDC, (303) 497 - 6472, haber@ngdc.noaa.gov
 *
 * COMMENTS:This function is only called if MEMTRACK is defined.  If environmental
 * variable MEMTRACK is not defined then memory log tracking is disabled.
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * KEYWORDS:
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "MEMStrncmp" 

#ifdef MEMTRACK_B
int MEMStrncmp(const char *cs, const char *ct, size_t n)
#else
int MEMStrncmp(const char *cs, const char *ct, size_t n, char *tag, char *routine_name, char *cfile_name, int line_number)
#endif
{
#ifndef MEMTRACK_B
	char msg[MAX_LTH];
	
	if(cs == NULL){
		sprintf(msg,"comparing NULL string (cs) (%s)", tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
	if(*cs == FILL_CHAR){
		sprintf(msg,"comparing FILL_CHAR string (cs) (%s)", tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
	if(ct == NULL){
		sprintf(msg,"searching NULL string (ct) (%s)", tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
	if(*ct == FILL_CHAR){
		sprintf(msg,"searching FILL_CHAR string (ct) (%s)", tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
#endif

	return(strncmp(cs, ct, n));
}

/*
 * NAME:    MEMStrchr
 *      
 * PURPOSE: Same as strchr(), but checks for some memory errors.
 *
 * USAGE:   char *MEMStrchr(const char *cs, int c, char *tag, char *routine_name, char *cfile_name, int line_number);
 *
 * RETURNS: same as strchr()
 *
 * DESCRIPTION:	This function provides a wrapper around strchr which
 *						is called only if MEMTRACK is defined. The wrapper makes several
 *						memory checks: it checks to make sure that the string being searched
 *						is not NULL, and that that string is not pointing to memory
 *						which is allocated, but not filled.
 *              
 * Logs a warning if:
 * 1) cs is NULL
 * 2) first character of cs is FILL_CHAR
 *              
 * AUTHOR:  Adapted from an article in the August 1990 issue of Dr. Dobbs
 *          Journal (Copyright (c) 1990, Cornerstone Systems Group, Inc.)
 *          by T. Habermann, NGDC, (303) 497 - 6472, haber@ngdc.noaa.gov
 *
 * COMMENTS:This function is only called if MEMTRACK is defined.  If environmental
 * variable MEMTRACK is not defined then memory log tracking is disabled.
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * KEYWORDS:
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "MEMStrchr" 

#ifdef MEMTRACK_B
char *MEMStrchr(const char *cs, int c)
#else
char *MEMStrchr(const char *cs, int c, char *tag, char *routine_name, char *cfile_name, int line_number)
#endif
{
#ifndef MEMTRACK_B
	char msg[MAX_LTH];
	
	if(cs == NULL){
		sprintf(msg,"searching NULL string (%s)", tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
	if(*cs == FILL_CHAR){
		sprintf(msg,"searching FILL_CHAR string (%s)", tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
#endif

	return(strchr(cs, c));
}

/*
 * NAME:    MEMStrrchr
 *      
 * PURPOSE: Same as strrchr(), but checks for some memory errors.
 *
 * USAGE:   char *MEMStrrchr(const char *cs, int c, char *tag, char *routine_name, char *cfile_name, int line_number);
 *
 * RETURNS: same as strrchr()
 *
 * DESCRIPTION:	This function provides a wrapper around strrchr which
 *						is called only if MEMTRACK is defined. The wrapper makes several
 *						memory checks: it checks to make sure that the string being searched
 *						is not NULL, and that that string is not pointing to memory
 *						which is allocated, but not filled.
 *              
 * Logs a warning if:
 * 1) cs is NULL
 * 2) first character of cs is FILL_CHAR
 *              
 * AUTHOR:  Adapted from an article in the August 1990 issue of Dr. Dobbs
 *          Journal (Copyright (c) 1990, Cornerstone Systems Group, Inc.)
 *          by T. Habermann, NGDC, (303) 497 - 6472, haber@ngdc.noaa.gov
 *
 * COMMENTS:This function is only called if MEMTRACK is defined.  If environmental
 * variable MEMTRACK is not defined then memory log tracking is disabled.
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * KEYWORDS:
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "MEMStrrchr" 

#ifdef MEMTRACK_B
char *MEMStrrchr(const char *cs, int c)
#else
char *MEMStrrchr(const char *cs, int c, char *tag, char *routine_name, char *cfile_name, int line_number)
#endif
{
#ifndef MEMTRACK_B
	char msg[MAX_LTH];

	if(cs == NULL){
		sprintf(msg,"searching NULL string (%s)", tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
	if(*cs == FILL_CHAR){
		sprintf(msg,"searching FILL_CHAR string (%s)", tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
#endif

	return(strrchr(cs, c));
}

/*
 * NAME:    MEMStrstr
 *      
 * PURPOSE: Same as strstr(), but checks for some memory errors.
 *
 * USAGE:   char *MEMStrstr(const char *cs, const char *ct, char *tag, char *routine_name, char *cfile_name, int line_number);
 *
 * RETURNS: same as strstr()
 *
 * DESCRIPTION:	This function provides a wrapper around strchr which
 *						is called only if MEMTRACK is defined. The wrapper makes several
 *						memory checks: it checks to make sure that the string being searched
 *						is not NULL, and that that string is not pointing to memory
 *						which is allocated, but not filled.
 *              
 * Logs a warning if:
 * 1) cs is NULL
 * 2) first character of cs is FILL_CHAR
 * 3) ct is NULL
 * 4) first character of ct is FILL_CHAR
 *  
 * AUTHOR:  Adapted from an article in the August 1990 issue of Dr. Dobbs
 *          Journal (Copyright (c) 1990, Cornerstone Systems Group, Inc.)
 *          by T. Habermann, NGDC, (303) 497 - 6472, haber@ngdc.noaa.gov
 *
 * COMMENTS:This function is only called if MEMTRACK is defined.  If environmental
 * variable MEMTRACK is not defined then memory log tracking is disabled.
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * KEYWORDS:
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "MEMStrstr" 

#ifdef MEMTRACK_B
char *MEMStrstr(const char *cs, const char *ct)
#else
char *MEMStrstr(const char *cs, const char *ct, char *tag, char *routine_name, char *cfile_name, int line_number)
#endif
{
#ifndef MEMTRACK_B
	char msg[MAX_LTH];
	
	FF_MEM_ENTRY entry;
	FF_MEM_LOG   log;

	entry.state = ALLOC;

	entry.addr = (long)cs;
	entry.size = 0;

	entry.tag = tag;
	entry.routine = routine_name;
	entry.file = cfile_name;
	entry.line = line_number;

	log.entry = &entry;

	if(cs == NULL){
		sprintf(msg,"searching NULL string (cs) (%s)", tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
	if(*cs == FILL_CHAR){
		sprintf(msg,"searching FILL_CHAR string (cs) (%s)", tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
	if(ct == NULL){
		sprintf(msg,"searching NULL string (ct) (%s)", tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
	if(*ct == FILL_CHAR){
		sprintf(msg,"searching FILL_CHAR string (ct) (%s)", tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
#endif

	return(strstr(cs, ct));
}

/*
 * NAME:    MEMMemcpy
 *      
 * PURPOSE: Same as memcpy(), but checks for some memory errors.
 *
 * USAGE:   void *MEMMemcpy(char *s, const char *ct, size_t n, char *tag, char *routine_name, char *cfile_name, int line_number);
 *
 * RETURNS: same as memcpy()
 * 
 *
 * DESCRIPTION:	This function provides a wrapper around memcpy which is called only if
 * MEMTRACK is defined.  Furthermore, checks memtrack file for previous allocation and
 * checks sizes for fit.  Warns if FILL_CHAR is first character of ct, if address of
 * cs cannot be found in memory log, or if given allocated size is less than size of ct.
 *              
 * Logs a warning if:
 * 1) first character of ct is FILL_CHAR
 * 2) s is NULL
 * 3) memory allocation of s is not tracked in log file
 * 4) strlen(ct) > allocation size of s (when tracked)
 *  
 * AUTHOR:  Adapted from an article in the August 1990 issue of Dr. Dobbs
 *          Journal (Copyright (c) 1990, Cornerstone Systems Group, Inc.)
 *          by Mark A. Ohrenschall, NGDC, (303) 497 - 6124, mao@ngdc.noaa.gov
 *
 * COMMENTS:This function is only called if MEMTRACK is defined.  If environmental
 * variable MEMTRACK is not defined then memory log tracking is disabled.
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * KEYWORDS:
 *
 * ERRORS:
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "MEMMemcpy" 

#ifdef MEMTRACK_B
#if FF_CC == FF_CC_UNIX
char *MEMMemcpy(char *s, const char *ct, size_t n)
#endif
#if FF_CC == FF_CC_MSVC1 || FF_CC == FF_CC_MSVC4 || FF_CC == FF_CC_MACCW
void *MEMMemcpy(char *s, const char *ct, size_t n)
#endif
#else
#if FF_CC == FF_CC_UNIX
char *MEMMemcpy(char *s, const char *ct, size_t n, char *tag, char *routine_name, char *cfile_name, int line_number)
#endif
#if FF_CC == FF_CC_MSVC1 || FF_CC == FF_CC_MSVC4 || FF_CC == FF_CC_MACCW
void *MEMMemcpy(char *s, const char *ct, size_t n, char *tag, char *routine_name, char *cfile_name, int line_number)
#endif
#endif

{
#ifndef MEMTRACK_B
	char msg[MAX_LTH];
	int error;
	
	FF_MEM_ENTRY entry;
	FF_MEM_LOG   log;

	entry.state = ALLOC;

	entry.addr = (long)s;
	entry.size = 0;

	entry.tag = tag;
	entry.routine = routine_name;
	entry.file = cfile_name;
	entry.line = line_number;

	log.entry = &entry;

	if (*ct == FILL_CHAR) {
		sprintf(msg,"copying FILL_CHAR buffer (%s)", tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
		
	if(s == NULL) {
		sprintf(msg,"copying into NULL buffer (%s)", tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
	
	error = MEMTrack_find_entry(&log);
	if (!error && !log.entry->size)
	{
		sprintf(msg,"copying %lu bytes into untracked buffer:%lx (%s)", (unsigned long)n, (long)s, tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
	
	if (!error && log.entry->size && log.entry->size < n) {
		sprintf(msg,"copying %lu bytes into %lu byte buffer (%s)", (unsigned long)n, (unsigned long)log.entry->size, tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
#endif
	
	return(memcpy(s, ct, n));
}

/*
 * NAME:    MEMMemmove
 *      
 * PURPOSE: Same as memmove(), but checks for some memory errors.
 *
 * USAGE:  void *MEMMemmove(char *s, const char *ct, size_t n, char *tag, char *routine_name, char *cfile_name, int line_number);
 *
 * RETURNS: same as memmove()
 * 
 *
 * DESCRIPTION:	This function provides a wrapper around memmove which is called only if
 * MEMTRACK is defined.  Furthermore, checks memtrack file for previous allocation and
 * checks sizes for fit.  Warns if FILL_CHAR is first character of ct, if address of
 * cs cannot be found in memory log, or if given allocated size is less than size of ct.
 *              
 * Logs a warning if:
 * 1) first character of ct is FILL_CHAR
 * 2) s is NULL
 * 3) memory allocation of s is not tracked in log file
 * 4) strlen(ct) > allocation size of s (when tracked)
 *  
 * AUTHOR:  Adapted from an article in the August 1990 issue of Dr. Dobbs
 *          Journal (Copyright (c) 1990, Cornerstone Systems Group, Inc.)
 *          by Mark A. Ohrenschall, NGDC, (303) 497 - 6124, mao@ngdc.noaa.gov
 *
 * COMMENTS:This function is only called if MEMTRACK is defined.  If environmental
 * variable MEMTRACK is not defined then memory log tracking is disabled.
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * KEYWORDS:
 *
 * ERRORS:
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "MEMMemmove" 

#ifdef MEMTRACK_B
#if FF_CC == FF_CC_UNIX
char *MEMMemmove(char *s, const char *ct, size_t n)
#endif
#if FF_CC == FF_CC_MSVC1 || FF_CC == FF_CC_MSVC4 || FF_CC == FF_CC_MACCW
void *MEMMemmove(char *s, const char *ct, size_t n)
#endif
#else
#if FF_CC == FF_CC_UNIX
char *MEMMemmove(char *s, const char *ct, size_t n, char *tag, char *routine_name, char *cfile_name, int line_number)
#endif
#if FF_CC == FF_CC_MSVC1 || FF_CC == FF_CC_MSVC4 || FF_CC == FF_CC_MACCW
void *MEMMemmove(char *s, const char *ct, size_t n, char *tag, char *routine_name, char *cfile_name, int line_number)
#endif
#endif
{
#ifndef MEMTRACK_B
	char msg[MAX_LTH];
	int error;
	
	FF_MEM_ENTRY entry;
	FF_MEM_LOG   log;

	entry.state = ALLOC;

	entry.addr = (long)s;
	entry.size = 0;

	entry.tag = tag;
	entry.routine = routine_name;
	entry.file = cfile_name;
	entry.line = line_number;

	log.entry = &entry;

	if (*ct == FILL_CHAR) {
		sprintf(msg,"moving FILL_CHAR buffer (%s)", tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
		
	if(s == NULL) {
		sprintf(msg,"moving into NULL buffer (%s)", tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
	
	error = MEMTrack_find_entry(&log);
	if (!error && !log.entry->size)
	{
		sprintf(msg,"moving %lu bytes into untracked buffer:%lx (%s)", (unsigned long)n, (long)s, tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
	
	if (!error && log.entry->size && log.entry->size < n) {
		sprintf(msg,"moving %lu bytes into %lu byte buffer (%s)", (unsigned long)n, (unsigned long)log.entry->size, tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
#endif
	
	return(memmove(s, ct, n));
}

/*
 * NAME:    MEMMemset
 *      
 * PURPOSE: Same as memset(), but checks for some memory errors.
 *
 * USAGE:  void  *MEMMemset(void *dest, int c, unsigned int count, char *tag, char *routine_name, char *cfile_name, int line_number);
 *
 * RETURNS: same as memset()
 * 
 *
 * DESCRIPTION:	This function provides a wrapper around memset which is called only if
 * MEMTRACK is defined.  Furthermore, checks memtrack file for previous allocation and
 * checks sizes for fit.  Warns if FILL_CHAR is first character of ct, if address of
 * cs cannot be found in memory log, or if given allocated size is less than size of ct.
 *              
 * Logs a warning if:
 * 1) memory allocation of buffer is not tracked in log file
 * 2) count > allocation size of buffer (if tracked)
 *
 * AUTHOR:  Adapted from an article in the August 1990 issue of Dr. Dobbs
 *          Journal (Copyright (c) 1990, Cornerstone Systems Group, Inc.)
 *          by Mark A. Ohrenschall, NGDC, (303) 497 - 6124, mao@ngdc.noaa.gov
 *
 * COMMENTS:This function is only called if MEMTRACK is defined.  If environmental
 * variable MEMTRACK is not defined then memory log tracking is disabled.
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 * KEYWORDS:
 *
 * ERRORS:
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "MEMMemset" 

#ifdef MEMTRACK_B
void *MEMMemset(void *dest, int c, unsigned int count)
#else
void *MEMMemset(void *dest, int c, unsigned int count, char *tag, char *routine_name, char *cfile_name, int line_number)
#endif
{
#ifndef MEMTRACK_B
	char msg[MAX_LTH];
	int error;

	FF_MEM_ENTRY entry;
	FF_MEM_LOG   log;

	entry.state = ALLOC;

	entry.addr = (long)dest;
	entry.size = 0;

	entry.tag = tag;
	entry.routine = routine_name;
	entry.file = cfile_name;
	entry.line = line_number;

	log.entry = &entry;

	error = MEMTrack_find_entry(&log);
	if (!error && !log.entry->size)
	{
		sprintf(msg,"setting %lu bytes in untracked buffer:%lx (%s)", (unsigned long)count, (long)dest, tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
	
	if (!error && log.entry->size && log.entry->size < count) {
		sprintf(msg,"setting %lu bytes in %lu byte buffer (%s)", (unsigned long)count, (unsigned long)log.entry->size, tag);
		MEMTrack_msg(msg, routine_name, cfile_name, line_number);
	}
#endif
	
	return(memset(dest, c, count));
}

/*****************************************************************************
 * NAME: set_posts
 *
 * PURPOSE:  Set post markers at head and tail of memory block
 *
 * USAGE:
 *
 * RETURNS: void
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

static void set_posts(FF_MEM_ENTRY_PTR entry)
{
	memcpy((char *)entry->addr + entry->size, POSTPOST, ONE_POST_SIZE);
	memcpy((char *)entry->addr - ONE_POST_SIZE, PREPOST, ONE_POST_SIZE);
}

/*****************************************************************************
 * NAME: check_posts
 *
 * PURPOSE: Check post markers at head and tail of memory block
 *
 * USAGE:
 *
 * RETURNS:  TRUE if posts have unexpected value, FALSE if not
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

static int check_posts(FF_MEM_ENTRY_PTR entry)
{
	if (memcmp((char *)entry->addr + entry->size, POSTPOST, ONE_POST_SIZE))
		return(TRUE);
	else if (memcmp((char *)entry->addr - ONE_POST_SIZE, PREPOST, ONE_POST_SIZE))
		return(TRUE);
	else
		return(FALSE);
}

static int heap_check(FF_MEM_LOG_PTR log)
{
#ifndef MEMTRACK_B
	char  line[MAX_LTH];
#endif
	int error;

	error = MEMTrack_fp(log);
	if (!error && log->file)
	{
		FF_MEM_ENTRY file_entry;

		file_entry.state = ALLOC;

#ifdef MEMTRACK_B
		while (fread(&file_entry, sizeof(file_entry), 1, log->file))
#else
		while (fgets(line,sizeof(line),log->file))
#endif
		{
#ifdef MEMTRACK_B
			if (ALLOC != file_entry.state)
				continue;
#else
			if (line[0] != ALLOC)
				continue;

			sscanf(line,"%*c %lx %lu", &file_entry.addr, &file_entry.size);
#endif

			if (check_posts(&file_entry))
			{
#ifndef MEMTRACK_B
				char msg[MAX_LTH];

				sprintf(msg, "Memory corrupted: %lx (detected at: \"%s\")", file_entry.addr, log->entry->tag);
				MEMTrack_msg(msg, log->entry->routine, log->entry->file, log->entry->line);
#endif
				error = TRUE;
				break;
			}
		}

		fclose(log->file);
	}
	else if (error && !log->file)
		error = 0;
	
	if (error)
		FF_DIE

	return(error);
}

static int leak_test(FF_MEM_LOG_PTR log)
{
	int error;
	BOOLEAN leak_found = FALSE;

	error = MEMTrack_fp(log);
	if (!error && log->file)
	{
#ifdef MEMTRACK_B
		FF_MEM_ENTRY file_entry;

		while (fread(&file_entry, sizeof(file_entry), 1, log->file))
#else
		char  line[MAX_LTH];

		while (fgets(line,sizeof(line),log->file))
#endif
		{
#ifdef MEMTRACK_B
			if (ALLOC != file_entry.state)
#else
			if (ALLOC != line[0])
#endif
				continue;

#ifndef MEMTRACK_B
			fprintf(stderr, "%s", line);
#endif
			leak_found = TRUE;
			FF_DIE
		}

		fclose(log->file);
	}
	else if (error && !log->file)
		error = 0;
	
	if (leak_found)
		return(MEM_GENERAL);
	else
		return(0);
}

