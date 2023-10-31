/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) 2010-2016 The HDF Group                                     *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF.  The full HDF copyright notice, including       *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file and in the print documentation copyright notice.         *
 * COPYING can be found at the root of the source code distribution tree;    *
 * the copyright notice in printed HDF documentation can be found on the     *
 * back of the title page.  If you do not have access to either version,     *
 * you may request a copy from help@hdfgroup.org.                            *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
  \file odl.y
  \brief  ODL parser for h4mapwriter.
*/

%code requires {

#define YYSTYPE char *
#define YYDEBUG 1


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <uuid/uuid.h>          /* for UUID generation */    
#include "hmap.h"

extern int odl_line_num;	/* defined in odl.lex */
extern unsigned int ID_GA;
} 

%code {
static int indent = 3;
static int list_count = 0;
static int data_type = 0;
char* strval = NULL;
VOIDP buf = NULL;
    
int odllex(void);
void odlerror(FILE* arg,  char *s);
static void process_group(FILE* arg,  char *s);
static void end_group(FILE* arg);
static void process_attr(FILE* arg,  char *s);
static void end_attr(FILE* arg);
} 

%require "2.4"

%parse-param {FILE *arg}
%name-prefix "odl"
%defines
%debug
%verbose

%expect 10

%token GROUP
%token END_GROUP
%token OBJECT
%token END_OBJECT
%token END

%token INT
%token FLOAT
%token STR
%token COMMENT

%%
attributes: 
    {
    }
    attribute
    | attributes attribute
;
    	    	
attribute: GROUP '=' STR
    {
	    process_group(arg, $3);
    }
    attr_list
    {

    }
    END_GROUP '=' STR
    {
        end_group(arg);
    }
    | OBJECT '=' STR 
    {
	    process_group(arg, $3);
    }
    attr_list {
    }
    END_OBJECT '=' STR
    {
        end_group(arg);
    }
    | STR
    {
        process_attr(arg, $1);
    }
    '=' data
    {
        end_attr(arg);
    }
    | COMMENT {

    }

    | error
    {

    }
;

attr_list:  	/* empty */
    	    	| attribute
    	    	| attr_list attribute
;

data:           ints
                | floats
                | strs
{
    write_array_attribute(arg, DFNT_CHAR, strlen(strval), strval, 
                          strlen(strval), -1, -1,
                          indent+1);
    HDfree(strval);
    
}
                | '(' data2 ')'
{

    if (data_type == DFNT_CHAR) {
        write_array_attribute(arg, DFNT_CHAR, strlen($2)-1, $2, 
                              strlen($2)-1, -1, -1,
                              indent+1);
    }
    
    if (data_type == DFNT_INT32) {
        write_array_attribute(arg, DFNT_INT32, list_count, buf,
                              -1, -1, -1,
                              indent+1);
        HDfree(buf);
        buf = NULL;
    }
    if (data_type == DFNT_FLOAT32) {
        write_array_attribute(arg, DFNT_FLOAT32, list_count, buf,
                              -1, -1, -1,
                              indent+1);
        HDfree(buf);
        buf = NULL;
    }    
}
;

data2:          floatints
{

}
                | strs
{
    data_type = DFNT_CHAR;
}
;

ints:           INT
		{
                    int val = atoi($1);
                    write_array_attribute(arg, DFNT_INT32, 1, &val, 
                                          -1, -1, -1,
                                          indent+1);                    
		}
		| ints ',' INT
		{
                    list_count++;
                    fprintf(stderr, "ODL has integer number list without ().\n");
                    exit(-1);                    
		}
;

floats:		FLOAT
		{
                    float val = atof($1);
                    write_array_attribute(arg, DFNT_FLOAT32, 1, &val, 
                                          -1, -1, -1,
                                          indent+1);

		}
		| floats ',' FLOAT
		{
                    list_count++;
                    fprintf(stderr, "ODL has floating number list without ().\n");
                    exit(-1);
		}
;

floatints:	float_or_int
		{
                    list_count++;
                    if (buf == NULL) {
                        if (data_type == DFNT_FLOAT32) {
                            buf = (float32 *) HDmalloc(sizeof(float32));
                            ((float32 *)buf)[0] = atof($1);
                        }
                        else {
                            buf = (int32 *) HDmalloc(sizeof(int32));
                            ((int32 *)buf)[0] = atoi($1);
                        }
                        
                    }

		}
		| floatints ',' float_or_int
		{
                    list_count++;
                    if(data_type == DFNT_FLOAT32) {
                        buf = (float32 *) HDrealloc((VOIDP) buf,
                                                    list_count * sizeof(float32));
                        ((float32 *)buf)[list_count-1] = atof($3);
                    }
                    else {
                        buf = (int32 *) HDrealloc((VOIDP) buf,
                                                    list_count * sizeof(int32));
                        ((int32 *)buf)[list_count-1] = atoi($3);                        
                    }
		}
;

float_or_int:   FLOAT
{
    data_type = DFNT_FLOAT32;
}
| INT
{
    data_type = DFNT_INT32;
}
;

strs:		STR
		{
                    strval = (char*) HDmalloc(strlen($1)+1);
                    strncpy(strval, $1, strlen($1));
                    strval[strlen($1)] = '\0';
                    
		}
                | strs ',' STR
		{
		}
;

%%

/*
 This function is required for linking.
*/
void
odlerror(FILE * arg, char *s)
{
}

static void 
process_group(FILE * arg, char *s)
{
    write_group(arg, s, "N/A", "", indent);
    ++indent;
}

static void 
end_group(FILE * arg)
{
    --indent;    
    end_elm(arg, TAG_GRP, indent);    
}

static void 
process_attr(FILE * arg, char *s)
{
    uuid_t id;         /* for uuid */
    char uuid_str[37]; /* ex. "1b4e28ba-2fa1-11d2-883f-0016d3cca427" + "\0" */
    list_count = 0;
    data_type = 0;

    uuid_generate(id);
    uuid_unparse_lower(id, uuid_str);
    
    start_elm(arg, TAG_GATTR, indent);
    fprintf(arg, "name=\"%s\" id=\"ID_GA%d\" uuid=\"%s\">", s, ++ID_GA,
            uuid_str);
    ++indent;
}

static void 
end_attr(FILE * arg)
{
    --indent;
    end_elm(arg, TAG_GATTR, indent);    
}
