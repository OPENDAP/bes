%{


#include <string.h>
#include <assert.h>

#ifndef YY_PROTO
#define YY_PROTO(proto) proto
#endif

  
#define YYSTYPE char *
#define YY_DECL int hdfeos2lex YY_PROTO(( void ))
#define YY_READ_BUF_SIZE 16384

#include "hdfeos2.tab.hh"

int yy_line_num = 1;
static int start_line;		/* used in quote and comment error handlers */

%}
    
%option noyywrap
%option prefix="hdfeos2"
%option outfile="lex.hdfeos2.cc"

%x quote
%x comment

GROUP             	GROUP
END_GROUP       	END_GROUP
OBJECT          	OBJECT
END_OBJECT      	END_OBJECT
END             	END
HE5_GCTP_GEO    	GCTP_GEO
DATA_TYPE       	DataType=[A-Z0-9_]*
PROJECTION      	Projection
GRID_STRUCTURE  	GridStructure
SWATH_STRUCTURE  	SwathStructure
GRID_NAME       	GridName
SWATH_NAME       	SwathName
DIMENSION_SIZE  	Size
DIMENSION_NAME  	DimensionName
DATA_FIELD_NAME		DataFieldName
GEO_FIELD_NAME		GeoFieldName
DIMENSION_LIST		DimList
XDIM                    XDim=
YDIM                    YDim=
GRID_ORIGIN             GridOrigin
HDFEGDUL                HDFE_GD_UL 

INT	[-+]?[0-9]+

MANTISA ([0-9]+\.?[0-9]*)|([0-9]*\.?[0-9]+)
EXPONENT (E|e)[-+]?[0-9]+

FLOAT	[-+]?{MANTISA}{EXPONENT}?

STR 	[-+a-zA-Z0-9_./:&%+\-\ ]+

NEVER   [^a-zA-Z0-9_/.+\-{}:&;,%]

%%

{GROUP}	    	    	hdfeos2lval = yytext; return GROUP;
{END_GROUP}    	    	hdfeos2lval = yytext; return END_GROUP;
{OBJECT}    	        hdfeos2lval = yytext; return OBJECT;
{END_OBJECT}    	hdfeos2lval = yytext; return END_OBJECT;
{END}                   /* Ignore */
{INT}                   hdfeos2lval = yytext; return INT;
{FLOAT}                 hdfeos2lval = yytext; return FLOAT;
{PROJECTION}   	    	hdfeos2lval = yytext; return PROJECTION;
{DATA_TYPE}   	    	hdfeos2lval = yytext; return DATA_TYPE;
{GRID_STRUCTURE}    	hdfeos2lval = yytext; return GRID_STRUCTURE;
{SWATH_STRUCTURE}    	hdfeos2lval = yytext; return SWATH_STRUCTURE;
{HE5_GCTP_GEO} 	    	hdfeos2lval = yytext; return HE5_GCTP_GEO;
{GRID_ORIGIN}   	hdfeos2lval = yytext; return GRID_ORIGIN;
{HDFEGDUL}   		hdfeos2lval = yytext; return HDFEGDUL;
{XDIM}	    	    	hdfeos2lval = yytext; return XDIM;
{YDIM}	    	    	hdfeos2lval = yytext; return YDIM;
{GRID_NAME}           	hdfeos2lval = yytext; return GRID_NAME;
{SWATH_NAME}           	hdfeos2lval = yytext; return SWATH_NAME;
{DIMENSION_SIZE} 	hdfeos2lval = yytext; return DIMENSION_SIZE;
{DIMENSION_NAME} 	hdfeos2lval = yytext; return DIMENSION_NAME;
{DIMENSION_LIST} 	hdfeos2lval = yytext; return DIMENSION_LIST;
{DATA_FIELD_NAME} 	hdfeos2lval = yytext; return DATA_FIELD_NAME;
{GEO_FIELD_NAME} 	hdfeos2lval = yytext; return GEO_FIELD_NAME;
{STR}	    	    	hdfeos2lval = yytext; return STR;
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

			  hdfeos2lval = yytext;

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
