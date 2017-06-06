
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of ff_handler a FreeForm API handler for the OPeNDAP
// DAP2 data server.

// Copyright (c) 2005 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
//
// This is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your
// option) any later version.
// 
// This software is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

// (c) COPYRIGHT URI/MIT 1998
// Please read the full copyright statement in the file COPYRIGHT.
//
// Authors:
//      jhrg,jimg       James Gallagher (jgallagher@gso.uri.edu)

// Implementation of the DODS_Date_Factory class

#include "config_ff.h"

static char rcsid[] not_used = "$Id$";


#include <cstdlib>
#include <string>

#include <AttrTable.h>
#include <Error.h>
#include <dods-datatypes.h>
#include <dods-limits.h>

#include "DODS_Date_Factory.h"
#include "util_ff.h"

// attribute_name defaults to "DODS_Date." 1/21/2002 jhrg
DODS_Date_Factory::DODS_Date_Factory(DDS &dds, const string &attribute_name)
{
    // Read the names of the variables which encode year, month and
    // day from the DAS. These are contained in the DODS_Date attribute
    // container. 
    
    AttrTable *at = dds.get_attr_table().find_container(attribute_name);
    if (!at)
	throw Error(unknown_error,
		    string("DODS_Date_Factory requires that the ")
		    + attribute_name 
		    + string("DODS_Date attribute be present."));

    string year_name = at->get_attr("year_variable");
    string year_base = at->get_attr("year_base");
    string month_name = at->get_attr("month_variable");
    string day_name = at->get_attr("day_variable");
    string year_day_name = at->get_attr("year_day_variable");
    string month_day_name = at->get_attr("month_day_const");

    // Check to see if these dates are ymd or yd type dates.

    if (year_day_name == "" && day_name != "")
	_format = ymd;
    else if (year_day_name != "" && day_name == "")
	_format = yd;
    else if (year_day_name == "" && day_name == "")
      _format = ym;
    else
	throw Error(unknown_error,
"DODS_Date_Factory requires that one, and only one, of the attributes\n\
day_variable or year_day_variable be present.");

    // Extract year_base if it's present, else set year base to zero.

    if (year_base == "")
	_year_base = 0;		// changed NULL to 0. 5/23/2001 jhrg
    else {
	const char *c = year_base.c_str();
	char *c2;
	_year_base = strtol(c, &c2, 0);
	if (c == c2 || _year_base == DODS_LONG_MAX || _year_base == DODS_LONG_MIN)
	    throw Error(unknown_error,
"The year_base attribute value cannot be converted to a valid integer.");
    }

    // Extract month_day if it's present, else set month_day to 15 (default).

    _month_day = 15;
    if (_format == ym && month_day_name != "")
      {
	const char *c = month_day_name.c_str();
	char *c2;
	_month_day = strtol(c, &c2, 0);
	if (c == c2 || _month_day == DODS_LONG_MAX || _month_day == DODS_LONG_MIN)
	    throw Error(unknown_error,
"The month_day attribute value cannot be converted to a valid integer.");
    }

    // Now check that these variables actually exist and that they have
    // sensible types.

    _year = dds.var(year_name);
    if ((_year->type() != dods_int16_c) && (_year->type() != dods_uint16_c) &&
	(_year->type() != dods_int32_c) && (_year->type() != dods_uint32_c))
	throw Error(unknown_error, "DODS_Date_Factory: The variable used for the year must be an integer.");

    switch (_format) {
      case ymd: {
	  _month = dds.var(month_name);
	  if (!is_integer_type(_month))
	      throw Error(unknown_error, 
"DODS_Date_Factory: The variable used for the month must be an integer.");

	  _day = dds.var(day_name);
	  if (!is_integer_type(_day))
	      throw Error(unknown_error, 
"DODS_Date_Factory: The variable used for days must be an integer.");
	  _year_day = NULL;
	  break;
      }

      case yd: {
	  _month = NULL;
	  _day = NULL;
	  _year_day = dds.var(year_day_name);
	  if (!is_integer_type(_year))
	      throw Error(unknown_error, 
"DODS_Date_Factory: The variable used for the year-day must be an integer.");
	  break;
      }
      case ym: {
	_month = dds.var(month_name);
	if (!is_integer_type(_month))
	  throw Error(unknown_error, 
		      "DODS_Date_Factory: The variable used for the month must be an integer.");
	_day = NULL;
	_year_day = NULL;
	break;
      }

      default:
	throw Error(unknown_error,
"DODS_Date_Factory: Not able to figure out the date format.");
	break;
    }
}

DODS_Date
DODS_Date_Factory::get()
{
    dods_uint32 year = get_integer_value(_year);

    switch (_format) {
      case ymd: {
	  dods_uint32 month = get_integer_value(_month);
	  dods_uint32 day = get_integer_value(_day);

	  return DODS_Date(year + _year_base, month, day);
	  break;
      }

      case yd: {
	  dods_uint32 year_day = get_integer_value(_year_day);

	  return DODS_Date(year + _year_base, year_day);
	  break;
      }

      case ym: {
	  dods_uint32 month = get_integer_value(_month);

	  int day = _month_day;
	  date_format fmt = ym;

	  return DODS_Date(year + _year_base, month, day, fmt);
	  break;
      }

      default:
	throw Error(unknown_error, 
"DODS_Date_Factory: Unknown date format, should never get here!");
    }
}

// $Log: DODS_Date_Factory.cc,v $
// Revision 1.6  2003/02/10 23:01:52  jimg
// Merged with 3.2.5
//
// Revision 1.5  2001/09/28 23:19:43  jimg
// Merged with 3.2.3.
//
// Revision 1.4.2.3  2002/01/22 02:19:35  jimg
// Fixed bug 62. Users built fmt files that used types other than int32
// for date and time components (e.g. int16). I fixed the factory classes
// so that DODS_Date and DODS_Time objects will be built correctly when
// any of the integer (or in the case of seconds, float) data types are
// used. In so doing I also refactored the factory classes so that code
// duplication was reduced (by using inhertiance).
// Added two tests for the new capabilities (see date_time.1.exp, the last
// two tests).
//
// Revision 1.4.2.2  2001/05/23 19:04:31  jimg
// Changed NULL to 0 in an assignment to _year_base (which is an int) because
// of a g++ warning.
//
// Revision 1.4.2.1  2001/05/23 18:26:20  dan
// Modified to support year/month date representations,
// and to support ISO8601 output formats.
//
// Revision 1.4  2000/10/11 19:37:55  jimg
// Moved the CVS log entries to the end of files.
// Changed the definition of the read method to match the dap library.
// Added exception handling.
// Added exceptions to the read methods.
//
// Revision 1.3  2000/08/31 22:16:53  jimg
// Merged with 3.1.7
//
// Revision 1.2.8.1  2000/08/03 20:18:57  jimg
// Removed config_dap.h and replaced it with config_ff.h (in *.cc files;
// neither should be included in a header file).
// Changed code that calculated leap year information so that it uses the
// functions in date_proc.c/h.
//
// Revision 1.2  1999/05/04 02:55:35  jimg
// Merge with no-gnu
//
// Revision 1.1.10.1  1999/05/01 04:50:20  brent
// converted old String.h to the new std C++ <string> code
//
// Revision 1.1  1999/01/22 20:44:34  jimg
// Added
//
