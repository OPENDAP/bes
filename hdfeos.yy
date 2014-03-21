/*
// -*- C++ -*-

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
   Grammar for the HDF-EOS StructMetadata attribute.
   This grammar can be used with the bison parser
   generator to build a parser for the DAS. It assumes that a scanner called
   `hdfeoslex()' exists and that the objects DAS and AttrTable also exist.

   jeh 6/19/1998
*/

%code requires {

#define YYSTYPE char *
#define YYDEBUG 1

// static char rcsid[] not_used = {"$Id$"};

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <assert.h>

#include <vector>
#include <sstream>

using namespace std;

#include "DAS.h"
#include "Error.h"
#include "BESDebug.h"
#include "parser.h"
//#include "hdfeos.tab.hh"

#ifdef TRACE_NEW
#include "trace_new.h"
#endif

using namespace libdap ;

// These macros are used to access the `arguments' passed to the parser. A
// pointer to an error object and a pointer to an integer status variable are
// passed in to the parser within a structure (which itself is passed as a
// pointer). Note that the ERROR macro explicitly casts OBJ to an ERROR *. 

#define ATTR_OBJ(arg) ((AttrTable *)((parser_arg *)(arg))->_object)
#define ERROR_OBJ(arg) ((parser_arg *)(arg))->_error
#define STATUS(arg) ((parser_arg *)(arg))->_status

// #define YYPARSE_PARAM arg

extern int hdfeos_line_num;	/* defined in hdfeos.lex */

} // code requires

%code {

static string name;	/* holds name in attr_pair rule */
static string type;	/* holds type in attr_pair rule */
static string last_grid_swath;  /* holds HDF-EOS name for aliasing */
static int commentnum=0;   /* number of current comment */

using namespace libdap;

static vector<AttrTable *> *attr_tab_stack;

// I use a vector of AttrTable pointers for a stack

#define TOP_OF_STACK (attr_tab_stack->back())
#define SECOND_IN_STACK ((*attr_tab_stack)[attr_tab_stack->size()-2])
#define PUSH(x) (attr_tab_stack->push_back(x))
#define POP (attr_tab_stack->pop_back())
#define STACK_LENGTH (attr_tab_stack->size())
#define STACK_EMPTY (attr_tab_stack->empty())

#define TYPE_NAME_VALUE(x) type << " " << name << " " << (x)

#if 0
static const char *NO_DAS_MSG =
"The attribute object returned from the dataset was null\n\
Check that the URL is correct.";
#endif

void mem_list_report();
int hdfeoslex(void);
void hdfeoserror(parser_arg *arg, const char *s);
static void process_group(parser_arg *arg, const string &s);

} //code

%require "2.4"

%parse-param {parser_arg *arg}
%name-prefix "hdfeos"
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

/*
  Parser algorithm: 

  Look for a `variable' name (this can be any identifier, but by convention
  it is either the name of a variable in a dataset or the name of a grouping
  of global attributes). Create a new attribute table for this identifier and
  push the new attribute table onto a stack. If attribute tuples
  (type-name-value tuples) are found, intern them in the attribute table
  found on the top of the stack. If the start attribute table of a new
  attribute table if found (before the current table is closed), create the
  new table and push *it* on the stack. As attribute tables are closed, pop
  them off the stack.

  This algorithm ensures that we can nest attribute tables to an arbitrary
  depth).

  Alias are handled using mfuncs of both the DAS and AttrTable objects. This
  is necessary because the first level of a DAS object can contain only
  AttrTables, not attribute tuples. Whereas, the subsequent levels can
  contain both. Thus the compete definition is split into two objects. In
  part this is also a hold over from an older design which did not
  have the recursive properties of the current design.

  Alias can be made between attributes within a given lexical level, from one
  level to the next within a sub-hierarchy or across hierarchies.

  Tokens:

  BYTE, INT32, UINT32, FLOAT64, STRING and URL are tokens for the type
  keywords. The tokens INT, FLOAT, STR and ID are returned by the scanner to
  indicate the type of the value represented by the string contained in the
  global DASLVAL. These two types of tokens are used to implement type
  checking for the attributes. See the rules `bytes', etc. Additional tokens:
  ATTR (indicates the start of an attribute object) and ALIAS (indicates an
  alias). 
*/

attributes:    	/* Create the AttrTable stack if necessary */
    {
	   if (!attr_tab_stack)
	       attr_tab_stack = new vector<AttrTable *>;
    }
    attribute
    | attributes attribute
;
    	    	
attribute: GROUP '=' STR
    {
	    process_group((parser_arg *)arg, $3);
	}
    attr_list
    {
	   /* pop top of stack; store in attr_tab */
	   BESDEBUG("h4", " Popped attr_tab: " << TOP_OF_STACK << endl);
	   POP;
	}
	END_GROUP '=' STR

	| OBJECT '=' STR
    {
	    process_group((parser_arg *)arg, $3);
	}
    attr_list
    {
        /* pop top of stack; store in attr_tab */
		BESDEBUG("h4", " Popped attr_tab: " << TOP_OF_STACK << endl);
		POP;
	}
	END_OBJECT '=' STR

    | STR 
    { 
        name = $1; 
	}
    '=' data

	| COMMENT {
	    ostringstream name, comment;
	    name << "comment" << commentnum++;
#ifdef ATTR_STRING_QUOTE_FIX
        comment << $1;
#else
        comment << "\"" << $1 << "\"";
#endif
		 BESDEBUG("h4",name.str() << ":" << comment.str() << endl);

		AttrTable *a;
		if (STACK_EMPTY)
		    a = ATTR_OBJ(arg);
		else
		    a = TOP_OF_STACK;
		if (!a->append_attr(name.str(), "String", comment.str())) {
		    ostringstream msg;
		    msg << "`" << name.str() << "' previously defined.";
		    parse_error((parser_arg *)arg, msg.str().c_str());
		    YYABORT;
		}
	}

    | error
    {
        AttrTable *a;
		if (STACK_EMPTY)
			a = ATTR_OBJ(arg);
		else
			a = TOP_OF_STACK;
#ifdef ATTR_STRING_QUOTE_FIX
        a->append_attr(name.c_str(), "String", "Error processing EOS attributes");
#else
		a->append_attr(name.c_str(), "String", "\"Error processing EOS attributes\"");
#endif
        /*Using parse_error causes the memory leaking. So don't use this function. instead just
          set the status to FALSE.  KY 2014-02-25*/
                arg->set_status(FALSE);
        /*string msg = "Could not parse item; was expecting a GROUP, OBJECT or String: ";
        msg += $1;
		parse_error((parser_arg *)arg, msg.c_str());
        */
		/* Don't abort; keep parsing to try and pick up more
		   attributes. 3/30/2000 jhrg */
 		/* YYABORT; */
	}
;

attr_list:  	/* empty */
    	    	| attribute
    	    	| attr_list attribute
;

data:           ints
                | floats
                | strs
                | '(' data2 ')'
;

/* To avoid parse errors on a mixture of ints and floats, only allow floats */
/* and strings inside a list.  For this, we need a new rule to recognize */
/* a mixture of floats and ints. */
data2:          floatints
                | strs
;

ints:           INT
		{
		    /* NB: On the Sun (SunOS 4) strtol does not check for */
		    /* overflow. Thus it will never figure out that 4 */
		    /* billion is way to large to fit in a 32 bit signed */
		    /* integer. What's worse, long is 64  bits on Alpha and */
		    /* SGI/IRIX 6.1... jhrg 10/27/96 */
		    /* type = "Int32"; */
		    BESDEBUG("h4", "Adding INT: " << TYPE_NAME_VALUE($1) << endl);
		    BESDEBUG("h4", " to AttrTable: " << TOP_OF_STACK << endl);
		    if (!(check_int32($1) 
			  || check_uint32($1))) {
			ostringstream msg;
			msg << "`" << $1 << "' is not an Int32 value.";
			parse_error((parser_arg *)arg, msg.str().c_str());
			YYABORT;
		    }
		    else if (!TOP_OF_STACK->append_attr(name, "Int32", $1)) {
			ostringstream msg;
			msg << "`" << name << "' previously defined.";
			parse_error((parser_arg *)arg, msg.str().c_str());
			YYABORT;
		    }
		}
		| ints ',' INT
		{
		    type = "Int32";
		    BESDEBUG("h4", "Adding INT: " << TYPE_NAME_VALUE($3) << endl);
		    if (!(check_int32($3)
			  || check_uint32($3))) {
			ostringstream msg;
			msg << "`" << $3 << "' is not an Int32 value.";
			parse_error((parser_arg *)arg, msg.str().c_str());
			YYABORT;
		    }
		    else if (!TOP_OF_STACK->append_attr(name, type, $3)) {
			ostringstream msg;
			msg << "`" << name << "' previously defined.";
			parse_error((parser_arg *)arg, msg.str().c_str());
			YYABORT;
		    }
		}
;

floats:		FLOAT
		{
		    type = "Float64";
		    BESDEBUG("h4", "Adding FLOAT: " << TYPE_NAME_VALUE($1) << endl);
		    if (!check_float64($1)) {
			ostringstream msg;
			msg << "`" << $1 << "' is not a Float64 value.";
			parse_error((parser_arg *)arg, msg.str().c_str());
			YYABORT;
		    }
		    else if (!TOP_OF_STACK->append_attr(name, type, $1)) {
			ostringstream msg;
			msg << "`" << name << "' previously defined.";
			parse_error((parser_arg *)arg, msg.str().c_str());
			YYABORT;
		    }
		}
		| floats ',' FLOAT
		{
		    type = "Float64";
		    BESDEBUG("h4", "Adding FLOAT: " << TYPE_NAME_VALUE($3) << endl);
		    if (!check_float64($3)) {
			ostringstream msg;
			msg << "`" << $1 << "' is not a Float64 value.";
			parse_error((parser_arg *)arg, msg.str().c_str());
			YYABORT;
		    }
		    else if (!TOP_OF_STACK->append_attr(name, type, $3)) {
			ostringstream msg;
			msg << "`" << name << "' previously defined.";
			parse_error((parser_arg *)arg, msg.str().c_str());
			YYABORT;
		    }
		}
;

floatints:	float_or_int
		{
		    type = "Float64";
		    BESDEBUG("h4", "Adding FLOAT: " << TYPE_NAME_VALUE($1) << endl);
		    if (!check_float64($1)) {
			ostringstream msg;
			msg << "`" << $1 << "' is not a Float64 value.";
			parse_error((parser_arg *)arg, msg.str().c_str());
			YYABORT;
		    }
		    else if (!TOP_OF_STACK->append_attr(name, type, $1)) {
			ostringstream msg;
			msg << "`" << name << "' previously defined.";
			parse_error((parser_arg *)arg, msg.str().c_str());
			YYABORT;
		    }
		}
		| floatints ',' float_or_int
		{
		    type = "Float64";
		    BESDEBUG("h4", "Adding FLOAT: " << TYPE_NAME_VALUE($3) << endl);
		    if (!check_float64($3)) {
			ostringstream msg;
			msg << "`" << $3 << "' is not a Float64 value.";
			parse_error((parser_arg *)arg, msg.str().c_str());
			YYABORT;
		    }
		    else if (!TOP_OF_STACK->append_attr(name, type, $3)) {
			ostringstream msg;
			msg << "`" << name << "' previously defined.";
			parse_error((parser_arg *)arg, msg.str().c_str());
			YYABORT;
		    }
		}
;

float_or_int:   FLOAT | INT
;

strs:		STR
		{
		    type = "String";
		    BESDEBUG("h4", "Adding STR: " << TYPE_NAME_VALUE($1) << endl);
		    if (!TOP_OF_STACK->append_attr(name, type, $1)) {
			ostringstream msg;
			msg << "`" << name << "' previously defined.";
			parse_error((parser_arg *)arg, msg.str().c_str());
			YYABORT;
		    }
#if 0
		    if (name=="GridName" || name=="SwathName" || name=="PointName") {
		      // Strip off quotes in new ID
		      string newname = $1+1;
		      newname.erase(newname.end()-1);
		      // and convert embedded spaces to _
		      string::size_type space = 0;
		      while((space = newname.find_first_of(' ', space)) != newname.npos) {
			     newname[space] = '_';
		      }
 		      SECOND_IN_STACK->attr_alias(newname, last_grid_swath);
		    }
#endif		    
		}
                | strs ',' STR
		{
		    type = "String";
		    BESDEBUG("h4", "Adding STR: " << TYPE_NAME_VALUE($3) << endl);
		    if (!TOP_OF_STACK->append_attr(name, type, $3)) {
			ostringstream msg;
			msg << "`" << name << "' previously defined.";
			parse_error((parser_arg *)arg, msg.str().c_str());
			YYABORT;
		    }
		}
;

%%

// This function is required for linking, but DODS uses its own error
// reporting mechanism.

void
hdfeoserror(parser_arg *, const char *)
{
}

// I wrote this because I thought at one point that it was the solution to 
// having some of the libdap methods change out from under the hdfeos code
// here. In the end, I added a new find method to the AttrTable class that 
// doesn't require FQNs for the paths. (see AttrTable::recurrsive_find)
static string
build_fqn(AttrTable *at, string fqn)
{
    // The strange behavior at the top level is because the top level of an
    // AttrTable (i.e. the DAS) is anonymous. Another bad design... jhrg 2/8/06
    if (!at || !at->get_parent() || at->get_name().empty())
        return fqn;
    else
        return build_fqn(at->get_parent(), at->get_name() + string(".") + fqn);
}

static void 
process_group(parser_arg * arg, const string & id)
{
    AttrTable *at;
    BESDEBUG("h4", "Processing ID: " << id << endl);
    /* If we are at the outer most level of attributes, make
       sure to use the AttrTable in the DAS. */
    if (STACK_EMPTY) {
        at = ATTR_OBJ(arg)->get_attr_table(id);
        if (!at)
            at = ATTR_OBJ(arg)->append_container(id);
    } else {
        at = TOP_OF_STACK->get_attr_table(id);
        if (!at)
            at = TOP_OF_STACK->append_container(id);
    }

    if (id.find("GRID_") == 0 || id.find("SWATH_") == 0 ||
        id.find("POINT_") == 0) {
        last_grid_swath = id;
    }

    PUSH(at);
    BESDEBUG("h4", " Pushed attr_tab: " << at << endl);
}
