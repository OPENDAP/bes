
/*
 -*- mode: c++; c-basic-offset:4 -*-

 This file is part of libdap, A C++ implementation of the OPeNDAP Data
 Access Protocol.

 Copyright (c) 2002,2003 OPeNDAP, Inc.
 Author: James Gallagher <jgallagher@opendap.org>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

 (c) COPYRIGHT URI/MIT 1999
*/ 

/*
  Scanner for grid selection sub-expressions. The scanner is not reentrant,
  but can share a name space with other scanners.

   Note:
   1) The `defines' file gse.tab.h is built using `bison -d'.
   2) Define YY_DECL such that the scanner is called `gse_lex'.
   3) When bison builds the gse.tab.h file, it uses `gse_' instead
   of `yy' for variable name prefixes (e.g., yylval --> gse_lval).

   1/13/99 jhrg
*/

%{

#include "config.h"

#include <string>
#include <cstring>

#include "Error.h"

#ifndef YY_PROTO
#define YY_PROTO(proto) proto
#endif

#define YY_DECL int gse_lex YY_PROTO(( void ))
#define ID_MAX 256
#define YY_NO_UNPUT 1
#define YY_NO_INPUT 1
#define YY_FATAL_ERROR(msg) {\
    throw(Error(string("Error scanning grid constraint expression text: ") + string(msg))); \
}

#include "gse.tab.hh"

using namespace std;
using namespace libdap;

static void store_int32();
static void store_float64();
static void store_id();
static void store_op(int op);

%}

%option noyywrap
%option nounput
%option 8bit
%option prefix="gse_"
%option outfile="lex.gse.cc"

NAN     [Nn][Aa][Nn]
INF     [Ii][Nn][Ff]

SCAN_INT	[-+]?[0-9]+

SCAN_MANTISA	([0-9]+\.?[0-9]*)|([0-9]*\.?[0-9]+)
SCAN_EXPONENT	(E|e)[-+]?[0-9]+

SCAN_FLOAT	([-+]?{SCAN_MANTISA}{SCAN_EXPONENT}?)|({NAN})|({INF})

/* See das.lex for comments about the characters allowed in a WORD.
   10/31/2001 jhrg */

SCAN_WORD	[-+a-zA-Z0-9_/%.\\][-+a-zA-Z0-9_/%.\\#]*

SCAN_EQUAL	=
SCAN_NOT_EQUAL	!=
SCAN_GREATER	>
SCAN_GREATER_EQL >=
SCAN_LESS	<
SCAN_LESS_EQL	<=

NEVER		[^a-zA-Z0-9_/%.#:+\-,]

%%

{SCAN_INT}	store_int32(); return SCAN_INT;
{SCAN_FLOAT}	store_float64(); return SCAN_FLOAT;

{SCAN_WORD}	store_id(); return SCAN_WORD;

{SCAN_EQUAL}	store_op(SCAN_EQUAL); return SCAN_EQUAL;
{SCAN_NOT_EQUAL} store_op(SCAN_NOT_EQUAL); return SCAN_NOT_EQUAL;
{SCAN_GREATER}	store_op(SCAN_GREATER); return SCAN_GREATER;
{SCAN_GREATER_EQL} store_op(SCAN_GREATER_EQL); return SCAN_GREATER_EQL;
{SCAN_LESS}	store_op(SCAN_LESS); return SCAN_LESS;
{SCAN_LESS_EQL}	store_op(SCAN_LESS_EQL); return SCAN_LESS_EQL;

%%

// Three glue routines for string scanning. These are not declared in the
// header gse.tab.h nor is YY_BUFFER_STATE. Including these here allows them
// to see the type definitions in lex.gse.c (where YY_BUFFER_STATE is
// defined) and allows callers to declare them (since callers outside of this
// file cannot declare YY_BUFFER_STATE variable).

void *
gse_string(const char *str)
{
    return (void *)gse__scan_string(str);
}

void
gse_switch_to_buffer(void *buf)
{
    gse__switch_to_buffer((YY_BUFFER_STATE)buf);
}

void
gse_delete_buffer(void *buf)
{
    gse__delete_buffer((YY_BUFFER_STATE)buf);
}

// Note that the grid() CE function only deals with numeric maps (8/28/2001
// jhrg) and that all comparisons are done using doubles. 

static void
store_int32()
{
    gse_lval.val = atof(yytext);
}

static void
store_float64()
{
    gse_lval.val = atof(yytext);
}

static void
store_id()
{
    strncpy(gse_lval.id, yytext, ID_MAX-1);
    gse_lval.id[ID_MAX-1] = '\0';
}

static void
store_op(int op)
{
    gse_lval.op = op;
}

