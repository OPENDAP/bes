/*
// Copyright 1996, by the California Institute of Technology.
// ALL RIGHTS RESERVED. United States Government Sponsorship
// acknowledged. Any commercial use must be negotiated with the
// Office of Technology Transfer at the California Institute of
// Technology. This software may be subject to U.S. export control
// laws and regulations. By accepting this software, the user
// agrees to comply with all applicable U.S. export laws and
// regulations. User has the responsibility to obtain export
// licenses, or other export authority as may be required before
// exporting such information to foreign countries or providing
// access to foreign persons.

// Author: Jake Hamby, NASA/Jet Propulsion Laboratory
//         Jake.Hamby@jpl.nasa.gov
//
//
*/

/*
   Scanner for the HDF-EOS StructMetadata attribute.  This file works with
   gnu's flex scanner generator. It returns either ATTR, ID, VAL, TYPE or
   one of the single character tokens `{', `}', `;', `,' or `\n' as integers.
   In the case of an ID or VAL, the scanner stores a pointer to the lexeme
   in yylval (whose type is char *).

   The scanner discards all comment text.

   The scanner returns quoted strings as VALs. Any characters may appear in a
   quoted string except backslash (\) and quote("). To include these escape
   them with a backslash.
   
   The scanner is not reentrant, but can share name spaces with other
   scanners.
   
   Note:
   1) The `defines' file hdfeos.tab.h is built using `bison -d'.
   2) Define YY_DECL such that the scanner is called `hdfeoslex'.
   3) When bison builds the hdfeos.tab.h file, it uses `hdfeos' instead of `yy' for
   variable name prefixes (e.g., yylval --> hdfeoslval).
   4) The quote stuff is very complicated because we want backslash (\)
   escapes to work and because we want line counts to work too. In order to
   properly scan a quoted string two C functions are used: one to remove the
   escape characters from escape sequences and one to remove the trailing
   quote on the end of the string. 

   jeh 6/19/1998
*/

%{
#include "config_hdf.h"

#include <string.h>
#include <assert.h>

#include <parser.h>

#ifndef YY_PROTO
#define YY_PROTO(proto) proto
#endif

#define YYSTYPE char *
#define YY_DECL int hdfeoslex YY_PROTO(( void ))

#include "hdfeos.tab.hh"

int hdfeos_line_num = 1;
static int start_line;		/* used in quote and comment error handlers */

%}
    
%option noyywrap
%option nounput
%option 8bit
%option prefix="hdfeos"
%option outfile="lex.hdfeos.cc"

%x quote
%x comment

GROUP      GROUP
END_GROUP  END_GROUP
OBJECT     OBJECT
END_OBJECT END_OBJECT
END        END|EN

INT	[-+]?[0-9]+

MANTISA ([0-9]+\.?[0-9]*)|([0-9]*\.?[0-9]+)
EXPONENT (E|e)[-+]?[0-9]+

FLOAT	[-+]?{MANTISA}{EXPONENT}?

STR 	[-+a-zA-Z0-9_./:%+&]+

NEVER   [^a-zA-Z0-9_/.+\-{}:;,%&]

%%

{GROUP}	    	    	hdfeoslval = yytext; return GROUP;
{END_GROUP}    	    	hdfeoslval = yytext; return END_GROUP;
{OBJECT}    	        hdfeoslval = yytext; return OBJECT;
{END_OBJECT}    	    hdfeoslval = yytext; return END_OBJECT;
{END}                   /* Ignore */

{INT}                   hdfeoslval = yytext; return INT;
{FLOAT}                 hdfeoslval = yytext; return FLOAT;
{STR}	    	    	hdfeoslval = yytext; return STR;

"="                     return (int)*yytext;
"("                     return (int)*yytext;
")"                     return (int)*yytext;
","                     return (int)*yytext;
";"                     /* Ignore */

[ \t]+
\n	    	    	++hdfeos_line_num;
<INITIAL><<EOF>>    	yy_init = 1; hdfeos_line_num = 1; yyterminate();

"/*"			BEGIN(comment); start_line = hdfeos_line_num; yymore();
<comment>"*/"		{ 
    			  BEGIN(INITIAL); 

			  hdfeoslval = yytext;

			  return COMMENT;
                        }
<comment>[^"\n\\*]*	yymore();
<comment>[^"\n\\*]*\n	yymore(); ++hdfeos_line_num;
<comment>\*[^/\n]       yymore();
<comment>\*\n           yymore(); ++hdfeos_line_num;
<comment>\\.		yymore();
<comment><<EOF>>	{
                          char msg[256];
			  sprintf(msg,
				  "Unterminated comment (starts on line %d)\n",
				  start_line);
			  YY_FATAL_ERROR(msg);
                        }

\"			BEGIN(quote); start_line = hdfeos_line_num; yymore();
<quote>[^"\n\\]*	yymore();
<quote>[^"\n\\]*\n	yymore(); ++hdfeos_line_num;
<quote>\\.		yymore();
<quote>\"		{ 
    			  BEGIN(INITIAL); 

			  hdfeoslval = yytext;

			  return STR;
                        }
<quote><<EOF>>		{
                          char msg[256];
			  sprintf(msg,
				  "Unterminated quote (starts on line %d)\n",
				  start_line);
			  YY_FATAL_ERROR(msg);
                        }

{NEVER}                 {
                          if (yytext) {	/* suppress msgs about `' chars */
                            fprintf(stderr, "Character '%c' (%d) is not", *yytext, *yytext);
                            fprintf(stderr, " allowed (except within");
			    fprintf(stderr, " quotes) and has been ignored\n");
			  }
			}
%%

// Three glue routines for string scanning.

void *
hdfeos_string(const char *str)
{
    return (void *)hdfeos_scan_string(str);
}

void
hdfeos_switch_to_buffer(void *buf)
{
    hdfeos_switch_to_buffer((YY_BUFFER_STATE)buf);
}

void
hdfeos_delete_buffer(void *buf)
{
    hdfeos_delete_buffer((YY_BUFFER_STATE)buf);
}
