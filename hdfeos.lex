%{


#include <string.h>
#include <assert.h>

#define YYSTYPE char *
#define YY_DECL int hdfeoslex YY_PROTO(( void ))
#define YY_READ_BUF_SIZE 16384

#include "hdfeos.tab.h"

int yy_line_num = 1;
static int start_line;		/* used in quote and comment error handlers */

%}
    
%option noyywrap
%x quote
%x comment
GROUP             	GROUP
END_GROUP       	END_GROUP
OBJECT          	OBJECT
END_OBJECT      	END_OBJECT
END             	END
HE5_GCTP_GEO    	HE5_GCTP_GEO
DATA_TYPE       	DataType=  
PROJECTION      	Projection
GRID_STRUCTURE  	GridStructure
GRID_NAME       	GridName
DIMENSION_SIZE  	Size
DIMENSION_NAME  	DimensionName
DATA_FIELD_NAME		DataFieldName
DIMENSION_LIST		DimList

INT	[-+]?[0-9]+

MANTISA ([0-9]+\.?[0-9]*)|([0-9]*\.?[0-9]+)
EXPONENT (E|e)[-+]?[0-9]+

FLOAT	[-+]?{MANTISA}{EXPONENT}?

STR 	[-+a-zA-Z0-9_./:%+\-\ ]+

NEVER   [^a-zA-Z0-9_/.+\-{}:;,%]

%%

{GROUP}	    	    	hdfeoslval = yytext; return GROUP;
{END_GROUP}    	    	hdfeoslval = yytext; return END_GROUP;
{OBJECT}    	        hdfeoslval = yytext; return OBJECT;
{END_OBJECT}    	hdfeoslval = yytext; return END_OBJECT;
{END}                   /* Ignore */
{INT}                   hdfeoslval = yytext; return INT;
{FLOAT}                 hdfeoslval = yytext; return FLOAT;
{PROJECTION}   	    	hdfeoslval = yytext; return PROJECTION;
{DATA_TYPE}   	    	hdfeoslval = yytext; return DATA_TYPE;
{GRID_STRUCTURE}    	hdfeoslval = yytext; return GRID_STRUCTURE;
{HE5_GCTP_GEO} 	    	hdfeoslval = yytext; return HE5_GCTP_GEO;
{GRID_NAME}           	hdfeoslval = yytext; return GRID_NAME;
{DIMENSION_SIZE} 	hdfeoslval = yytext; return DIMENSION_SIZE;
{DIMENSION_NAME} 	hdfeoslval = yytext; return DIMENSION_NAME;
{DIMENSION_LIST} 	hdfeoslval = yytext; return DIMENSION_LIST;
{DATA_FIELD_NAME} 	hdfeoslval = yytext; return DATA_FIELD_NAME;
{STR}	    	    	hdfeoslval = yytext; return STR;


"="                     return (int)*yytext;
"("                     return (int)*yytext;
")"                     return (int)*yytext;
","                     return (int)*yytext;
\"                      /* Ignore */
";"                     /* Ignore */

[ \t]+
\n	    	    	++yy_line_num;
<INITIAL><<EOF>>    	yy_init = 1; yy_line_num = 1; yyterminate();
<comment>[^"\n\\*]*	yymore();
<comment>[^"\n\\*]*\n	yymore(); ++yy_line_num;
<comment>\*[^/\n]       yymore();
<comment>\*\n           yymore(); ++yy_line_num;
<comment>\\.		yymore();
<comment><<EOF>>	{
                          char msg[256];
			  sprintf(msg,
				  "Unterminated comment (starts on line %d)\n",
				  start_line);
			  YY_FATAL_ERROR(msg);
                        }
<quote>[^"\n\\]*	yymore();
<quote>[^"\n\\]*\n	yymore(); ++yy_line_num;
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
