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
   1) The `defines' file he5dds.tab.h is built using `bison -d'.
   2) Define YY_DECL such that the scanner is called `he5ddslex'.
   3) When bison builds the he5dds.tab.h file, it uses `he5dds' instead of `yy' for
   variable name prefixes (e.g., yylval --> he5ddslval).
   4) The quote stuff is very complicated because we want backslash (\)
   escapes to work and because we want line counts to work too. In order to
   properly scan a quoted string two C functions are used: one to remove the
   escape characters from escape sequences and one to remove the trailing
   quote on the end of the string. 

   jeh 6/19/1998
*/

%{
#ifndef YY_PROTO
#define YY_PROTO(proto) proto
#endif
#define YYSTYPE char *
#define YY_DECL int he5daslex YY_PROTO(( void ))
#define YY_READ_BUF_SIZE 16384
#define ECHO if (fwrite( yytext, yyleng, 1, yyout )) {}

#include <string.h>
#include <assert.h>
#include <string>
#include <sstream>
#include <iostream>

#include "config_hdf5.h"
#include "parser.h"
#include "he5das.tab.hh"

using namespace std;
using namespace libdap;

int he5das_line_num = 1;
static int start_line;		/* used in quote and comment error handlers */

%}
    
%option noyywrap
%option prefix="he5das"
%option outfile="lex.he5das.cc"
%option 8bit

%x quote
%x comment

GROUP      GROUP
END_GROUP  END_GROUP
OBJECT     OBJECT
END_OBJECT END_OBJECT
END        END
DATA_TYPE  DataType=[A-Z0-9_]*    
INT	[-+]?[0-9]+

MANTISA ([0-9]+\.?[0-9]*)|([0-9]*\.?[0-9]+)
EXPONENT (E|e)[-+]?[0-9]+

FLOAT	[-+]?{MANTISA}{EXPONENT}?

STR 	[-+a-zA-Z0-9_./:%+\-]+ 


NEVER   [^a-zA-Z0-9_/.+\-{}:;,%]

%%

{GROUP}	    	    	he5daslval = yytext; return DAS_GROUP;
{END_GROUP}    	    	he5daslval = yytext; return DAS_END_GROUP;
{OBJECT}    	        he5daslval = yytext; return DAS_OBJECT;
{END_OBJECT}    	he5daslval = yytext; return DAS_END_OBJECT;
{END}                   /* Ignore */
{INT}                   he5daslval = yytext; return DAS_INT;
{FLOAT}                 he5daslval = yytext; return DAS_FLOAT;
{DATA_TYPE}	    	/* Ignore */
{STR}	    	    	he5daslval = yytext; return DAS_STR;

"="                     return (int)*yytext;
"("                     return (int)*yytext;
")"                     return (int)*yytext;
","                     return (int)*yytext;
";"                     /* Ignore */

[ \t]+
\n	    	    	++he5das_line_num;
<INITIAL><<EOF>>    	yy_init = 1; he5das_line_num = 1; yyterminate();

"/*"			BEGIN(comment); start_line = he5das_line_num; yymore();
<comment>"*/"		{ 
    			  BEGIN(INITIAL); 

			  he5daslval = yytext;

			  return DAS_COMMENT;
                        }
<comment>[^"\n\\*]*	yymore();
<comment>[^"\n\\*]*\n	yymore(); ++he5das_line_num;
<comment>\*[^/\n]       yymore();
<comment>\*\n           yymore(); ++he5das_line_num;
<comment>\\.		yymore();
<comment><<EOF>>	{
                          ostringstream msg;
                          msg << "Unterminated comment (starts on line "
                              << start_line << ")";
                          YY_FATAL_ERROR(msg.str().c_str());
                        }

\"			BEGIN(quote); start_line = he5das_line_num; yymore();
<quote>[^"\n\\]*	yymore();
<quote>[^"\n\\]*\n	yymore(); ++he5das_line_num;
<quote>\\.		yymore();
<quote>\"		{ 
    			  BEGIN(INITIAL); 

			  he5daslval = yytext;

			  return DAS_STR;
                        }
<quote><<EOF>>		{
                          ostringstream msg;
                          msg << "Unterminated quote (starts on line "
                              << start_line << ")";
                          YY_FATAL_ERROR(msg.str().c_str());
                        }

{NEVER}                 {
                          if (yytext) {	/* suppress msgs about `' chars */
                            fprintf(stderr, "Character '%c' (%d) is not", *yytext, *yytext);
                            fprintf(stderr, " allowed (except within");
			    fprintf(stderr, " quotes) and has been ignored\n");
			  }
			}
%%
