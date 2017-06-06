/*
 * FILENAME: memtrack.h
 *
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


#ifndef MEMTRACK_H__
#define MEMTRACK_H__

#ifndef FF_CC
#error "Are you including freeform.h? -- a compiler type must be set!"
#endif

#define MEMTRACK_LOG "memtrack.log"

#define MEMTRACK_OFF   0
#define MEMTRACK_BASIC 1
#define MEMTRACK_TRACE 2
#define MEMTRACK_ULTRA 3

typedef struct memtrack_entry_struct_t
{
#ifndef MEMTRACK_B
	char *tag;
	char *routine;
	char *file;
	int line;
#endif

	long addr;
	unsigned long size;
	char state;
} FF_MEM_ENTRY, *FF_MEM_ENTRY_PTR, **FF_MEM_ENTRY_HANDLE;

typedef struct memtrack_log
{
	FILE *file;
	long pos;
	FF_MEM_ENTRY_PTR entry;
} FF_MEM_LOG, *FF_MEM_LOG_PTR, **FF_MEM_LOG_HANDLE;


#define NO_TAG "generic tag"

#ifndef ROUTINE_NAME
#define ROUTINE_NAME "unfilled () name"
#endif

#ifdef FIND_UNWRAPPED_MEM
#ifndef MEMTRACK
#define MEMTRACK MEMTRACK_ULTRA
#endif

#define calloc  I_should_be_memCalloc
#define free    I_should_be_memFree
#define malloc  I_should_be_memMalloc
#define realloc I_should_be_memRealloc
#define strdup  I_should_be_memStrdup

#if MEMTRACK == MEMTRACK_ULTRA

#define strcpy	I_should_be_memStrcpy
#define strncpy	I_should_be_memStrncpy
#define strcat	I_should_be_memStrcat
#define strncat	I_should_be_memStrncat
#define strcmp	I_should_be_memStrcmp
#define strncmp	I_should_be_memStrncmp
#define strchr	I_should_be_memStrchr
#define strrchr	I_should_be_memStrrchr
#define strstr	I_should_be_memStrstr
#define memcpy	I_should_be_memMemcpy
#define memmove	I_should_be_memMemmove
#define memset	I_should_be_memMemset

#endif /* MEMTRACK == MEMTRACK_ULTRA */

#endif  /* FIND_UNWRAPPED_MEM */

#if MEMTRACK >= MEMTRACK_BASIC

#ifdef MEMTRACK_B

#define memCalloc(NOBJ, SIZE, TAG)	MEMCalloc(NOBJ, SIZE)
#define memFree(P, TAG)	MEMFree(P)
#define memMalloc(SIZE, TAG)	MEMMalloc(SIZE)
#define memRealloc(P, SIZE, TAG)	MEMRealloc(P, SIZE)
#define memStrdup(STRING, TAG)	MEMStrdup(STRING)
#define memExit(STATUS, TAG)	MEMExit(STATUS)

void *MEMCalloc(size_t nobj, size_t size);
void  MEMFree(void *p);
void *MEMMalloc(size_t size);
void *MEMRealloc(void *p, size_t size);
char *MEMStrdup(const char *string);
void MEMExit(int status);

#else

#define memCalloc(NOBJ, SIZE, TAG)	MEMCalloc(NOBJ, SIZE, TAG, ROUTINE_NAME, __FILE__, __LINE__)
#define memFree(P, TAG)	MEMFree(P, TAG, ROUTINE_NAME, __FILE__, __LINE__)
#define memMalloc(SIZE, TAG)	MEMMalloc(SIZE, TAG, ROUTINE_NAME, __FILE__, __LINE__)
#define memRealloc(P, SIZE, TAG)	MEMRealloc(P, SIZE, TAG, ROUTINE_NAME, __FILE__, __LINE__)
#define memStrdup(STRING, TAG)	MEMStrdup(STRING, TAG, ROUTINE_NAME, __FILE__, __LINE__)
#define memExit(STATUS, TAG)	MEMExit(STATUS, TAG, ROUTINE_NAME, __FILE__, __LINE__)

void *MEMCalloc(size_t nobj, size_t size, char *tag, char *routine_name, char *cfile_name, int line_number);
void  MEMFree(void *p, char *tag, char *routine_name, char *cfile_name, int line_number);
void *MEMMalloc(size_t size, char *tag, char *routine_name, char *cfile_name, int line_number);
void *MEMRealloc(void *p, size_t size, char *tag, char *routine_name, char *cfile_name, int line_number);
char *MEMStrdup(const char *string, char *tag, char *routine_name, char *cfile_name, int line_number);
void MEMExit(int status, char *tag, char *routine_name, char *cfile_name, int line_number);

#endif /* (else) MEMTRACK_B */

#else /* MEMTRACK >= MEMTRACK_BASIC */

#define memCalloc(NOBJ, SIZE, TAG)      calloc(NOBJ, SIZE)

#ifdef MEMSAFREE
#define memFree(P, TAG) {free(P);P = NULL;}
#else
#define memFree(P, TAG) free(P)
#endif /* MEMSAFREE */

#define memMalloc(SIZE, TAG)    malloc(SIZE)
#define memRealloc(P, SIZE, TAG)        realloc(P, SIZE)
#define memStrdup(STRING, TAG)  os_strdup(STRING)
#define memExit(STATUS, TAG)    exit(STATUS)

#endif /* MEMTRACK >= MEMTRACK_BASIC */

#if MEMTRACK >= MEMTRACK_TRACE

#ifdef MEMTRACK_B
#define memTrace(TAG)           MEMTrace()
void MEMTrace(void);
#else
#define memTrace(MSG) MEMTrace(MSG, ROUTINE_NAME, __FILE__, __LINE__);
void MEMTrace(char *msg, char *routine_name, char *cfile_name, int line_number);
#endif /* MEMTRACK_B */

#else
#define memTrace(MSG) ;
#endif /* MEMTRACK >= MEMTRACK_TRACE */

#if MEMTRACK == MEMTRACK_ULTRA

#define memStrcpy(S, CT, TAG)	MEMStrcpy(S, CT, TAG, ROUTINE_NAME, __FILE__, __LINE__)
#define memStrncpy(S, CT, N, TAG)	MEMStrncpy(S, CT, N, TAG, ROUTINE_NAME, __FILE__, __LINE__)
#define memStrcat(S, CT, TAG)	MEMStrcat(S, CT, TAG, ROUTINE_NAME, __FILE__, __LINE__)
#define memStrncat(S, CT, N, TAG)	MEMStrncat(S, CT, N, TAG, ROUTINE_NAME, __FILE__, __LINE__)
#define memStrcmp(CS, CT, TAG)	MEMStrcmp(CS, CT, TAG, ROUTINE_NAME, __FILE__, __LINE__)
#define memStrncmp(CS, CT, N, TAG)	MEMStrncmp(CS, CT, N, TAG, ROUTINE_NAME, __FILE__, __LINE__)
#define memStrchr(CS, C, TAG)	MEMStrchr(CS, C, TAG, ROUTINE_NAME, __FILE__, __LINE__)
#define memStrrchr(CS, C, TAG)	MEMStrrchr(CS, C, TAG, ROUTINE_NAME, __FILE__, __LINE__)
#define memStrstr(CS, CT, TAG)	MEMStrstr(CS, CT, TAG, ROUTINE_NAME, __FILE__, __LINE__)
#define memMemcpy(S, CT, N, TAG)	MEMMemcpy(S, CT, N, TAG, ROUTINE_NAME, __FILE__, __LINE__)
#define memMemmove(S, CT, N, TAG)	MEMMemmove(S, CT, N, TAG, ROUTINE_NAME, __FILE__, __LINE__)
#define memMemset(DEST, C, COUNT, TAG) MEMMemset(DEST, C, COUNT, TAG, ROUTINE_NAME, __FILE__, __LINE__)

#ifdef MEMTRACK_B
char *MEMStrcpy(char *s, const char *ct);
char *MEMStrncpy(char *s, const char *ct, size_t n);
char *MEMStrcat(char *s, const char *ct);
char *MEMStrncat(char *s, const char *ct, size_t n);
int   MEMStrcmp(const char *cs, const char *ct);
int   MEMStrncmp(const char *cs, const char *ct, size_t n);
char *MEMStrchr(const char *cs, int c);
char *MEMStrrchr(const char *cs, int c);
char *MEMStrstr(const char *cs, const char *ct);
void *MEMMemset(void *dest, int c, unsigned int count);

#if FF_CC == FF_CC_MSVC1 || FF_CC == FF_CC_MSVC4 || FF_CC == FF_CC_MACCW

void *MEMMemcpy(void *s, const void *ct, size_t n);
void *MEMMemmove(void *s, const void *ct, size_t n);

#endif

#if FF_CC == FF_CC_UNIX

char *MEMMemcpy(char *s, const char *ct, size_t n);
char *MEMMemmove(char *s, const char *ct, size_t n);

#endif

#else
char *MEMStrcpy(char *s, const char *ct, char *tag, char *routine_name, char *cfile_name, int line_number);
char *MEMStrncpy(char *s, const char *ct, size_t n, char *tag, char *routine_name, char *cfile_name, int line_number);
char *MEMStrcat(char *s, const char *ct, char *tag, char *routine_name, char *cfile_name, int line_number);
char *MEMStrncat(char *s, const char *ct, size_t n, char *tag, char *routine_name, char *cfile_name, int line_number);
int   MEMStrcmp(const char *cs, const char *ct, char *tag, char *routine_name, char *cfile_name, int line_number);
int   MEMStrncmp(const char *cs, const char *ct, size_t n, char *tag, char *routine_name, char *cfile_name, int line_number);
char *MEMStrchr(const char *cs, int c, char *tag, char *routine_name, char *cfile_name, int line_number);
char *MEMStrrchr(const char *cs, int c, char *tag, char *routine_name, char *cfile_name, int line_number);
char *MEMStrstr(const char *cs, const char *ct, char *tag, char *routine_name, char *cfile_name, int line_number);
void *MEMMemset(void *dest, int c, unsigned int count, char *tag, char *routine_name, char *cfile_name, int line_number);

#if FF_CC == FF_CC_MSVC1 || FF_CC == FF_CC_MSVC4 || FF_CC == FF_CC_MACCW

void *MEMMemcpy(void *s, const void *ct, size_t n, char *tag, char *routine_name, char *cfile_name, int line_number);
void *MEMMemmove(void *s, const void *ct, size_t n, char *tag, char *routine_name, char *cfile_name, int line_number);

#endif

#if FF_CC == FF_CC_UNIX

char *MEMMemcpy(char *s, const char *ct, size_t n, char *tag, char *routine_name, char *cfile_name, int line_number);
char *MEMMemmove(char *s, const char *ct, size_t n, char *tag, char *routine_name, char *cfile_name, int line_number);

#endif

#endif /* MEMTRACK_B */

#else /* MEMTRACK == MEMTRACK_ULTRA */

#define memStrcpy(S, CT, TAG)   strcpy(S, CT)
#define memStrncpy(S, CT, N, TAG)       strncpy(S, CT, N)
#define memStrcat(S, CT, TAG)   strcat(S, CT)
#define memStrncat(S, CT, N, TAG)       strncat(S, CT, N)
#define memStrcmp(CS, CT, TAG)  strcmp(CS, CT)
#define memStrncmp(CS, CT, N, TAG) strncmp(CS, CT, N)
#define memStrchr(CS, C, TAG)   strchr(CS, C)
#define memStrrchr(CS, C, TAG)  strrchr(CS, C)
#define memStrstr(CS, CT, TAG)  strstr(CS, CT)
#define memMemcpy(S, CT, N, TAG)        memcpy(S, CT, N)
#define memMemmove(S, CT, N, TAG)       memmove(S, CT, N)
#define memMemset(DEST,C,COUNT,TAG)	memset(DEST,C,COUNT)

#endif /* (#else) MEMTRACK == MEMTRACK_ULTRA */

#endif /* (NOT) MEMTRACK_H__ */

