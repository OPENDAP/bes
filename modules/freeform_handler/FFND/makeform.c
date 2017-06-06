/* FILENAME:  formlist.c
 *
 * CONTAINS: 
 * Public functions:
 *
 * ff_text_pre_parser
 *
 * Private functions:
 *
 * add_to_variable_list
 * find_EOL
 * find_last_word_on_line
 * find_name
 * get_first_line
 * get_format_type_and_name
 * get_next_line
 * get_section_type
 * is_format
 * is_input_equiv_section
 * is_last_sect
 * is_output_equiv_section
 * make_format
 * parse_fmt_section
 * restore_text_line
 * skip_lead_whitespace
 * update_format_list
 * update_name_table_list
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

#define WANT_FF_TYPES
#include <freeform.h>

FFF_LOOKUP variable_types[NUM_VARIABLE_TYPES] = {
	{"text",        FFV_TEXT},
	{"int8",        FFV_INT8},
	{"uint8",       FFV_UINT8},
	{"int16",       FFV_INT16},
	{"uint16",      FFV_UINT16},
	{"int32",       FFV_INT32},
	{"uint32",      FFV_UINT32},
	{"int64",       FFV_INT64},
	{"uint64",      FFV_UINT64},
	{"float32",     FFV_FLOAT32},
	{"float64",     FFV_FLOAT64},
	{"enote",       FFV_ENOTE},
	{"NEWLINE",     FFV_EOL},
	{"equation",    FFV_EQN},

	{"constant",    FFV_CONSTANT},
	{"initial",     FFV_INITIAL},

	{"char",        FFV_TEXT},     /* provided for backwards compatibility */
	{"uchar",       FFV_UINT8},    /* provided for backwards compatibility */
	{"short",       FFV_INT16},    /* provided for backwards compatibility */
	{"ushort",      FFV_UINT16},   /* provided for backwards compatibility */
	{"long",        FFV_INT32},    /* provided for backwards compatibility */
	{"ulong",       FFV_UINT32},   /* provided for backwards compatibility */
	{"float",       FFV_FLOAT32},  /* provided for backwards compatibility */
	{"double",      FFV_FLOAT64},  /* provided for backwards compatibility */

	{(char *)NULL,  FFV_NULL},
};

/* Define the composite Format types */
#define FFF_BINARY_DATA         (FFF_BINARY | FFF_DATA)
#define FFF_ASCII_DATA          (FFF_ASCII  | FFF_DATA)
#define FFF_FLAT_DATA          (FFF_FLAT  | FFF_DATA)

#define FFF_BINARY_INPUT_DATA   (FFF_BINARY | FFF_DATA | FFF_INPUT)
#define FFF_ASCII_INPUT_DATA    (FFF_ASCII  | FFF_DATA | FFF_INPUT)
#define FFF_FLAT_INPUT_DATA    (FFF_FLAT  | FFF_DATA | FFF_INPUT)

#define FFF_BINARY_OUTPUT_DATA  (FFF_BINARY | FFF_DATA | FFF_OUTPUT)    
#define FFF_ASCII_OUTPUT_DATA   (FFF_ASCII  | FFF_DATA | FFF_OUTPUT)    
#define FFF_FLAT_OUTPUT_DATA   (FFF_FLAT  | FFF_DATA | FFF_OUTPUT)    

#define FFF_BINARY_FILE_HEADER              (FFF_BINARY | FFF_FILE | FFF_HEADER)
#define FFF_ASCII_FILE_HEADER               (FFF_ASCII  | FFF_FILE | FFF_HEADER)
#define FFF_FLAT_FILE_HEADER               (FFF_FLAT  | FFF_FILE | FFF_HEADER)

#define FFF_INPUT_BINARY_FILE_HEADER        (FFF_INPUT | FFF_BINARY | FFF_FILE | FFF_HEADER)
#define FFF_INPUT_ASCII_FILE_HEADER (FFF_INPUT | FFF_ASCII  | FFF_FILE | FFF_HEADER)
#define FFF_INPUT_FLAT_FILE_HEADER (FFF_INPUT | FFF_FLAT  | FFF_FILE | FFF_HEADER)

#define FFF_OUTPUT_BINARY_FILE_HEADER       (FFF_OUTPUT | FFF_BINARY | FFF_FILE | FFF_HEADER)
#define FFF_OUTPUT_ASCII_FILE_HEADER        (FFF_OUTPUT | FFF_ASCII  | FFF_FILE | FFF_HEADER)
#define FFF_OUTPUT_FLAT_FILE_HEADER        (FFF_OUTPUT | FFF_FLAT  | FFF_FILE | FFF_HEADER)

#define FFF_BINARY_FILE_HEADER_SEP  (FFF_BINARY | FFF_SEPARATE | FFF_FILE | FFF_HEADER)
#define FFF_ASCII_FILE_HEADER_SEP   (FFF_ASCII  | FFF_SEPARATE | FFF_FILE | FFF_HEADER)
#define FFF_FLAT_FILE_HEADER_SEP   (FFF_FLAT  | FFF_SEPARATE | FFF_FILE | FFF_HEADER)

#define FFF_INPUT_BINARY_FILE_HEADER_SEP    (FFF_INPUT | FFF_BINARY | FFF_SEPARATE | FFF_FILE | FFF_HEADER)
#define FFF_INPUT_ASCII_FILE_HEADER_SEP             (FFF_INPUT | FFF_ASCII  | FFF_SEPARATE | FFF_FILE | FFF_HEADER)
#define FFF_INPUT_FLAT_FILE_HEADER_SEP             (FFF_INPUT | FFF_FLAT  | FFF_SEPARATE | FFF_FILE | FFF_HEADER)

#define FFF_INPUT_ASCII_FILE_HEADER_SEP_VAR  (FFF_INPUT | FFF_ASCII  | FFF_SEPARATE | FFF_FILE | FFF_HEADER | FFF_VARIED)
#define FFF_INPUT_FLAT_FILE_HEADER_SEP_VAR  (FFF_INPUT | FFF_FLAT  | FFF_SEPARATE | FFF_FILE | FFF_HEADER | FFF_VARIED)

#define FFF_OUTPUT_BINARY_FILE_HEADER_SEP   (FFF_OUTPUT | FFF_BINARY | FFF_SEPARATE | FFF_FILE | FFF_HEADER)
#define FFF_OUTPUT_ASCII_FILE_HEADER_SEP    (FFF_OUTPUT | FFF_ASCII  | FFF_SEPARATE | FFF_FILE | FFF_HEADER)
#define FFF_OUTPUT_FLAT_FILE_HEADER_SEP    (FFF_OUTPUT | FFF_FLAT  | FFF_SEPARATE | FFF_FILE | FFF_HEADER)

#define FFF_OUTPUT_ASCII_FILE_HEADER_SEP_VAR    (FFF_OUTPUT | FFF_ASCII  | FFF_SEPARATE | FFF_FILE | FFF_HEADER | FFF_VARIED)
#define FFF_OUTPUT_FLAT_FILE_HEADER_SEP_VAR    (FFF_OUTPUT | FFF_FLAT  | FFF_SEPARATE | FFF_FILE | FFF_HEADER | FFF_VARIED)

#define FFF_BINARY_REC_HEADER                       (FFF_BINARY | FFF_REC | FFF_HEADER)
#define FFF_ASCII_REC_HEADER                        (FFF_ASCII  | FFF_REC | FFF_HEADER)
#define FFF_FLAT_REC_HEADER                (FFF_FLAT  | FFF_REC | FFF_HEADER)

#define FFF_INPUT_BINARY_REC_HEADER  (FFF_INPUT | FFF_BINARY | FFF_REC | FFF_HEADER)
#define FFF_INPUT_ASCII_REC_HEADER  (FFF_INPUT | FFF_ASCII  | FFF_REC | FFF_HEADER)
#define FFF_INPUT_FLAT_REC_HEADER  (FFF_INPUT | FFF_FLAT  | FFF_REC | FFF_HEADER)

#define FFF_OUTPUT_BINARY_REC_HEADER (FFF_OUTPUT | FFF_BINARY | FFF_REC | FFF_HEADER)
#define FFF_OUTPUT_ASCII_REC_HEADER (FFF_OUTPUT | FFF_ASCII  | FFF_REC | FFF_HEADER)
#define FFF_OUTPUT_FLAT_REC_HEADER (FFF_OUTPUT | FFF_FLAT  | FFF_REC | FFF_HEADER)

#define FFF_BINARY_REC_HEADER_SEP   (FFF_BINARY | FFF_SEPARATE | FFF_REC | FFF_HEADER)
#define FFF_ASCII_REC_HEADER_SEP    (FFF_ASCII  | FFF_SEPARATE | FFF_REC | FFF_HEADER)
#define FFF_FLAT_REC_HEADER_SEP    (FFF_FLAT  | FFF_SEPARATE | FFF_REC | FFF_HEADER)

#define FFF_INPUT_BINARY_REC_HEADER_SEP     (FFF_INPUT | FFF_BINARY | FFF_SEPARATE | FFF_REC | FFF_HEADER)
#define FFF_INPUT_ASCII_REC_HEADER_SEP      (FFF_INPUT | FFF_ASCII  | FFF_SEPARATE | FFF_REC | FFF_HEADER)
#define FFF_INPUT_FLAT_REC_HEADER_SEP      (FFF_INPUT | FFF_FLAT  | FFF_SEPARATE | FFF_REC | FFF_HEADER)

#define FFF_OUTPUT_BINARY_REC_HEADER_SEP    (FFF_OUTPUT | FFF_BINARY | FFF_SEPARATE | FFF_REC | FFF_HEADER)
#define FFF_OUTPUT_ASCII_REC_HEADER_SEP     (FFF_OUTPUT | FFF_ASCII  | FFF_SEPARATE | FFF_REC | FFF_HEADER)
#define FFF_OUTPUT_FLAT_REC_HEADER_SEP     (FFF_OUTPUT | FFF_FLAT  | FFF_SEPARATE | FFF_REC | FFF_HEADER)

#define FFF_BINARY_RECORD  (FFF_RECORD | FFF_BINARY)
#define FFF_ASCII_RECORD   (FFF_RECORD | FFF_ASCII)
#define FFF_FLAT_RECORD   (FFF_RECORD | FFF_FLAT)

FFF_LOOKUP format_types[NUM_FORMAT_TYPES] = {
	{"binary_data",        FFF_BINARY_DATA},
	{"binary_input_data",  FFF_BINARY_INPUT_DATA},
	{"binary_output_data", FFF_BINARY_OUTPUT_DATA},
	{"binary_file_header",        FFF_BINARY_FILE_HEADER},
	{"binary_input_file_header",  FFF_INPUT_BINARY_FILE_HEADER},
	{"binary_output_file_header", FFF_OUTPUT_BINARY_FILE_HEADER},
	{"binary_file_header_separate",        FFF_BINARY_FILE_HEADER_SEP},
	{"binary_input_file_header_separate",  FFF_INPUT_BINARY_FILE_HEADER_SEP},
	{"binary_output_file_header_separate",        FFF_OUTPUT_BINARY_FILE_HEADER_SEP},
	{"binary_record_header",                      FFF_BINARY_REC_HEADER},
	{"binary_input_record_header",    FFF_INPUT_BINARY_REC_HEADER},
	{"binary_output_record_header",   FFF_OUTPUT_BINARY_REC_HEADER},
	{"binary_record_header_separate", FFF_BINARY_REC_HEADER_SEP},
	{"binary_input_record_header_separate",  FFF_INPUT_BINARY_REC_HEADER_SEP},
	{"binary_output_record_header_separate", FFF_OUTPUT_BINARY_REC_HEADER_SEP},
	{"binary_RECORD",  FFF_BINARY_RECORD},

	{"flat_data",         FFF_FLAT_DATA},
	{"flat_input_data",   FFF_FLAT_INPUT_DATA},
	{"flat_output_data",  FFF_FLAT_OUTPUT_DATA},
	{"flat_file_header",         FFF_FLAT_FILE_HEADER},
	{"flat_input_file_header",   FFF_INPUT_FLAT_FILE_HEADER},
	{"flat_output_file_header",           FFF_OUTPUT_FLAT_FILE_HEADER},
	{"flat_file_header_separate",         FFF_FLAT_FILE_HEADER_SEP},
	{"flat_input_file_header_separate",   FFF_INPUT_FLAT_FILE_HEADER_SEP},
	{"flat_input_file_header_separate_varied",   FFF_INPUT_FLAT_FILE_HEADER_SEP_VAR},
	{"flat_output_file_header_separate",         FFF_OUTPUT_FLAT_FILE_HEADER_SEP},
	{"flat_output_file_header_separate_varied",  FFF_OUTPUT_FLAT_FILE_HEADER_SEP_VAR},
	{"flat_record_header",           FFF_FLAT_REC_HEADER},
	{"flat_input_record_header",     FFF_INPUT_FLAT_REC_HEADER},
	{"flat_output_record_header",    FFF_OUTPUT_FLAT_REC_HEADER},
	{"flat_record_header_separate",         FFF_FLAT_REC_HEADER_SEP},
	{"flat_input_record_header_separate",   FFF_INPUT_FLAT_REC_HEADER_SEP},
	{"flat_output_record_header_separate",  FFF_OUTPUT_FLAT_REC_HEADER_SEP},
	{"flat_RECORD",   FFF_FLAT_RECORD},

	{"dbase_data",         FFF_FLAT_DATA},
	{"dbase_input_data",   FFF_FLAT_INPUT_DATA},
	{"dbase_output_data",  FFF_FLAT_OUTPUT_DATA},
	{"dbase_file_header",         FFF_FLAT_FILE_HEADER},
	{"dbase_input_file_header",   FFF_INPUT_FLAT_FILE_HEADER},
	{"dbase_output_file_header",           FFF_OUTPUT_FLAT_FILE_HEADER},
	{"dbase_file_header_separate",         FFF_FLAT_FILE_HEADER_SEP},
	{"dbase_input_file_header_separate",   FFF_INPUT_FLAT_FILE_HEADER_SEP},
	{"dbase_input_file_header_separate_varied",   FFF_INPUT_FLAT_FILE_HEADER_SEP_VAR},
	{"dbase_output_file_header_separate",         FFF_OUTPUT_FLAT_FILE_HEADER_SEP},
	{"dbase_output_file_header_separate_varied",  FFF_OUTPUT_FLAT_FILE_HEADER_SEP_VAR},
	{"dbase_record_header",           FFF_FLAT_REC_HEADER},
	{"dbase_input_record_header",     FFF_INPUT_FLAT_REC_HEADER},
	{"dbase_output_record_header",    FFF_OUTPUT_FLAT_REC_HEADER},
	{"dbase_record_header_separate",         FFF_FLAT_REC_HEADER_SEP},
	{"dbase_input_record_header_separate",   FFF_INPUT_FLAT_REC_HEADER_SEP},
	{"dbase_output_record_header_separate",  FFF_OUTPUT_FLAT_REC_HEADER_SEP},
	{"dbase_RECORD",   FFF_FLAT_RECORD},

	{"ASCII_data",         FFF_ASCII_DATA},
	{"ASCII_input_data",   FFF_ASCII_INPUT_DATA},
	{"ASCII_output_data",  FFF_ASCII_OUTPUT_DATA},
	{"ASCII_file_header",         FFF_ASCII_FILE_HEADER},
	{"ASCII_input_file_header",   FFF_INPUT_ASCII_FILE_HEADER},
	{"ASCII_output_file_header",  FFF_OUTPUT_ASCII_FILE_HEADER},
	{"ASCII_file_header_separate",         FFF_ASCII_FILE_HEADER_SEP},
	{"ASCII_input_file_header_separate",   FFF_INPUT_ASCII_FILE_HEADER_SEP},
	{"ASCII_input_file_header_separate_varied",   FFF_INPUT_ASCII_FILE_HEADER_SEP_VAR},
	{"ASCII_output_file_header_separate",         FFF_OUTPUT_ASCII_FILE_HEADER_SEP},
	{"ASCII_output_file_header_separate_varied",  FFF_OUTPUT_ASCII_FILE_HEADER_SEP_VAR},
	{"ASCII_record_header",           FFF_ASCII_REC_HEADER},
	{"ASCII_input_record_header",     FFF_INPUT_ASCII_REC_HEADER},
	{"ASCII_output_record_header",    FFF_OUTPUT_ASCII_REC_HEADER},
	{"ASCII_record_header_separate",  FFF_ASCII_REC_HEADER_SEP},
	{"ASCII_input_record_header_separate",   FFF_INPUT_ASCII_REC_HEADER_SEP},
	{"ASCII_output_record_header_separate",  FFF_OUTPUT_ASCII_REC_HEADER_SEP},
	{"ASCII_RECORD",   FFF_ASCII_RECORD},

	{(char *)NULL,                           FFF_NULL},
};

typedef enum sect_types_enum
{
 RESERVED              = 0,
 IN_SECT               = 1,
 FMT_SECT              = 2,
 INPUT_EQV_SECT        = 3,
 OUTPUT_EQV_SECT       = 4,
 BEGIN_CONSTANT_SECT   = 5,
 BEGIN_NAME_EQUIV_SECT = 6,
 LAST_SECT             = 7,
 ZEROTH_SECT           = 8  /* initial state before a real section is found */
} sect_types_t;

#ifdef ROUTINE_NAME
#undef ROUTINE_NAME
#endif
#define ROUTINE_NAME "make_format"

static char *skip_lead_whitespace(char *s)
{
	assert(s);

	while (*s && (isspace((int)*s) || *s == '\x1a') && strcspn(s, UNION_EOL_CHARS))
		s++;
	
	return(s);
}

#define is_comment_line(str) (*(str) == '/' ? TRUE : strspn(str, UNION_EOL_CHARS) ? TRUE : FALSE)

#define RESTORE_CHAR(text_line, save_char) text_line[strlen(text_line)] = save_char

/*****************************************************************************
 * NAME:  get_token
 *
 * PURPOSE:  Get a (NULL-terminated) token from a line
 *
 * USAGE:  token = get_token(token, &save_char);
 *
 * RETURNS:  A pointer to the next token (which is NULL-terminated)
 *
 * DESCRIPTION:  Scan text for the a token delimited by whitespace.  If quote
 * characters (") are detected, take the quoted string as a single token.
 * In searching for a token start do not skip over newlines.
 *
 * When a token is found, set save_char to the character after the token,
 * then write a NULL character into that position.  This save_char value
 * should be an argument to the next get_token function call -- the first
 * call to get_token has save_char as the NULL character.  When save_char
 * is not a NULL character argument, the character pointed to by save_char
 * is written over the NULL-terminator of text_line, thus restoring the
 * saved character.
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

static char *get_token
	(
	 char *text_line,
	 char *save_char
	)
{
	char *token_start = NULL;
	char *token_end = NULL;

	assert(text_line);

	if (*save_char)
	{
		token_start = text_line + strlen(text_line);
		RESTORE_CHAR(text_line, *save_char);
	}
	else
		token_start = text_line;

	while (*token_start && strspn(token_start, LINESPACE)) /* skip non-EOL whitespace */
		++token_start;

	token_end = NULL;
	if (*token_start == '"')
	{
		token_end = strchr(token_start + 1, '"');
		if (token_end)
			++token_end;
	}

	if (!token_end)
	{
		token_end = token_start;
		while (*token_end && strcspn(token_end, WHITESPACE)) /* scan until any whitespace */
			++token_end;
	}

	*save_char = *token_end;
	*token_end = STR_END;

	return token_start;
}

/*****************************************************************************
 * NAME:  get_format_type_and_name
 *
 * PURPOSE:  Get the format type and title from a text format description
 *
 * USAGE:  if (get_format_type_and_name(text, &format_type, &format_name)) { }
 *
 * RETURNS:  TRUE if a valid format type, FALSE if not
 *
 * DESCRIPTION:  Parse text into two tokens, lookup the format type of the
 * first token, and set the format_name to the second token.
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

static BOOLEAN get_format_type_and_name
	(
	 char *sect_start, 
	 FF_TYPES_t *fmt_type,
	 char **fmt_name
	)
{
	char *token = NULL;
	char save_char = STR_END;
	
		/* See if a format type string is at start of line */
		/* Parse candidate format type string into a NULL-terminated string
		   for ff_lookup_number
		*/

	token = sect_start;
	token = get_token(token, &save_char);
	*fmt_type = ff_lookup_number(format_types, token);
	
	token = get_token(token, &save_char);
	*fmt_name = token;

	RESTORE_CHAR(token, save_char);
	
	return((BOOLEAN)(*fmt_type != FF_VAR_TYPE_FLAG));
}

/*****************************************************************************
 * NAME:  find_EOL
 *
 * PURPOSE:  Return a pointer to the newline of a text buffer
 *
 * USAGE:  newline = find_EOL(text_line);
 *
 * RETURNS:  A pointer to the first newline character of a text buffer,
 * NULL if none found.
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

static char *find_EOL(char *s)
{
	size_t spn;
	
	if (!FF_STRLEN(s))
		return(NULL);

	spn = strcspn(s, UNION_EOL_CHARS);
	
	return(s + spn);
}

static sect_types_t kind_of_equiv_section(char *text_line)
{
	size_t text_line_len = strlen(text_line);

	if (text_line_len && !FF_SUBSTRCMP(text_line, NTKN_INPUT_EQV))
		return(INPUT_EQV_SECT);
	else if (text_line_len && !FF_SUBSTRCMP(text_line, NTKN_OUTPUT_EQV))
		return(OUTPUT_EQV_SECT);
	else if (text_line_len && !FF_SUBSTRCMP(text_line, NTKN_BEGIN_CONSTANT))
		return(BEGIN_CONSTANT_SECT);
	else if (text_line_len && !FF_SUBSTRCMP(text_line, NTKN_BEGIN_NAME_EQUIV))
		return(BEGIN_NAME_EQUIV_SECT);
	else
		return(0);
}

static BOOLEAN is_equiv_section(char *text_line, sect_types_t current_sect_type)
{
	sect_types_t sect_type = 0;

	/* Have we encountered an input_eqv or output_eqv line followed by a begin constant
	   or begin name_equiv line?  If so, don't report the beginning of a (new) equivalence
		section.  This would lose the output_eqv identifying type for the equivalence
		section.
	*/
	sect_type = kind_of_equiv_section(text_line);
	if ((current_sect_type == INPUT_EQV_SECT || current_sect_type == OUTPUT_EQV_SECT) &&
		 (sect_type == BEGIN_CONSTANT_SECT || sect_type == BEGIN_NAME_EQUIV_SECT)
	   )
	{
		return(FALSE);
	}

	if (sect_type)
		return(TRUE);
	else
		return(FALSE);
}

static sect_types_t is_last_sect(char *text_line)
{
	return(find_EOL(text_line) ? FALSE : TRUE);
}

static BOOLEAN is_format
	(
	 char *text_line 
	)
{
	FF_TYPES_t fmt_type;
	char *fmt_name;
	
	if (get_format_type_and_name(text_line, &fmt_type, &fmt_name))
		return(TRUE);
	else
		return(FALSE);
}

/*****************************************************************************
 * NAME: get_section_type
 *
 * PURPOSE:  Determine section type of text block
 *
 * USAGE:  sect_type = get_section_type(text_line, current_sect_type);
 *
 * RETURNS:  An enumerated section type
 *
 * DESCRIPTION:  Check to see if the first line of text is a comment, format
 * description, an input equivalence section, an output equivalence section,
 * or if there are no more sections.
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

static sect_types_t get_section_type
	(
	 char *text_line,
	 sect_types_t current_section_type
	)
{
	sect_types_t sect_type;
	
	/* The order of the first two if-tests cannot be changed! */
	if (is_comment_line(text_line))
		sect_type = ZEROTH_SECT;
	else if (is_format(text_line))
		sect_type = FMT_SECT;
	else if (is_equiv_section(text_line, current_section_type))
		sect_type = kind_of_equiv_section(text_line);
	else if (is_last_sect(text_line))
		sect_type = LAST_SECT;
	/*
	else if (add_your_BOOLEAN_test_here(text_line))
		sect_type = ADD_YOUR_SECTION_TYPE_HERE;
	*/
	else
	{
		if (current_section_type == ZEROTH_SECT)
			sect_type = FMT_SECT;
		else
			sect_type = IN_SECT;
	}
	
	return(sect_type);
}

static char *get_first_line(char *s)
{
	return(skip_lead_whitespace(s));
}

static char *get_next_line(char *s)
{
	char *t;
	
	assert(s);
	
	t = find_EOL(s);
	if (t)
	{
		t += strspn(t, UNION_EOL_CHARS);
		t = skip_lead_whitespace(t);
	}
	else
		t = s + strlen(s);
	
	return(t);
}

static void check_old_style_EOL_var
	(
	 VARIABLE_PTR var
	)
{
	if (!strcmp("EOL", var->name) && IS_CONSTANT(var))
	{
		var->type &= ~FFV_DATA_TYPES;
		var->type |= FFV_EOL;
	}
}

/*****************************************************************************
 * NAME:  parse_array_variable
 *
 * PURPOSE:  Parse the array description of an array variable
 *
 * USAGE:  error = parse_array_variable(&token, var);
 *
 * RETURNS:  Zero on success, and error code on failure
 *
 * DESCRIPTION:  Copy the array descriptor into var->array_desc_str.
 * If a record variable or a keyworded variable type, copy the variable
 * type into var->record_title.
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

static int parse_array_variable
	(
	 char **array_desc_str,
	 VARIABLE_PTR var
	)
{
	int error = 0;
	char *token = NULL;
	char save_char = STR_END;
	FF_TYPES_t var_type = FFV_NULL;

	FF_VALIDATE(var);

	FFV_TYPE(var) = FF_ARRAY;

	token = get_token(*array_desc_str, &save_char);
	while (strlen(token) && os_strcmpi(token, "OF"))
		token = get_token(*array_desc_str, &save_char);

	if (!strlen(token))
	{
		char *cp = strrchr(*array_desc_str, ']');

		if (!cp || os_strncmpi(cp + 1, "OF", 2))
			return err_push(ERR_VARIABLE_DESC, "Expecting \"OF\" to end array description");

		*token = save_char;

		token = cp + 1;

		save_char = token[2];
		token[2] = STR_END;
	}

	*token++ = STR_END;
	var->array_desc_str = (char *)memStrdup(*array_desc_str, "var->array_desc_str");
	if (!var->array_desc_str)
		return err_push(ERR_MEM_LACK, *array_desc_str);
	else
		os_str_trim_whitespace(var->array_desc_str, var->array_desc_str);

	token = get_token(token, &save_char);

	if (FF_STRLEN(token))
	{
		var_type = ff_lookup_number(variable_types, token);
		if (var_type != FF_VAR_TYPE_FLAG)
			FFV_TYPE(var) |= var_type;
		else
		{
			FFV_TYPE(var) = FFV_RECORD; /* or assign FF_VAR_TYPE_KEYWORD? */

			/* Is the array element type a Record type? (i.e., a format description?)
				Assume for the time being that it is, and error-out later...
			*/

			/* Remember the variable type, which can be a record title, keyword, or typo */

			var->record_title = (char *)memStrdup(token, "var->record_title");
			if (!var->record_title)
				error = err_push(ERR_MEM_LACK, "");

			os_str_replace_char(var->record_title, '"', ' ');
			os_str_trim_whitespace(var->record_title, var->record_title);
		}
	}
	else
	{
		error = err_push(ERR_VARIABLE_DESC, "Expecting a variable type or a record format title for \"%s\"", var->name);
	}
	
	token = get_token(token, &save_char);
	RESTORE_CHAR(token, save_char);

	(*array_desc_str)[strlen(*array_desc_str)] = 'O'; /* The 'O' in "OF" */

	*array_desc_str = token;

	return(error);
}

#define NEED_TO_CHECK_VARIABLE_SIZE(format, var) \
(IS_BINARY(format) && !IS_TEXT(var) && !IS_CONSTANT(var) && !IS_INITIAL(var) && FFV_DATA_TYPE(var) && !IS_RECORD_VAR(var))

/*****************************************************************************
 * NAME: add_to_variable_list
 *
 * PURPOSE:  Parse a variable description line of a format description and
 * add variable to format's variable list.
 *
 * USAGE:  error = add_to_variable_list(text_line, format);
 *
 * RETURNS:  Zero on success, an error code otherwise.
 *
 * DESCRIPTION:  Three types of variable description lines are recognized:
 *
 * 1) A FreeForm Classic variable description.  This describes a field within a record
 * that describes sequence data.  Such a line of text looks like:
 *
 * name start_pos end_pos type precision
 * 
 * (where name is any character string -- no whitespace	-- start_pos, end_pos,
 *  and precision are non-negative integers, and type is a variable type 
 *  character string.)
 *
 * For example:  data 1 2 int16 0
 *
 * 2) A FreeForm ND simple array variable description.  This describes a simple array.  A
 * simple array has a fundamental variable type as its type (e.g., int16).  Such a line of
 * text looks like:
 *
 * name start_pos end_pos ARRAY["name1" start_index TO end_index] OF type precision
 * 
 * (where [, ], ARRAY, TO, and OF are literal, start_index and end_index are non-negative
 *  integers, and name, start_pos, end_pos, type, and precision are as above.  Bracketed
 *  array descriptors may repeat (e.g., ["name2" start_index TO end_index]["name3"..., etc.)
 *  and optional fields may exist after end_index (e.g., SB separation or SEPARATION
 *  separation.)
 *
 * For example:  data 1 2 ARRAY["rows" 1 to 180]["cols" 1 to 360] OF int16 0
 *
 * 3) A FreeForm ND record array variable description.  This describes a record array.  A
 * record array has a FreeForm Classic format as its type.  This means you are describing
 * an array of records, in which each record is composed of one or more fundamental variable
 * types.
 *
 * The variable description line is parsed, and the fields of the VARIABLE structure
 * are assigned values accordingly.
 *
 * 1) var->name
 * 2) var->start_pos
 * 3) var->end_pos
 * 4) var->type
 * 5) var->precision
 * 6) var->array_desc_str (Only for array variables)
 * 7) var->record_title (Only for record variables or keyworded variable types)
 * 
 * Lastly, the format->num_vars and format->length fields are updated.
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

static int add_to_variable_list(char *text_line, FORMAT_PTR format)
{
	VARIABLE_PTR var = NULL;

	char save_char = STR_END;

	char *token = NULL;
	char *endptr = NULL;

	int error = 0;

	if (!format->variables)
	{
		format->variables = dll_init();
		if (!format->variables)
			return(ERR_MEM_LACK);
	}

	token = text_line;
	token = get_token(token, &save_char);
	if (FF_STRLEN(token))
	{
		var = ff_create_variable(token);
		if (var == NULL)
			return ERR_MEM_LACK;
#if 0
			error = ERR_MEM_LACK;
#endif
		if (var->name[0] == '"' && var->name[strlen(var->name) - 1] == '"')
		{
			memmove(var->name, var->name + 1, strlen(var->name) - 2);
			var->name[strlen(var->name) - 2] = STR_END;
		}
	}
	else
	{
		error = err_push(ERR_VARIABLE_DESC, "Expecting a variable name (\"%s\")", format->name);
		goto add_to_variable_list_exit;
	}

	if (!dll_add(format->variables))
	{
		ff_destroy_variable(var);
		error = ERR_MEM_LACK;
		goto add_to_variable_list_exit;
	}

	dll_assign(var, DLL_VAR, dll_last(format->variables));

	token = get_token(token, &save_char);
	if (FF_STRLEN(token))
	{
		errno = 0;
		var->start_pos = strtol(token, &endptr, 10);
		if (errno || FF_STRLEN(endptr))
		{
			error = err_push(errno ? errno : ERR_PARAM_VALUE, "Bad number for variable start position: %s", token);
			goto add_to_variable_list_exit;
		}
	}
	else
	{
		error = err_push(ERR_VARIABLE_DESC, "Expecting a start position for \"%s\"", var->name);
		goto add_to_variable_list_exit;
	}

	token = get_token(token, &save_char);
	if (FF_STRLEN(token))
	{
		errno = 0;
		var->end_pos = strtol(token, &endptr, 10);
		if (errno || FF_STRLEN(endptr))
		{
			error = err_push(errno ? errno : ERR_PARAM_VALUE, "Bad number for variable end position: %s", token);
			goto add_to_variable_list_exit;
		}
	}
	else
	{
		error = err_push(ERR_VARIABLE_DESC, "Expecting an end position for \"%s\"", var->name);
		goto add_to_variable_list_exit;
	}

	token = get_token(token, &save_char);
	if (FF_STRLEN(token))
	{
		FFV_TYPE(var) = ff_lookup_number(variable_types, token);
		if (FFV_TYPE(var) == FF_VAR_TYPE_FLAG)
		{
			if (os_strncmpi("ARRAY", token, 5) == 0)
			{
				RESTORE_CHAR(token, save_char);
				save_char = STR_END;
				error = parse_array_variable(&token, var);
				if (error)
					goto add_to_variable_list_exit;

				format->type |= FF_ARRAY;
			}
			else
			{
				/* Is this a keyworded variable type?  If so, remember name of keyword in record_title */
				if (IS_KEYWORDED_PARAMETER(token))
				{
					FFV_TYPE(var) = 0;

					assert(!var->record_title);

					if (var->record_title)
						memFree(var->record_title, "var->record_title");

					var->record_title = (char *)memStrdup(token, "token");
					if (!var->record_title)
					{
						error = err_push(ERR_MEM_LACK, "");
						goto add_to_variable_list_exit;
					}
				}
				else
				{
					error = err_push(ERR_UNKNOWN_VAR_TYPE, token);
					goto add_to_variable_list_exit;
				}
			}
		}
	}
	else
	{
		error = err_push(ERR_VARIABLE_DESC, "Expecting a variable type or array description for \"%s\"", var->name);
		goto add_to_variable_list_exit;
	}

	token = get_token(token, &save_char);
	if (FF_STRLEN(token))
	{
		errno = 0;
		var->precision = (short)strtol(token, &endptr, 10);
		if (errno || FF_STRLEN(endptr))
		{
			error = err_push(errno ? errno : ERR_PARAM_VALUE, "Bad number for variable precision: %s", token);
			goto add_to_variable_list_exit;
		}
	}
	else
	{
		if (IS_ARRAY(var))
		{
			error = err_push(ERR_VARIABLE_DESC, "Expecting a precision for \"%s\"", var->name);
			goto add_to_variable_list_exit;
		}
	}

	if (var->end_pos < var->start_pos)
	{
		error = err_push(ERR_VARIABLE_DESC,"End Position < Start Position\n%s", text_line);
		goto add_to_variable_list_exit;
	}

	/* Determine The Variable Type */
	if (var->start_pos == 0 && var->end_pos == 0)
	{
		if (IS_BINARY(format))
		{
			error = err_push(ERR_UNKNOWN_FORMAT_TYPE, "Illegal to have delimited binary format");
			goto add_to_variable_list_exit;
		}
		else if (IS_ARRAY(format))
		{
			error = err_push(ERR_UNKNOWN_FORMAT_TYPE, "Illegal to have delimited array format");
			goto add_to_variable_list_exit;
		}

		format->type |= FFF_VARIED;
	}

	if (NEED_TO_CHECK_VARIABLE_SIZE(format, var))
	{
		if (ffv_type_size(var->type) != var->end_pos - var->start_pos + 1)
		{
			char save_eol_char = STR_END;
			char *end_of_line = find_EOL(text_line);

			if (end_of_line)
			{
				save_eol_char = *end_of_line;
				*end_of_line = STR_END;
			}

			error = err_push(ERR_VARIABLE_SIZE,"Expecting ending position for binary field %s to be %d", var->name, var->start_pos + ffv_type_size(var->type) - 1);

			if (end_of_line)
				*end_of_line = save_eol_char;

			goto add_to_variable_list_exit;
		}
	}

	check_old_style_EOL_var(var);
	
	/* Does length of CONSTANT variable name equal length of variable? */
	if (IS_CONSTANT(var) && !IS_EOL(var))
	{
		if (FF_STRLEN(var->name) > FF_VAR_LENGTH(var))
		{
			error = err_push(ERR_VARIABLE_SIZE, "Constant variable initializer (%s) is too long for field", var->name);

			goto add_to_variable_list_exit;
		}
		else if (FF_STRLEN(var->name) < FF_VAR_LENGTH(var))
			error = err_push(ERR_WARNING_ONLY + ERR_VARIABLE_SIZE, "Constant variable initializer (%s) is shorter than field", var->name);
	}

	format->num_vars++;
	format->length = max(format->length, var->end_pos);

add_to_variable_list_exit:

	if (error)
	{
		char *cp;
		char EOL_char = STR_END;

		/* Don't destroy variable since it will be destroyed in ff_destroy_format */

		cp = find_EOL(text_line);
		if (cp)
		{
			EOL_char = *cp;
			*cp = STR_END;
		}

		error = err_push(ERR_VARIABLE_DESC + (error > ERR_WARNING_ONLY ? ERR_WARNING_ONLY : 0),text_line);

		if (cp)
			*cp = EOL_char;

	}

	RESTORE_CHAR(token, save_char);

	return(error);
}

/*****************************************************************************
 * NAME:  append_EOL_to_format
 *
 * PURPOSE:  Append an EOL (NEWLINE) variable to a format
 *
 * USAGE:  error = append_EOL_to_format(format);
 *
 * RETURNS:  0 on success, ERR_MEM_LACK if insufficient memory
 *
 * DESCRIPTION:  Initialize a new variable with EOL type, assign its
 * start and end positions, and update format->length.
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

static int append_EOL_to_format
	(
	 FORMAT_PTR format
	)
{
	VARIABLE_PTR EOL_var = NULL;

	EOL_var = ff_create_variable("EOL");
	if (!EOL_var)
		return(ERR_MEM_LACK);

	if (!dll_add(format->variables))
	{
		ff_destroy_variable(EOL_var);
		return(ERR_MEM_LACK);
	}
		
	dll_assign(EOL_var, DLL_VAR, dll_last(format->variables));

	EOL_var->type = FFV_EOL;

	EOL_var->start_pos = FORMAT_LENGTH(format) + 1;
	EOL_var->end_pos = EOL_var->start_pos;
	format->length = EOL_var->end_pos;

	EOL_var->precision = 0;

	format->num_vars++;

	return(0);
}

/*
 * NAME:	make_format
 *
 * PURPOSE:	Parse variable descriptions, update format info.
 *
 * USAGE:	error = make_format(bufsize, format);
 *
 * RETURNS:	An error code on failure, zero otherwise
 *
 * DESCRIPTION:	
 * A format description is a block of text whose first line is a format type,
 * one or more separating spaces, and then a quoted format title.
 * Subsequent lines are one or more variable descriptions.
 *
 * For example:
 *
 * binary_data "default binary format"
 * data 1 1 uchar 0
 *
 * The above format description contains only one variable description, but
 * might contain several.
 *
 * The following fields of the format structure are assigned values accordingly:
 *
 * format->name
 * format->type
 * format->variables
 *
 * Also, if the format is ASCII and is not an array, an EOL variable is
 * automatically appended.
 *
 * AUTHOR: Mark A. Ohrenschall, NGDC, (303) 497 - 6124 mao@ngdc.noaa.gov
 *
 * COMMENTS:	
 *
 * ERRORS:
 *
 * SYSTEM DEPENDENT FUNCTIONS: 
 *
 * KEYWORDS:
 *
 */

static int make_format
	(
	 char *origin,
	 FF_BUFSIZE_PTR bufsize,
	 FORMAT_HANDLE hformat
	)
{
	char *text_line = NULL;
	char *fmt_name;
	int error = 0;
	
	assert(hformat);
	assert(*hformat == NULL);

	*hformat = NULL;

	text_line = get_first_line(bufsize->buffer);

	if (!text_line)
		return(err_push(ERR_MISSING_TOKEN, "Expecting a newline"));
	else
	{
		FF_TYPES_t check_format_type;
		
		if (get_format_type_and_name(text_line, &check_format_type, &fmt_name))
		{
			char *cp;
			char save_char = STR_END;
			
			cp = find_EOL(fmt_name);
			if (cp)
			{
				save_char = *cp;
				*cp = STR_END;
			}

			*hformat = ff_create_format(fmt_name, origin);
			if (!*hformat)
				return(ERR_MEM_LACK);
			
			(*hformat)->type = check_format_type;

			if (cp)
				*cp = save_char;

			if ((*hformat)->name[0] == '\"')
				(*hformat)->name[0] = ' ';
			cp = strrchr((*hformat)->name, '\"');
			if (cp)
				*cp = ' ';
			os_str_trim_whitespace((*hformat)->name, (*hformat)->name);

			text_line = get_next_line(text_line);
			if (!text_line)
				return(err_push(ERR_VARIABLE_DESC, "Expecting a variable description"));
		}
		else
		{
			char *cp;
			char save_char = STR_END;

			cp = find_EOL(text_line);
			if (cp)
			{
				save_char = *cp;
				*cp = STR_END;
			}

			error = err_push(ERR_UNKNOWN_FORMAT_TYPE, text_line);

			if (cp)
				*cp = save_char;

			return(error);
		}
	}

	while (strlen(text_line))
	{
		if (!is_comment_line(text_line) && strlen(text_line))
		{
			error = add_to_variable_list(text_line, *hformat);
			if (error && error < ERR_WARNING_ONLY)
				return(error);
		}
					
		text_line = get_next_line(text_line);
	}

	if (IS_ARRAY(*hformat) && IS_RECORD_FORMAT(*hformat))
		return(err_push(ERR_MAKE_FORM, "You cannot define a record format containing an array (%s)", (*hformat)->name));
	
	if (!(*hformat)->variables)
		return(err_push(ERR_NO_VARIABLES, "%s", (*hformat)->name));

	if (IS_ASCII(*hformat) && !IS_ARRAY(*hformat) && !IS_VARIED(*hformat))
		error = append_EOL_to_format(*hformat);

	/* experimental */
	if (IS_ARRAY(*hformat))
		(*hformat)->length = 0;

	return(error);
}

/*****************************************************************************
 * NAME:  update_format_list
 *
 * PURPOSE:  Add a format to the format list
 *
 * USAGE:  error = update_format_list(&bufsize, pp_object->u.hf_list);
 *
 * RETURNS:  Zero on success, an error code on failure
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

static int update_format_list
	(
	 char *origin,
	 FF_BUFSIZE_PTR desc_buffer,
	 FORMAT_LIST_HANDLE hf_list
	)
{
	FORMAT_PTR format = NULL;
	
	int error;
	
	error = make_format(origin, desc_buffer, &format);
	if (!error)
	{
		if (!*hf_list)
		{
			*hf_list = dll_init();
			if (!*hf_list)
				error = ERR_MEM_LACK;
		}
	}

	if (!error)
	{
		if (dll_add(*hf_list))
			dll_assign(format, DLL_FMT, dll_last(*hf_list));
		else
			error = ERR_MEM_LACK;
	}
	
	if (error && format)
		ff_destroy_format(format);

	return(error);
}

/*****************************************************************************
 * NAME:  update_name_table_list
 *
 * PURPOSE:  Add or merge a name table to the name table list
 *
 * USAGE:  error = update_name_table_list(&bufsize, pp_object->u.nt_list.hnt_list);
 *
 * RETURNS:  Zero on success or an error code on failure
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

static int update_name_table_list
	(
	 char *origin,
	 FF_BUFSIZE_PTR nt_buffer,
	 NAME_TABLE_LIST_HANDLE hnt_list
	)
{
	NAME_TABLE_PTR nt = NULL;
	
	int error;
	
	error = nt_parse(origin, nt_buffer, &nt);
	if (!error)
		error = nt_merge_name_table(hnt_list, nt);

	return(error);
}

#define SAME_IO_CONTEXT(sect, obj) (\
(sect == INPUT_EQV_SECT && (obj->u.nt_list.nt_io_type & FFF_INPUT)) || \
(sect == OUTPUT_EQV_SECT && (obj->u.nt_list.nt_io_type & FFF_OUTPUT)))

#ifdef ROUTINE_NAME
#undef ROUTINE_NAME
#endif
#define ROUTINE_NAME "ff_text_pre_parser"

/*
 * NAME: ff_text_pre_parser
 *
 * PURPOSE:  Parse a text buffer, determine contents, parse each
 * part as appropriate.
 *
 * USAGE: error = ff_text_pre_parser(bufsize, pp_object)
 *
 * RETURNS: An error code on failure, zero on success
 *
 * DESCRIPTION:  This function is intended to be a dispatcher for other
 * parsers (e.g., nt_create, make_format).  Text files or text buffers
 * can be parsed by this function to determine the type of each element
 * (e.g., equivalence table, format) and each element (section) is then
 * parsed as appropriate.  Since this is not the only point of creation
 * for a newly created data structure, it is added to a list, which is passed
 * as an argument by the calling routine.  It is up to the update_*_list
 * functions called by the specific parser to manage the lists of old and
 * new data structures.
 *
 * The start of any new section must be uniquely identifiable, and preferably
 * unique in occurrence.  This means that a section start should not be a
 * legitimate occurrence within its own, or another section.  For example,
 * defining a keyword called "input_eqv" within an input equivalence section
 * would cause a parsing error, as below:
 *
 * input_eqv
 * begin constant
 * input_eqv text something bogus
 * end constant
 *
 * The occurrence of the keyword input_eqv makes the parser think that an
 * input equivalence section has been encountered, marking the end of the
 * first input equivalence section (whose body is the begin constant line
 * and nothing else), and marking the beginning of the second equivalence section
 * (whose body is the end constant line, and the incidental "text something
 * bogus" following the input_eqv section identifier).
 *
 * The general scheme then is that each section begin with a unique identifier
 * (which may itself be a part of the section it is indicating) and each
 * section ends with the start of a new section, or the end of the buffer,
 * indicated by the NULL-terminator.
 *
 * Initially the state is outside of a section.  Each line is examined in
 * turn until a beginning of a section is found, then the state is within
 * a section.  Each line is further examined until a new section beginning or
 * the NULL-terminator, is found.  At this point the state becomes outside of
 * a section, and the previous section is dispatched to the appropriate parser.
 *
 * The above is repeated until there are no more sections.
 *
 * Currently recognized strings beginning a section are:
 * For formats:
 * 1) a recognized format format type string or number
 *
 * For equivalance tables:
 * 1) "input_eqv"
 * 2) "output_eqv"
 * 
 * AUTHOR:  Mark Ohrenschall, NGDC, (303) 497-6124, mao@ngdc.noaa.gov
 *
 * COMMENTS:  
 *
 * SYSTEM DEPENDENT FUNCTIONS:
 *
 */

int ff_text_pre_parser
	(
	 char *origin,
	 FF_BUFSIZE_PTR fmt_buffer,
	 PP_OBJECT_PTR pp_object
	)
{
	BOOLEAN called_untl = FALSE; /* called update_name_table_list */
	int error     = 0;
	int new_error = 0;

	char *current_sect_start;
	char *text_line;
	
	sect_types_t current_sect_type; /* type of the previous section */
	sect_types_t sect_type;
	BOOLEAN first_section = TRUE;
	
	assert(fmt_buffer);

	text_line = get_first_line(fmt_buffer->buffer);

	first_section      =        TRUE;
	current_sect_start =        NULL;
	current_sect_type  = ZEROTH_SECT;
	sect_type          =  !LAST_SECT;

	/* MAIN LOOP that parses each line of format buffer
	*/

	while (sect_type != LAST_SECT)
	{
		char save_char;
		
		sect_type = get_section_type(text_line, current_sect_type);
		
		switch (sect_type)
		{
			case ZEROTH_SECT:
			case IN_SECT:
				break;

			case FMT_SECT:
			case INPUT_EQV_SECT:
			case OUTPUT_EQV_SECT:
			case BEGIN_CONSTANT_SECT:
			case BEGIN_NAME_EQUIV_SECT:
			case LAST_SECT:
			/*
			case ADD_YOUR_SECTION_TYPE_HERE:
			*/
				if (first_section)
				{
					first_section = FALSE;
				}
				else
				{
					FF_BUFSIZE bufsize;

					/* Process current (previous) section */
					bufsize.buffer = current_sect_start;
					bufsize.bytes_used  =
					bufsize.total_bytes = (FF_BSS_t)((char HUGE *)text_line - (char HUGE *)current_sect_start);
				
					save_char = *text_line;
					*text_line = STR_END;
								
					switch (current_sect_type)
					{
						case FMT_SECT:
							if (pp_object->ppo_type == PPO_FORMAT_LIST)
							{
								error = update_format_list(origin, &bufsize, pp_object->u.hf_list);
								new_error = error;
							}
						break;
									
						case INPUT_EQV_SECT:
						case OUTPUT_EQV_SECT:
						case BEGIN_CONSTANT_SECT:
						case BEGIN_NAME_EQUIV_SECT:
							if (pp_object->ppo_type == PPO_NT_LIST &&
							    SAME_IO_CONTEXT(current_sect_type, pp_object)
							   )
							{
								called_untl = TRUE;
								error = update_name_table_list(origin, &bufsize, pp_object->u.nt_list.hnt_list);
								new_error = error;
							}
						break;
						
						/*
						case ADD_YOUR_SECTION_TYPE_HERE:
						*/
									
						default:
							assert(!ERR_SWITCH_DEFAULT);
							return(err_push(ERR_SWITCH_DEFAULT, "%s, %s:%d", ROUTINE_NAME, os_path_return_name(__FILE__), __LINE__));
					} /* switch on current section type */
					
					if (new_error)
						break;
				
					*text_line = save_char;
				} /* (else) if first section */

				current_sect_type  = sect_type;
				current_sect_start = text_line;
			break;
			
			default:
				assert(!ERR_SWITCH_DEFAULT);
				return(err_push(ERR_SWITCH_DEFAULT, "%s, %s:%d", ROUTINE_NAME, os_path_return_name(__FILE__), __LINE__));
		} /* switch on section type */
		
		if (new_error)
			break;

		if (sect_type != LAST_SECT)
			text_line = get_next_line(text_line);
  } /* while not the last section */

	if (pp_object->ppo_type == PPO_NT_LIST && called_untl == FALSE && !error)
		return(ERR_NO_NAME_TABLE);

	return(error);
}

