
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
 
// (c) COPYRIGHT URI/MIT 1999
// Please read the full copyright statement in the file COPYRIGHT_URI.
//
// Authors:
//      jhrg,jimg       James Gallagher <jgallagher@gso.uri.edu>

// Parse a Grid selection expression. This parser is a little different than
// the other parsers and uses its own argument class. See parser.h.

%code requires {

#include "config.h"

#include <iostream>

#include "Error.h"
#include "GSEClause.h"
#include "gse_parser.h"

using namespace std;
using namespace libdap;
using namespace functions;

// This macro is used to access the instance of a gse_arg class which is
// passed to the parser through a void *. See parser.h.

#define gse_arg(arg) ((gse_arg *)(arg))

// Assume bison 1.25
//#define YYPARSE_PARAM arg
#define YYERROR_VERBOSE 0

} // code requires

%code {

int gse_lex(void);
void gse_error(gse_arg *arg, const char *str);
GSEClause *build_gse_clause(gse_arg *arg, char id[ID_MAX], int op, double val);
GSEClause *build_rev_gse_clause(gse_arg *arg, char id[ID_MAX], int op,
				double val);
GSEClause *
build_dual_gse_clause(gse_arg *arg, char id[ID_MAX], int op1, double val1, 
		      int op2, double val2);

} // code

%require "2.4"

%parse-param {gse_arg *arg}
%name-prefix "gse_"
%defines
%debug
%verbose

%union {
    bool boolean;

    int op;
    char id[ID_MAX];
    double val;
}

%token <val> SCAN_INT
%token <val> SCAN_FLOAT

%token <id> SCAN_WORD
%token <id> SCAN_FIELD

%token <op> SCAN_EQUAL
%token <op> SCAN_NOT_EQUAL
%token <op> SCAN_GREATER
%token <op> SCAN_GREATER_EQL
%token <op> SCAN_LESS
%token <op> SCAN_LESS_EQL

%type <boolean> clause
%type <id> identifier
%type <op> relop
%type <val> constant

%%

clause:		identifier relop constant
                {
		    ((gse_arg *)arg)->set_gsec(
			build_gse_clause((gse_arg *)(arg), $1, $2, $3));
		    $$ = true;
		}
		| constant relop identifier
                {
		    ((gse_arg *)arg)->set_gsec(
		       build_rev_gse_clause((gse_arg *)(arg), $3, $2, $1));
		    $$ = true;
		}
		| constant relop identifier relop constant
                {
		    ((gse_arg *)arg)->set_gsec(
		       build_dual_gse_clause((gse_arg *)(arg), $3, $2, $1, $4,
					     $5));
		    $$ = true;
		}
;

identifier:	SCAN_WORD 
;

constant:       SCAN_INT
		| SCAN_FLOAT
;

relop:		SCAN_EQUAL
                | SCAN_NOT_EQUAL
		| SCAN_GREATER
		| SCAN_GREATER_EQL
		| SCAN_LESS
		| SCAN_LESS_EQL
;

%%

void
gse_error(gse_arg *, const char *)
{
    throw Error(
"An expression passed to the grid() function could not be parsed.\n\
Examples of expressions that will work are: \"i>=10.0\" or \"23.6<i<56.0\"\n\
where \"i\" is the name of one of the Grid's map vectors.");
}

static relop
decode_relop(int op)
{
    switch (op) {
      case SCAN_GREATER:
	return dods_greater_op;
      case SCAN_GREATER_EQL:
	return dods_greater_equal_op;
      case SCAN_LESS:
	return dods_less_op;
      case SCAN_LESS_EQL:
	return dods_less_equal_op;
      case SCAN_EQUAL:
	return dods_equal_op;
      default:
	throw Error(malformed_expr, "Unrecognized relational operator");
    }
}

static relop
decode_inverse_relop(int op)
{
    switch (op) {
      case SCAN_GREATER:
	return dods_less_op;
      case SCAN_GREATER_EQL:
	return dods_less_equal_op;
      case SCAN_LESS:
	return dods_greater_op;
      case SCAN_LESS_EQL:
	return dods_greater_equal_op;
      case SCAN_EQUAL:
	return dods_equal_op;
      default:
	throw Error(malformed_expr, "Unrecognized relational operator");
    }
}

GSEClause *
build_gse_clause(gse_arg *arg, char id[ID_MAX], int op, double val)
{
    return new GSEClause(arg->get_grid(), (string)id, val, decode_relop(op));
}

// Build a GSE Clause given that the operands are reversed.

GSEClause *
build_rev_gse_clause(gse_arg *arg, char id[ID_MAX], int op, double val)
{
    return new GSEClause(arg->get_grid(), (string)id, val, 
			 decode_inverse_relop(op));
}

GSEClause *
build_dual_gse_clause(gse_arg *arg, char id[ID_MAX], int op1, double val1, 
		      int op2, double val2)
{
    // Check that the operands (op1 and op2) and the values (val1 and val2)
    // describe a monotonic interval.
    relop rop1 = decode_inverse_relop(op1);
    relop rop2 = decode_relop(op2);

    switch (rop1) {
      case dods_less_op:
      case dods_less_equal_op:
	if (rop2 == dods_less_op || rop2 == dods_less_equal_op)
	    throw Error(malformed_expr, 
"GSE Clause operands must define a monotonic interval.");
	break;
      case dods_greater_op:
      case dods_greater_equal_op:
	if (rop2 == dods_greater_op || rop2 == dods_greater_equal_op)
	    throw Error(malformed_expr, 
"GSE Clause operands must define a monotonic interval.");
	break;
      case dods_equal_op:
	break;
      default:
	throw Error(malformed_expr, "Unrecognized relational operator.");
    }

    return new GSEClause(arg->get_grid(), (string)id, val1, rop1, val2, rop2);
}

