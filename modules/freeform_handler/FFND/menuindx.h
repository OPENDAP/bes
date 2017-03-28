/*
 * FILENAME:  menuindx.h
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

#ifndef MENU_INDEX_FILE
/* Restrictions placed on menu files by all mn_ functions:
 *
 * Maximum line length:              490 bytes
 * Maximum length of section titles: 200 bytes
 * EOL characters:                   unix, mac, or PC, 
 *                                   but must be uniform throughout the file.
 * Maximum number of sections:       32,700
 * Maximum menu file size:           2,000,000,000 bytes
 * Maximum section size:             64000 bytes
 *
 * Restrictions inside mn_ functions:
 *
 * Minimum max_buffer_size:          1000 bytes
 * Maximum size of buffers created:  MENU_INDEX_PTR->max_buffer_size
 *
 */ 

#if 0 && !(defined(FREEFORM) && defined(XVT))
#include <assert.h>
#endif

#ifdef MEN2HTML
#include <m2herror.h>
#endif

/* Define the ROW_SIZES structure, if it has not already been defined */
/**********************************************************************/
/** NOTE: THIS STRUCTURE IS ALSO DEFINED IN DATAVIEW.H!!!!!!!!!!!!!! **/ 
/**********************************************************************/
#ifndef ROWSIZE_STRUCT_DEFINED
#define ROWSIZE_STRUCT_DEFINED
typedef	struct {
		long start;
		long num_bytes;
}ROW_SIZES, *ROW_SIZES_PTR;
#endif

#define ulong unsigned long

/* menu related structures: */
typedef struct menu_index_struct {
	void  *check_address;
	char  *menu_file;       /* path and file name */
	char  *file_eol_str;    /* EOL sequence for the menu file */
	char  index_origion;    /* boolean for memory/file index */
	char  correct_eol_len;  /* boolean for correction of EOL length */
	char  index_eol_len;    /* end-of-line length for index */
	char  menu_in_mem;      /* boolean for file/memory created menu */
    char  binary_menu;		/* boolean for binary menu 
    							(if BINARY_MENU section exists) */
	ulong lines_in_index;   /* (file index) number of lines in index */
	ulong bytes_in_index;   /* (file index) number of bytes in index */
	ulong lines_in_menu;    /* number of lines in menu, total */
	ulong bytes_in_menu;    /* number of bytes in menu, total */
	ulong max_buffer_size;  /* maximum buffer size */
	short file_eollen;      /* length of EOL sequence for the menu file */
	char  index_corrupt;    /* file index exists, but is corrupt */
	char  file_index_exists;/* boolean (TRUE if file index exists) */
	char  **index_buffer;   /* array of buffer pointers */
	ulong *buffer_length;   /* actual length of buffers array */
	short num_buffers;      /* the number of buffers required */
	short num_sections;     /* the number of sections in the menu */
	char  *menu_name;       /* the name of the menu 
								(as in a MENU_NAME section) */
	char  *text_content;	/* the text content of the menu 
								(as in the TEXT_CONTENT section) */
	char  *data_path_one;   /* the path to prepend to 
								documentation files 1st */
	char  *data_path_two;   /* the path to prepend to 
								documentation files 2nd */
} MENU_INDEX, *MENU_INDEX_PTR;

/* Please note that this struct, while designed as a DLL, IS NOT COMPATIBLE
 * WITH THE FREEFORM DLLs defined in adtlib.  DO NOT USE THE DLL_ MACROS with
 * these structures. This is a non-circular DLL, and various reasons exist
 * which justify not using the DLL functions on this structure. */
typedef struct menu_selection_dll_struct {
	void *check_address;
	struct menu_selection_dll_struct *next_selection;
	struct menu_selection_dll_struct *previous_selection;
	char *user_display;    /* The element to display on the user interface */
	char *next_event;      /* file name or new section name */
	char *additional_info; /* additional information for a file (HDF+...) */
	char help_exists;      /* boolean; true if help exists for selection */
	char *help_title;      /* the title of the help section (if exists) */
	ROW_SIZES_PTR help_sec;/* rowsize struct for help section (or NULL) */
	char selection_type;   /* The type of the selection */
	char ascii_data_file;  /* true if $ was encountered */
	short selection_num;   /* The number of the selection (i.e. 3rd of 5) */
	void *extra_info;      /* An extra information pointer 
								(In GeoVu, a MENU_OPEN struct */
} MENU_SELECTION, *MENU_SELECTION_PTR;

typedef struct menu_section_struct {
	void *check_address;
	MENU_INDEX_PTR mindex;  /* a pointer to the menu index */
	char *title;			/* the title of the section (may not occur in menu file) */
	char *section;			/* the section title as recorded in the menu file */
	char *parent;           /* the parent section (NULL if none) */
	char *next;             /* if section_type=help, section to display next */
	char section_type;      /* the type of the section (help or selection) */
	char help_exists;       /* boolean; true if help for section exists */
	char dynamic_section;   /* boolean; true if section was dynamically made */
	char *help_title;       /* the title of the help section (NULL if none) */
	ROW_SIZES_PTR help_sec; /* rowsize struct for the help section (or NULL)*/
	char *display;          /* the buffer to display (if section_type==help)*/
	int  num_choices;       /* The number of MENU_SELECTION dll members */
	MENU_SELECTION_PTR selection; /* Ptr to the first MENU_SELECTION dll 
					member (if section_type==selection)*/
	void *extra_info;       /* For use by whatever function calls us, for anything 
								(In GeoVu, a MENU_PLACEHOLDER struct) */
} MENU_SECTION, *MENU_SECTION_PTR;

/* If we are inside GeoVu, define the MENU_OPEN struct here */
#ifdef XVT

/* Define the menu_open struct */
typedef struct menu_open_struct {
	MENU_INDEX_PTR mindex;		/* A pointer to the menu index
									(if it exists, NULL if not) */
	int menu_navigation_count;	/* The number of sections of this menu
									open under navigation (This is used
									internally only in GETMENU.C and has 
									no significance outside of that module */
	int usage_count; 			/* The number of data bins open with this 
									menu (might need help) */
	char *path_to_data;			/* The path to data (if it exists) */
	char *menu_file_name;		/* The menu file (with path)*/
} MENU_OPEN, *MENU_OPEN_PTR;

#endif

/* Section type defines */
#define MENU_SECTION_TYPE_SELECTION 0
#define MENU_SECTION_TYPE_HELP      1

/* Directive type defines */
#define MENU_DIRECTIVE_NEW_SECTION   0
#define MENU_DIRECTIVE_TERM_SECTION  1
#define MENU_DIRECTIVE_NEW_MENU_FILE 2
#define MENU_DIRECTIVE_DATA_FILE     3

/* Menu index origion defines */
#define MENU_INDEX_FILE 0
#define MENU_INDEX_MEM 1

/* Menu origion defines */
#define MENU_IN_FILE 0
#define MENU_IN_MEM 1

/* the menu directory seperator is a backslash by standard */
#define MENU_DIR_SEP '\\'

/* Define the default menu name */
#define MENU_UNTITLED "Untitled"

/* Define the default text content */
#define MENU_TEXT_CONTENT "ASCII"

/* Define menu data section title extension */
#define MENU_DATA_SECTION "_DATASEC"

/* Define menu index section title and length */
#define MENU_INDEX_SECTION "*MENU INDEX"
#define MENU_INDEX_SECTION_LEN 11

/* Define menu text content section */
#define MENU_TEXT_CONTENT_SEC "TEXT_CONTENT"

#define MENU_FOPEN_R "rb"
#define MENU_FOPEN_W "wb"
#define MENU_FOPEN_U "r+b"

#ifdef SUNCC
#include <unistd.h>
#endif
 
/* The MENU_ERR_PUSH define is for greater flexibility in error handling;
 * these lines make sure that MENU_ERR_PUSH defaults to err_push.
 * 
 * Because MEN2HTML uses the menu functions, and must send errors to 
 * stdout (instead of stderr) and in HTML format, this macro is necessary.
 */
#ifndef MENU_ERR_PUSH
#define MENU_ERR_PUSH(a, b, c) err_push(b, c)
#endif

/* Function declarations for mn_index functions: */
MENU_INDEX_PTR mn_index_make (char *filename, ulong max_buf_size, char *outfilename);
int mn_index_remove(char *filename, char *outfilename);
int mn_index_get_offset(MENU_INDEX_PTR mindex, char *section, ROW_SIZES_PTR rowsize);
int mn_index_free(MENU_INDEX_PTR mindex);
int mn_index_find_title(MENU_INDEX_PTR mindex, char *postfix, ROW_SIZES_PTR rowsize, char **buffer);
int mn_index_set_paths(MENU_INDEX_PTR mindex, char *pathone, char *pathtwo);
char *mn_binary_fgets(char *string, int n, FILE *stream, char *file_eol_str);
char *mn_get_file_eol_str(char *filename);

/* Function declarations for other mn_ functions: */
int mn_sec_titles_to_buf(MENU_INDEX_PTR mindex, char *buf_to_use, int *num_sections, char **buf_filled);
int mn_section_get(MENU_INDEX_PTR mindex, char *buf_to_use, ROW_SIZES_PTR rowsize, char **buf_filled);
int mn_help_sec_find(MENU_INDEX_PTR mindex, char *lookup, ROW_SIZES_PTR rowsize, char *section_name);
int mn_sec_process(MENU_INDEX_PTR mindex, char *section_title, ROW_SIZES_PTR rowsize, char *parent_menu, MENU_SECTION_PTR *menu_sec);
int mn_help_sec_get(MENU_INDEX_PTR mindex, char *lookup, ROW_SIZES_PTR rowsize, char **buf_filled);
int mn_proc_section_free(MENU_SECTION_PTR menu_sec);
int mn_selection_free(MENU_SELECTION_PTR selection);
int mn_datasec_len_get(MENU_INDEX_PTR mindex, ROW_SIZES_PTR rowsize, long *length);
int mn_section_rebuild(MENU_SECTION_PTR msection, char **buffer);
MENU_SELECTION_PTR mn_selection_copy(MENU_SELECTION_PTR selection);




/* Macros for navigating the MENU_SELECTION dll */
#define MENU_NEXT_SELECTION(a) ((a)->next_selection)
#define MENU_PREV_SELECTION(a) ((a)->previous_selection)
#define MENU_REWIND_SELECTION_DLL(a) {while((a)->previous_selection) (a)=(a)->previous_selection;}

/* This macro removes and frees the MENU_SELECTION_PTR 'rmv' */
#define MENU_REMOVE_SELECTION(rmv, newptr) {                               \
	newptr = NULL;                                                         \
	if(rmv->previous_selection){                                           \
		rmv->previous_selection->next_selection = rmv->next_selection;     \
		newptr = rmv->previous_selection;                                  \
	}                                                                      \
	if(rmv->next_selection){                                               \
		rmv->next_selection->previous_selection = rmv->previous_selection; \
		newptr = rmv->next_selection;                                      \
	}                                                                      \
	mn_selection_free(rmv);                                                \
	rmv = NULL; }

/* This macro inserts the MENU_SELECTION_PTR 'ins' into 'list' */
#define MENU_INSERT_SELECTION(ins, list) {                                 \
	if(list){                                                              \
		ins->previous_selection = list;                                    \
		ins->next_selection = list->next_selection;                        \
		list->next_selection = ins;                                        \
		if(ins->next_selection)                                            \
			ins->next_selection->previous_selection = ins;                 \
	}                                                                      \
	else                                                                   \
		list = ins;}

#endif

#ifdef MEMTRAP
/* Macros for memory testing */
#define mAlloc(sz) mn_malloc(sz, __LINE__, ROUTINE_NAME)
#define reAlloc(mptr, sz) mn_realloc(mptr, sz, __LINE__, ROUTINE_NAME)
#define fRee(sz) mn_free(sz, __LINE__, ROUTINE_NAME)

void *mn_malloc(size_t memsize, int linenum, char *routine);
void *mn_realloc(void *memblk, size_t memsize, int linenum, char *routine);
void mn_free(void *memblk, int linenum, char *routine);

#define MEMTRAPFILE "c:\\memtrap"

#else

#define mAlloc(sz) memMalloc(sz, "menu")
#define reAlloc(mptr, sz) memRealloc(mptr, sz, "menu")
#define fRee(sz) memFree(sz, "menu")
#endif

#ifdef NO_FF 

/* We are NOT including freeform, so we need to define our very own 
 * strnstr function */
char *mn_strnstr(char *pcPattern, char *pcText, size_t uTextLen);
#define MN_STRNSTR(strpattern, strtext, textsize) mn_strnstr(strpattern, strtext, textsize)

#else

/* We are including freeform, use ff_strnstr */
#define MN_STRNSTR(strpattern, strtext, textsize) ff_strnstr(strpattern, strtext, textsize)

#endif
