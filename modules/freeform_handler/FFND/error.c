/* 
 * FILENAME: error.c
 *
 * CONTAINS: 
 * Public Functions:
 *
 * err_assert
 * err_clear() 
 * err_disp() 
 * err_pop
 * err_push() 
 * err_state() 
 *
 * Private Functions:
 *
 * err_bin_search
 * err_get_msg
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

#include <freeform.h>

#define NUM_ERROR_ENTRIES (sizeof(local_errlist) / sizeof(ERROR_RECORD))

/* Local variables */

typedef struct
{
	int   error_number;
	char *error_string;
} ERROR_RECORD;


static FF_ERROR_LIST error_list = NULL;
								
/* Local prototypes */
static char *err_get_msg(int);
static char *err_bin_search(int);

static ERROR_RECORD local_errlist[] = 
{
	/* NOTE: THESE ERRORS MAY BE CROSS-LISTED
		-- YOU MAY HAVE TO CHECK MORE THAN ONE SECTION TO FIND
		   THE APPROPRIATE ERROR DESCRIPTION */

	/* General Errors */
	{ERR_GENERAL,                    "Unable to complete requested command"},                 /*500*/
	{ERR_OPEN_FILE,                  "Opening file (file does not exist, or too many open files)"},                                 /*501*/
	{ERR_READ_FILE,                  "Reading file"},                                 /*502*/
	{ERR_WRITE_FILE,                 "Writing to file"},                              /*503*/
	{ERR_PTR_DEF,                    "Required internal data structure undefined"},                                  /*504*/
	{ERR_MEM_LACK,                   "Insufficient memory (RAM)"},                 /* 505 */
	{ERR_UNKNOWN,                    "Undefined {ERRor"},                                      /*506*/
	{ERR_FIND_FILE,                  "Finding file"},                                      /*507*/
	{ERR_FILE_DEFINED,               "File undefined"},                                     /*508*/
	{ERR_OUT_OF_RANGE,               "Value went out of range"},                     /*510*/
	{ERR_PROCESS_DATA,               "Processing data"},                              /*515*/
	{ERR_NUM_TOKENS,                 "Incorrect Number of Tokens on Line"},                    /* 519 */
	{ERR_FILE_EXISTS,                "File already exists (Can you delete or move the file?)"},   /* 522 */
	{ERR_CREATE_FILE,                "Creating file (Do you have write access to the file and directory?)"},    /* 523 */
	{ERR_WILL_OVERWRITE_FILE,        "File will be overwritten (Define \"nooverwrite\" keyword?)"},  /* 524 */
	{ERR_REMOVE_FILE,                "Removing file (is the file read-only?)"},                             /* 525 */
	{ERR_WONT_OVERWRITE_FILE,        "File already exists and will not be overwritten"},  /* 524 */

	/* Freeform {ERRors */
	{ERR_UNKNOWN_VAR_TYPE,           "Unknown variable type"},                                /*1000*/
	{ERR_UNKNOWN_PARAMETER,          "Could not get a value for parameter or keyword"},                  /*1001*/
	{ERR_CONVERT,                    "Problem in conversion"},                                /*1003*/
	{ERR_MAKE_FORM,                  "Making format"},                                /*1004*/
	{ERR_NO_VARIABLES,               "No variables found in format"},                                   /*1012*/
	{ERR_FIND_FORM,                  "Finding the named format"},                               /*1021*/
	{ERR_CONVERT_VAR,                "Could not determine type of conversion for the given variable"},           /*1022*/
	{ERR_ORPHAN_VAR,                 "Output variable has no relation to input"},                                /* 1023 */
	{ERR_POSSIBLE,                   "Possible problem when you next use output data"},

	/* Binview {ERRors */

	{ERR_SET_DBIN,                   "Setting data bin"},                             /*1509*/
	{ERR_PARTIAL_RECORD,             "Last record was incomplete and was not read"},      /*1517*/
	{ERR_NT_DEFINE,                  "Defining name table"},                          /*1520*/
	{ERR_UNKNOWN_FORMAT_TYPE,        "Unknown Format Type"},									/*1525*/
	
	/* EQUATION {ERRors */
	{ERR_EQN_SET,                 "Problem setting up equation variable"},

	{ERR_SDTS,          "Problem in SDTS conversion"},
	{ERR_SDTS_BYE_ATTR, "GeoVu keyword will not be encoded into the SDTS transfer"},
	
	/* String database and menu systems */

	{ERR_MAKE_MENU_DBASE,            "Creating menu database"},                       /*3000*/
	{ERR_NO_SUCH_SECTION,            "No Section with this title"},                           /*3001*/
	{ERR_GETTING_SECTION,            "Getting text for section"},                     /*3002*/
	{ERR_MENU,                       "Processing Menu"},                              /*3003*/

	{ERR_MN_BUFFER_TRUNCATED,        "Menu section buffer truncated"},                        /*3500*/
	{ERR_MN_SEC_NFOUND,              "Requested menu section not found"},                       /*3501*/
	{ERR_MN_FILE_CORRUPT,            "Menu file corrupt"},                                    /*3502*/
	{ERR_MN_REF_FILE_NFOUND,         "Referenced file not found"},                            /*3503*/
	
	/* Interpreter and parse {ERRors */

	{ERR_MISSING_TOKEN,              "Expected token(s) missing"},                               /* 4001 */
	{ERR_PARAM_VALUE,                "Invalid parameter value"},                              /* 4006 */
	{ERR_UNKNOWN_OPTION,             "Unknown option; for usage, run again without any arguments"}, /* 4013 */
	{ERR_IGNORED_OPTION,             "This option not used with this application"},             /* 4014 */
	{ERR_VARIABLE_DESC,              "Problem with variable description line"},               /* 4015 */
	{ERR_VARIABLE_SIZE,              "Incorrect field size for this variable"},         /* 4016 */
	{ERR_NO_EOL,                     "Expecting an End-Of-Line marker"},                        /* 4017 */

	/* ADTLIB {ERRors */

	{ERR_FIND_MAX_MIN,               "Finding max and min"},                          /* 6001 */
	{ERR_PARSE_EQN,                  "Parsing equation"},                             /* 6002 */
	{ERR_EE_VAR_NFOUND,              "Variable in equation not found"},                       /* 6003 */
	{ERR_CHAR_IN_EE,                 "Character data type in equation"},                      /* 6004 */
	{ERR_EE_DATA_TYPE,               "Mismatching data types in equation"},                   /* 6005 */
	{ERR_NDARRAY,                    "With N-dimensional array"},                     /* 6006 */
	{ERR_GEN_QUERY,                  "Processing query"},                             /* 6007 */
	
	/* NAME_TABLE {ERRors */

	{ERR_EXPECTING_SECTION_START, "Expecting Section Start:"},
	{ERR_EXPECTING_SECTION_END,   "Expecting Section End:"},
	{ERR_MISPLACED_SECTION_START, "Badly Placed Section Start:"},
	{ERR_MISPLACED_SECTION_END,   "Badly Placed Section End:"},
	{ERR_NT_MERGE,                "Error in merging/copying name tables"},
	{ERR_NT_KEYNOTDEF,            "Expected keyword not defined"},
	{ERR_EQV_CONTEXT,             "Definition(s) in equivalence section would be out of context"},

	{ERR_GEN_ARRAY, "Problem in performing array operation"},

	/* Programmer's eyes only, or programmer support */
	{ERR_API,                     "Error in Application Programmer Interface, contact support"},   /* 7900 */
	{ERR_SWITCH_DEFAULT,          "Unexpected default case in switch statement, contact support"}, /* 7901 */
	{ERR_ASSERT_FAILURE,          "Assertion Failure, contact support"},                                            /* 7902 */
	{ERR_NO_NAME_TABLE,           "Equivalence section has not been defined"},
	{ERR_API_BUF_LOCKED,          "API {ERRor -- internal buffer is already locked"},
	{ERR_API_BUF_NOT_LOCKED,      "API {ERRor -- internal buffer is not locked"}
};

#undef ROUTINE_NAME
#define ROUTINE_NAME "create_error"

static FF_ERROR_PTR create_error
	(
	 int code,
	 char *message
	)
{
	FF_ERROR_PTR error = NULL;

	error = (FF_ERROR_PTR)memMalloc(sizeof(FF_ERROR), "error");
	if (!error)
	{
		assert(error);
		return(NULL);
	}

#ifdef FF_CHK_ADDR
	error->check_address = error;
#endif

	error->code = code;
	error->message = (char *)memStrdup(message, "error->message");
	if (!error->message)
	{
		assert(error->message);
		memFree(error, "error");
		return(NULL);
	}

	os_str_replace_char(error->message, '\b', ':');

	error->problem = err_get_msg(code - (code > ERR_WARNING_ONLY ? ERR_WARNING_ONLY : 0));

	error->warning_ord = 0;
	error->error_ord   = 0;

	return(error);
}

#undef ROUTINE_NAME
#define ROUTINE_NAME "ff_destroy_error"

void ff_destroy_error(FF_ERROR_PTR error)
{
	assert(error);

	FF_VALIDATE(error);
#ifdef FF_CHK_ADDR
	error->check_address = NULL;
#endif

	error->code = 0;

	memFree(error->message, "error->message");
	error->message = NULL;

	error->problem = NULL;

	error->warning_ord = 0;
	error->error_ord   = 0;

	memFree(error, "error");
}

/*static jhrg 9/1/12*/
BOOLEAN is_a_warning
	(
	 FF_ERROR_PTR error
	)
{
	FF_VALIDATE(error);

	return((BOOLEAN)(error->code > ERR_WARNING_ONLY));
}

static void add_error
	(
	 int code,
	 char *message
	)
{
	DLL_NODE_PTR new_error_node = NULL;

	FF_ERROR_PTR error = create_error(code, message);

	if (error)
	{
		if (!error_list)
		{
			error_list = dll_init();
			if (!error_list)
			{
				assert(error_list);
				return;
			}
		}

		if (error_list)
		{
			FF_ERROR_PTR last_error = FF_EP(dll_last(error_list));

			new_error_node = dll_add(error_list);
			if (new_error_node)
			{
				dll_assign(error, DLL_ERR, new_error_node);
				if (is_a_warning(error))
				{
					error->error_ord = last_error ? last_error->error_ord : 0;
					error->warning_ord = 1 + (last_error ? last_error->warning_ord : 0);
				}
				else
				{
					error->error_ord = 1 + (last_error ? last_error->error_ord : 0);
					error->warning_ord = last_error ? last_error->warning_ord : 0;
				}
			}
			else
			{
				assert(new_error_node);
				ff_destroy_error(error);
			}
		}
	}
}

int err_count(void)
{
	FF_ERROR_PTR error = NULL;
	int total = 0;

	if (error_list)
	{
		error = FF_EP(dll_last(error_list));
		if (error)
			total = error->error_ord + error->warning_ord;
	}

	return total;
}

static FF_ERROR_PTR pop_error(void)
{
	FF_ERROR_PTR error = NULL;

	if (error_list)
	{
		error = FF_EP(dll_last(error_list));
		if (error)
			dll_delete_node(dll_last(error_list));

		if (!FF_EP(dll_first(error_list)))
		{
			dll_free_list(error_list);
			error_list = NULL;
		}
	}

	return(error);
}

/*static jhrg 9/11/12 */
FF_ERROR_PTR pull_error(void)
{
	FF_ERROR_PTR error = NULL;

	if (error_list)
	{
		error = FF_EP(dll_first(error_list));
		if (error)
			dll_delete_node(dll_first(error_list));

		if (!FF_EP(dll_first(error_list)))
		{
			dll_free_list(error_list);
			error_list = NULL;
		}
	}

	return(error);
}

int verr_push
	(
	 const int ercode,
	 const char *format,
	 va_list va_args
	)
{
	enum {MAX_ERRSTR_SIZE = 2 * MAX_PATH};

	char message[MAX_ERRSTR_SIZE]; /* big enough? */


	assert(ercode);
	assert(format);

	vsnprintf(message, MAX_ERRSTR_SIZE, format, va_args);

	add_error(ercode, message);

	return(ercode);
}

/*
 * NAME:    err_push 
 *
 * PURPOSE: To push an error message and a possible further
 *          error description onto stacks which may later 
 *          be displayed
 *
 * USAGE:   void err_push(int error_defined_constant,
 *                        PSTR further_error_description)
 *          further_error_description may be NULL 
 *
 * RETURNS: void
 *              
 *
 * DESCRIPTION: Three simultaneous array-based stacks are used 
 *              to describe an error. Memory is allocated for each
 *              routine name stack element (routine_stack) and for 
 *              each further error description stack element (str_stack).
 *              If the error stacks are full, a stack full message 
 *              is displayed and control is transferred back to the
 *              user in which he\she can display stack errors if
 *              desired. If a memory allocation error occurs, control
 *              is also given to the user with an appropriate explanation.
 *
 * ERR_DEBUG_MSG has a special usage.  It is both an error code and message
 * in its own right, but if added to another error code and sent into err_push()
 * it adds the meaning that this message should be viewed only in a "debugging
 * version", and never in a "release version".  In summary, for programmers'
 * and/or testers' eyes only.  To enable this feature, compile all source code
 * files with the preprocessor macro DEBUG_MSG defined.
 *
 * AUTHOR:  Mark Van Gorp, NGDC, (303)497-6221, mvg@kryton.ngdc.noaa.gov
 *
 * SYSTEM DEPENDENT FUNCTIONS: 
 *  Although no dependent function exists, the following error creating
 *  process must be followed before new errors can be pushed:
 *
 * ALL NEW ERROR ENTRIES MUST START AT INTEGERS > 500. FOR EXAMPLE, A NEW                   
 * BLOCK OF ERROR MESSAGES MAY START AT 1000; THE NEXT BLOCK AT 1500; ETC.      
 * REASON--- ERROR MESSAGES < 500 MAY INTERFERE WITH SYSTEM ERRORS.                                 
 * HOW TO ADD ERROR MESSAGES:                                                                                                       
 *                                                                                                                                                               
 * 1. #define a numeric error descriptive constant (Greater than 500) in err.h                                                                                                                              
 *                                                                                                                                                               
 * 2. Add your defined constant, along with an error message describing it in
 *    local_errlist.                                                               
 *
 * 3. Adding error messages have ONE STIPULATION: They must be
 *    inserted into local_errlist in increasing numeric order of
 *    the constants. (ie 500, 505, 603, ...,n; NOT 500, 603, 505,...n)
 *    This will enable the binary search to work effectively and find
 *    the appropriate error message.
 *
 * To distinguish between an error seen in a debug version versus a release
 * versions the following option is provided:
 * 
 *    Within the err_push() call, add ERR_DEBUG_MSG to the error code, e.g.,
 *    err_push(ERR_MY_ERROR + ERR_DEBUG_MSG, some_explanation_str);
 *    This means that in this particular call to err_push(), the debug version
 *    will display ERR_MY_ERROR, but in the release version it will not be
 *    displayed.
 *
 * COMMENTS:
 *
 * KEYWORDS:            error system 
 *
 */

int err_push
	(
	 const int ercode,
	 const char *format,
	 ...
	)
{
	va_list va_args;

	assert(ercode);
	assert(format);

	va_start(va_args, format);
	
	verr_push(ercode, format, va_args);

	va_end(va_args);

	return(ercode);
}

/*
 * NAME:    err_state 
 *
 * PURPOSE: To test if any error mesages have been pushed onto
 *          the error stack
 *
 * USAGE:   ERR_BOOLEAN err_state()
 *
 * RETURNS: TRUE (1) if errors exist; otherwise FALSE (0)
 *
 * DESCRIPTION: Err_state() is a boolean function which evaluates to 
 *              TRUE if errors exist on the stack. Stack_index keeps
 *              track of the stack head and is zero if no errors
 *              have been pushed.                                                               
 *
 * AUTHOR:  Mark Van Gorp, NGDC, (303)497-6221, mvg@kryton.ngdc.noaa.gov
 *
 * SYSTEM DEPENDENT FUNCTIONS:  None
 *
 * COMMENTS:
 *
 * KEYWORDS:            error system 
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "err_state"

ERR_BOOLEAN err_state(void)
{
	if (error_list && FF_EP(dll_first(error_list)))
		return(TRUE);
	else
		return(FALSE);
}

int err_pop(void)
{
	int ercode = 0;
	FF_ERROR_PTR error = NULL;

	error = pop_error();
	if (error)
	{
		FF_VALIDATE(error);

		ercode = error->code;

		ff_destroy_error(error);
	}
	
	return(ercode);
}

/*
 * NAME:    err_clear 
 *
 * PURPOSE: To clear the error message stacks
 *
 * USAGE:   void err_clear()
 *
 * RETURNS: void
 *
 * DESCRIPTION: Clears the string-based stacks (routine_stack and str_stack)
 *              and deallocates the memory. Clear_stack_index denotes the 
 *              number of array elements used on the stack. Err_clear() is
 *              generally called only by err_disp().
 *
 * AUTHOR:  Mark Van Gorp, NGDC, (303)497-6221, mvg@kryton.ngdc.noaa.gov
 *
 * SYSTEM DEPENDENT FUNCTIONS:  None
 *
 * COMMENTS:
 *
 * KEYWORDS:            error system 
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "err_clear"

void err_clear(void)
{
	if (error_list)
	{
		dll_free_holdings(error_list);
		error_list = NULL;
	}
}

static void print_error
	(
	 FILE *fp,
	 FF_ERROR_PTR error,
	 BOOLEAN error_logging,
	 BOOLEAN error_screen
	)
{
	FF_VALIDATE(error);

	if (error_logging)
	{
		fprintf(fp, "\n%s %d: %s", is_a_warning(error) ? "WARNING" : "ERROR", is_a_warning(error) ? error->warning_ord : error->error_ord, error->problem);
		fprintf(fp, "\nEXPLANATION: %s\n", error->message);
	}

	if (error_screen)
	{
		fprintf(stderr, "\n%s %d: %s", is_a_warning(error) ? "WARNING" : "ERROR", is_a_warning(error) ? error->warning_ord : error->error_ord, error->problem);
		fprintf(stderr, "\nEXPLANATION: %s\n", error->message);
	}
}

/*
 * NAME:    err_disp 
 *
 * PURPOSE: To display the pushed stack errors
 *
 * USAGE:   void err_disp()
 *
 * RETURNS: The lowest error value pushed on the stack
 *
 * DESCRIPTION: Err_disp() displays the top error message from the error
 *              stack and queries the user if he\she would like to see more
 *              error messages (if the stack is not empty). If the user
 *              wishes to see more errors, a 'y' or 'Y' must be entered and
 *              the loop is repeated. When the stack is empty or no more 
 *              error messages are needed, err_clear() is called to clear
 *              the stacks. 
 *
 * AUTHOR:  Mark Van Gorp, NGDC, (303)497-6221, mvg@kryton.ngdc.noaa.gov
 *
 * SYSTEM DEPENDENT FUNCTIONS:  isatty, fileno
 *
 * COMMENTS:
 *
 * KEYWORDS:            error system 
 *
 */

#undef ROUTINE_NAME
#define ROUTINE_NAME "err_disp"
int err_disp(FF_STD_ARGS_PTR std_args)
{
	int error_return = INT_MAX;

	char reply[4]; /* even for alignment/anal purposes */

	int num_warnings = 0;
	int num_errors = 0;

	char num_warnings_str[10];
	char num_errors_str[10];

	BOOLEAN user_interactive = FALSE;
	BOOLEAN error_logging = FALSE;
	BOOLEAN error_screen = FALSE;

	FILE *fp = NULL;
	FF_ERROR_PTR error = NULL;

	if (std_args)
		FF_VALIDATE(std_args);

	if (!error_list)
		return 0;

	num_warnings = ((FF_ERROR_PTR)(FF_EP(dll_last(error_list))))->warning_ord;
	num_errors = ((FF_ERROR_PTR)(FF_EP(dll_last(error_list))))->error_ord;

	error = pull_error();
	if (error)
		error_return = min(error_return, error->code);
	else
		return 0;

	if (std_args && std_args->error_log)
	{
		fp = fopen(std_args->error_log, "at");
		if (!fp)
		{
			error_logging = FALSE;
			fprintf(stderr, "Cannot open %s to log errors!!!\n", std_args->error_log);
		}
		else
			error_logging = TRUE;
	}

	if ((std_args && std_args->error_prompt == FALSE) || (std_args && std_args->log_file))
		user_interactive = FALSE;
	else if (isatty(fileno(stdin))) /* stdin is not redirected */
		user_interactive = TRUE;
	
	if (!std_args || !std_args->log_file)
		error_screen = TRUE;

	if (num_warnings)
		snprintf(num_warnings_str, 10, "%d", num_warnings);
	else
		snprintf(num_warnings_str, 10, "no");

	if (num_errors)
		snprintf(num_errors_str, 10, "%d", num_errors);
	else
		snprintf(num_errors_str, 10, "no");

	if (num_warnings && num_errors)
	{
		if (error_logging)
		{
			fprintf(fp, "\nThere %s %s warning%s and %s error%s!\n",
					num_warnings == 1 ? "is" : "are",
					num_warnings_str,
					num_warnings == 1 ? "" : "s",
					num_errors_str,
					num_errors == 1 ? "" : "s"
				   );
		}

		if (error_screen)
		{
			fprintf(stderr, "\nThere %s %s warning%s and %s error%s!\n",
					num_warnings == 1 ? "is" : "are",
					num_warnings_str,
					num_warnings == 1 ? "" : "s",
					num_errors_str,
					num_errors == 1 ? "" : "s"
				   );
		}
	}
	else if (num_warnings)
	{
		if (error_logging)
		{
			fprintf(fp, "\nThere %s %s warning%s!\n",
					num_warnings == 1 ? "is" : "are",
					num_warnings_str,
					num_warnings == 1 ? "" : "s"
				   );
		}

		if (error_screen)
		{
			fprintf(stderr, "\nThere %s %s warning%s!\n",
					num_warnings == 1 ? "is" : "are",
					num_warnings_str,
					num_warnings == 1 ? "" : "s"
				   );
		}
	}
	else if (num_errors)
	{
		if (error_logging)
		{
			fprintf(fp, "\nThere %s %s error%s!\n",
					num_errors == 1 ? "is" : "are",
					num_errors_str,
					num_errors == 1 ? "" : "s"
				   );
		}

		if (error_screen)
		{
			fprintf(stderr, "\nThere %s %s error%s!\n",
					num_errors == 1 ? "is" : "are",
					num_errors_str,
					num_errors == 1 ? "" : "s"
				   );
		}
	}

	while (error)
	{
		FF_ERROR_PTR next_error = NULL;

		next_error = pull_error();

		print_error(fp, error, error_logging, error_screen);

		if (!next_error && user_interactive && !is_a_warning(error))
		{
			// fflush(stdin); Undefined jhrg 3/18/11
			fprintf(stderr, "\nPress Enter to Acknowledge...");
			fgets(reply, 2, stdin);
		}
		else if (next_error && user_interactive && !is_a_warning(error))
		{
			// fflush(stdin);
			fprintf(stderr, "\nDisplay next message? (Y/N) : Y\b");
			fgets(reply, 2, stdin);
			if (toupper(reply[0]) != 'Y' && reply[0] != '\n')
			{
				user_interactive = FALSE;
				error_screen = FALSE;
			}
		}

		ff_destroy_error(error);

		error = next_error;
		if (error)
			error_return = min(error_return, error->code);
	} /* while error */

	if (error_logging)
		fprintf(fp, "\nNo more messages\n");

	if (error_screen)
		fprintf(stderr, "\nNo more messages\n");

	if (error_logging)
	{
		fprintf(stderr, "Messages have been recorded in %s\n", std_args->error_log);
		fclose(fp);
	}

	return error_return;
}


/*
 * NAME:    err_get_msg 
 *
 * PURPOSE: To get an error message from the error stack
 *
 * USAGE:   static char * err_get_msg(int error_defined_constant)
 *
 * RETURNS: A pointer to the error message string
 *
 * DESCRIPTION: Uses int msg (error_defined constant) to get the
 *              corresponding error message from the local_errlist
 *              array. If the error is not a system error, a binary
 *              search is called on the local error list. If no error
 *              is found, "Invalid error number" is returned. System
 *              errors are accounted for with a call to the operating
 *              system error list.
 *
 * AUTHOR:  Mark Van Gorp, NGDC, (303)497-6221, mvg@kryton.ngdc.noaa.gov
 *
 * SYSTEM DEPENDENT FUNCTIONS:  sys_errlist, sys_nerr
 *
 * COMMENTS:
 *
 * KEYWORDS:            error system 
 *
 */
#undef ROUTINE_NAME
#define ROUTINE_NAME "err_get_msg"
		
static char *err_get_msg(int msg)
{
	char *p_err_str = NULL;

#if FF_CC == FF_CC_MACCW || FF_CC == FF_CC_UNIX
#define sys_nerr 400
#endif

	if (msg < sys_nerr) 
#if FF_CC == FF_CC_MACCW || FF_CC == FF_CC_UNIX
		p_err_str = strerror(msg);
#else
		p_err_str = sys_errlist[msg];
#endif
	else    
		p_err_str = err_bin_search(msg);

	/* Check if no match was found */
		
	if (!p_err_str)
		p_err_str = "Invalid error number";
		
	return(p_err_str);
}

/*
 * NAME:    error_bin_search 
 *
 * PURPOSE: To search the error array data structure and retrieve
 *          the appropriate error message
 *
 * USAGE:   char * err_binary_search(int error_defined_constant)
 *
 * RETURNS: If successful: a pointer to the appropriate error message
 *          else NULL
 *
 * DESCRIPTION: A non-recursive binary search to find a match 
 *              between int msg (error_defined_constant) and an
 *              error number in local_errlist. If a match is found,
 *              the corresponding error string is returned, else NULL.
 *
 * AUTHOR:  Mark Van Gorp, NGDC, (303)497-6221, mvg@kryton.ngdc.noaa.gov
 *
 * SYSTEM DEPENDENT FUNCTIONS:  None
 *
 * COMMENTS:
 *
 * KEYWORDS:            error system
 *
 */

static char *err_bin_search(int msg)
{
	int low, high, mid;

	low = 0;
	high = NUM_ERROR_ENTRIES - 1;

	while (low <= high)
	{
		mid = (low + high) / 2;

		if (msg < local_errlist[mid].error_number)
			high = mid - 1;
		else if (msg > local_errlist[mid].error_number)
			low = mid + 1;
		else  /* found match */
			return(local_errlist[mid].error_string);
	}

	/* No match found : Invalid error code */

	return(NULL);

}

void _ff_err_assert
	(
	 char *msg,
	 char *file,
	 unsigned line
	)
{
	err_push(ERR_ASSERT_FAILURE, "%s, file %s, line %u", msg, os_path_return_name(file), line);
	err_disp(NULL);

#if FF_CC == FF_CC_MSVC4 || FF_CC == FF_CC_UNIX
	abort();
#else
	memExit(EXIT_FAILURE, "err_assert");
#endif
}

