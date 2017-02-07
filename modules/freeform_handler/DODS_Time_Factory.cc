
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

// Implementation of the DODS_Time_Factory class

#include "config_ff.h"

static char rcsid[] not_used ="$Id$";


#include <string>

#include "AttrTable.h"
#include "Error.h"
#include "InternalErr.h"
#include "dods-datatypes.h"
#include "util.h"
#include "util_ff.h"

#include "DODS_Time_Factory.h"

// attribute_name defaults to "DODS_TIME".
DODS_Time_Factory::DODS_Time_Factory(DDS &dds, const string &attribute_name)
{
    // Read the names of the variables which encode hours, minutes and
    // seconds from the DAS. These are contained in the DODS_Time attribute
    // container.

    AttrTable *at = dds.get_attr_table().find_container(attribute_name);
    if (!at)
	throw Error(string("DODS_Time_Factory requires that the ")
		    + attribute_name + string(" attribute be present."));

    string hours_name = at->get_attr("hours_variable");
    string mins_name = at->get_attr("minutes_variable");
    string secs_name = at->get_attr("seconds_variable");
    string gmt = at->get_attr("gmt_time");

    // If the gmt attribute is present that meanas that the times are GMT/UTC
    // times. Set the _gmt flag true, otherwise set it false.

    downcase(gmt);
    if (gmt == "true")
	_gmt = true;
    else
	_gmt = false;

    // Now check that these variables actually exist and that they have
    // sensible types.

    _hours = dds.var(hours_name);
    if (_hours && !is_integer_type(_hours))
	throw Error("DODS_Time_Factory: The variable used for hours must be an integer.");

    _minutes = dds.var(mins_name);
    if (_minutes && !is_integer_type(_minutes))
	throw Error("DODS_Time_Factory: The variable used for minutes must be an integer.");

    _seconds = dds.var(secs_name);
    if (_seconds && !(is_integer_type(_seconds) || is_float_type(_seconds)))
	throw Error("DODS_Time_Factory: The variable used for seconds must be an integer.");
}

DODS_Time
DODS_Time_Factory::get()
{
    return DODS_Time(get_integer_value(_hours), get_integer_value(_minutes),
		     get_float_value(_seconds), _gmt);
}
