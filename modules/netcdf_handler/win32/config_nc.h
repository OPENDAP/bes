#ifndef _config_nc_h
#define _config_nc_h

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define if you don't have vprintf but do have _doprnt.  */
/* #undef HAVE_DOPRNT */

/* Define if you have the vprintf function.  */
#define HAVE_VPRINTF 1

/* Define to `long' if <sys/types.h> doesn't define.  */
/* #undef off_t */

/* Define if you need to in order for stat and other things to work.  */
/* #undef _POSIX_SOURCE */

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef size_t */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* If defined, enable memory leak detection through libdbnew.a. Make sure to */
/* link with that library. */
/* #undef TRACE_NEW */

/* If defined, the DBG() macro defined in debug.h is activated. This macro */
/* is used for nominal program instrumentation */
/* #undef DEBUG */

/* If defined, the DBG2() macro defined in debug.h is activated. This macro */
/* is used for detailed program instrumentation. Anything that prints half a */
/* page or more of stuff every time it is executed should be inside DBG2(), */
/* not DBG(). */
/* #undef DEBUG2 */

/* Define if you have the strerror function.  */
#define HAVE_STRERROR 1

/* Define if you have the strtol function.  */
#define HAVE_STRTOL 1

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <limits.h> header file.  */
#define HAVE_LIMITS_H 1

/* Define if you have the <unistd.h> header file.  */
/* #define HAVE_UNISTD_H 1 */

/* Used by the NetCDF library */
/* #undef OLD_FILLVALUES */

/* The number of bytes in a char.  */
#define SIZEOF_CHAR 1

/* The number of bytes in a double.  */
#define SIZEOF_DOUBLE 8

/* The number of bytes in a float.  */
#define SIZEOF_FLOAT 4

/* The number of bytes in a int.  */
#define SIZEOF_INT 4

/* The number of bytes in a long.  */
#define SIZEOF_LONG 4

/* GNU gcc/g++ provides a way to mark variables, etc. as unused */

#if defined(__GNUG__) || defined(__GNUC__)
#define not_used __attribute__ ((unused))
#else
#define not_used 
#endif

#define DODS_SERVER_VERSION "nc-dods/3.4.3"

#endif /* _config_nc_h */


