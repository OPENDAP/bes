/*
 * FILENAME: os_utils.h
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

#ifndef OS_UTILS_H__
#define OS_UTILS_H__

#define UNIX_EOL_STRING "\n"
#define UNIX_EOL_LENGTH 1

#define MAC_EOL_STRING "\r"
#define MAC_EOL_LENGTH 1

#define DOS_EOL_STRING "\r\n"
#define DOS_EOL_LENGTH 2

#if FF_OS == FF_OS_UNIX

#define NATIVE_EOL_STRING UNIX_EOL_STRING
#define NATIVE_EOL_LENGTH UNIX_EOL_LENGTH

#endif /* #if FF_CC == FF_CC_UNIX */

#if FF_OS == FF_OS_MAC

#define NATIVE_EOL_STRING MAC_EOL_STRING
#define NATIVE_EOL_LENGTH MAC_EOL_LENGTH

#endif /* FF_CC == FF_CC_MACCW */

#if FF_OS == FF_OS_DOS

#define NATIVE_EOL_STRING DOS_EOL_STRING
#define NATIVE_EOL_LENGTH DOS_EOL_LENGTH

#endif /* FF_CC == FF_CC_MSVC1 || FF_CC == FF_CC_MSVC4 */

#if FF_CC == FF_CC_UNIX
#define osf_strcmp strcmp
#endif
  
#if FF_CC == FF_CC_MACCW
Handle PathNameFromFSSpec(FSSpecPtr myFSSPtr);
#define osf_strcmp os_strcmpi
#endif

#if FF_CC == FF_CC_MSVC1 || FF_CC == FF_CC_MSVC4
#define osf_strcmp os_strcmpi
#endif

#ifndef max /* maximum macro */
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef min /* minimum macro */
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef ROUND
/* usage: int_var = (int_var_type)ROUND(expr); -- expr should be floating point type */
#define ROUND(a) ((a) < 0 ? ceil((a) - 0.5 - DOUBLE_UP) : floor((a) + 0.5 + DOUBLE_UP))
#else
#error "ROUND macro is already defined -- contact support"
#endif

#ifndef TRUNC
#define TRUNC(a) ((a) < 0 ? ceil(a) : floor(a))
#else
#error "TRUNC macro is already defined -- contact support"
#endif

#define FF_STRLEN(a) ((a)?strlen(a):0)
#define ok_strlen(a) FF_STRLEN(a) /* phase this out */

#define FF_SUBSTRCMP(a,b) (((a)&&(b))?strncmp(a,b,min(FF_STRLEN(a),FF_STRLEN(b))):1)

#define OS_INVERSE_ESCAPE 0
#define OS_NORMAL_ESCAPE 1

#define UNION_EOL_CHARS "\x0a\x0d"

#define WHITESPACE "\x09\x0a\x0b\x0c\x0d\x20"
#define LINESPACE "\x09\x0b\x0c\x20"

/* casts below are to ensure logical, rather than arithmetic, bit shifts */
#define FLIP_4_BYTES(a)	(	(((a) & 0x000000FFu) << 24) | \
							(((a) & 0x0000FF00u) <<  8) | \
							(((unsigned long)(a) & 0x00FF0000u) >>  8) | \
							(((unsigned long)(a) & 0xFF000000u) >> 24) )

#define FLIP_2_BYTES(a)	( (((unsigned short)(a) & 0xFF00u) >> 8) | \
                          (((a) & 0x00FFu) << 8) )

/* prototypes for functions in os_utils.c */

#ifndef _BOOLEAN_DEFINED
#define _BOOLEAN_DEFINED

#ifndef _WINNT_ /* specific to MSVC++ 4.0 */
typedef short BOOLEAN; /* Boolean type */
#endif
#endif /* _BOOLEAN_DEFINED 3/25/99 jhrg */

#ifdef TRUE
#undef TRUE
#endif
#define TRUE 1
#ifdef FALSE
#undef FALSE
#endif
#define FALSE 0

char *os_strlwr(char *string);
char *os_strupr(char *);
unsigned long os_filelength(char *filename);
BOOLEAN os_file_exist(char *filename);
int os_strcmpi(const char* s1, const char* s2);
int os_strncmpi(const char* s1, const char* s2, size_t n);

#if FF_OS == FF_OS_MAC
void *os_mac_load_env(char * buffer); /* -rf02,-rf03 */
#endif

char *os_get_env(char *variable_name);

int os_path_cmp_paths(char *s, char *t);
BOOLEAN os_path_is_native(char *path);
char *os_path_make_native(char *native_path, char *path);
void os_path_find_parts(char *path, char **pathname, char **filename, char **fileext);
void os_path_find_parent(char *path, char **parentdir);
char *os_path_return_ext(char *pfname);
char *os_path_return_name(char *pfname);
char *os_path_return_path(char *pfname);
void os_path_get_parts(char *path, char *pathname, char *filename, char *fileext);
char *os_path_put_parts(char *fullpath, char *dirpath, char *filename, char *fileext);
void os_str_replace_char(char *string, char oldc, char newc);
BOOLEAN os_path_prepend_special(char *in_name, char *home_path, char *out_name);
char *os_str_trim_whitespace(char *dest, char *source);
char *os_str_trim_linespace(char *line);
void os_str_replace_unescaped_char1_with_char2(char char1, char char2, char *str);
void os_str_replace_escaped_char1_with_char2(const char escape, char char1, char char2, char *str);

char *os_strdup(char *);
char *os_strrstr(const char *s1, const char *s2);
#endif /* (NOT) OS_UTILS_H__ */

