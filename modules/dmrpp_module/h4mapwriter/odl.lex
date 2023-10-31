/*

 Scanner for ODL parser.

*/

%{
#include <string.h>
#include <assert.h>

#ifndef YY_PROTO
#define YY_PROTO(proto) proto
#endif

#define YYSTYPE char *
#define YY_DECL int odllex YY_PROTO(( void ))

#include "odl.tab.h"

int odl_line_num = 1;
static int start_line;		/* used in quote and comment error handlers */

%}
    
%option noyywrap
%option nounput
%option 8bit
%option prefix="odl"
%option outfile="lex.odl.c"

%x quote
%x comment

GROUP      GROUP
END_GROUP  END_GROUP
OBJECT     OBJECT
END_OBJECT END_OBJECT
END        END|EN
NULLVAL    Value=\ ;
INT	[-+]?[0-9]+

MANTISA ([0-9]+\.?[0-9]*)|([0-9]*\.?[0-9]+)
EXPONENT (E|e)[-+]?[0-9]+

FLOAT	[-+]?{MANTISA}{EXPONENT}?

STR 	[-+a-zA-Z0-9_./:%+&]+

NEVER   [^a-zA-Z0-9_/.+\-{}:;,%&]

%%

{GROUP}	    	    	odllval = yytext; return GROUP;
{END_GROUP}    	    	odllval = yytext; return END_GROUP;
{OBJECT}    	        odllval = yytext; return OBJECT;
{END_OBJECT}    	odllval = yytext; return END_OBJECT;
{END}                   /* Ignore */

{INT}                   odllval = yytext; return INT;
{FLOAT}                 odllval = yytext; return FLOAT;
{STR}	    	    	odllval = yytext; return STR;

"="                     return (int)*yytext;
"("                     return (int)*yytext;
")"                     return (int)*yytext;
","                     return (int)*yytext;
";"                     /* Ignore */

[ \t]+
\n	    	    	++odl_line_num;
<INITIAL><<EOF>>    	yy_init = 1; odl_line_num = 1; yyterminate();

"/*"			BEGIN(comment); start_line = odl_line_num; yymore();
<comment>"*/"		{ 
    			  BEGIN(INITIAL); 

			  odllval = yytext;

			  return COMMENT;
                        }
<comment>[^"\n\\*]*	yymore();
<comment>[^"\n\\*]*\n	yymore(); ++odl_line_num;
<comment>\*[^/\n]       yymore();
<comment>\*\n           yymore(); ++odl_line_num;
<comment>\\.		yymore();
<comment><<EOF>>	{
                          char msg[256];
			  sprintf(msg,
				  "Unterminated comment (starts on line %d)\n",
				  start_line);
			  YY_FATAL_ERROR(msg);
                        }
\"			BEGIN(quote); start_line = odl_line_num; yymore();
<quote>[^"\n\\]*	yymore();
<quote>[^"\n\\]*\n	yymore(); ++odl_line_num;
<quote>\\.		yymore();
<quote>\"		{ 
    			  BEGIN(INITIAL); 

			  odllval = yytext;

			  return STR;
                        }
<quote><<EOF>>		{
                          char msg[256];
			  sprintf(msg,
				  "Unterminated quote (starts on line %d)\n",
				  start_line);
			  YY_FATAL_ERROR(msg);
                        }
{NULLVAL} {
    fprintf(stderr, "<!-- ERROR:ODL has empty value in \"Value= ;\" line. -->\n");
}

{NEVER}                 {
                          if (yytext) {	/* suppress msgs about `' chars */
                            fprintf(stderr, "Character '%c' (%d) is not", *yytext, *yytext);
                            fprintf(stderr, " allowed (except within");
			    fprintf(stderr, " quotes) and has been ignored\n");
			  }
			}
%%


void *
odl_string(const char *str)
{
    return (void *)odl_scan_string(str);
}
