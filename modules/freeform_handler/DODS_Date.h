
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

// $Log: DODS_Date.h,v $
// Revision 1.8  2001/09/28 23:19:43  jimg
// Merged with 3.2.3.
//
// Revision 1.7.2.1  2001/05/23 18:25:49  dan
// Modified to support year/month date representations,
// and to support ISO8601 output formats.
//
// Revision 1.6  1999/07/22 21:28:08  jimg
// Merged changes from the release-3-0-2 branch
//
// Revision 1.5.6.1  1999/06/01 15:38:06  jimg
// Added code to parse and return floating point dates.
//
// Revision 1.5  1999/05/04 02:55:35  jimg
// Merge with no-gnu
//
// Revision 1.4.6.1  1999/05/01 04:40:30  brent
// converted old String.h to the new std C++ <string> code
//
// Revision 1.4  1999/01/08 22:08:18  jimg
// Fixed doc++ comments.
//
// Revision 1.3  1999/01/05 00:34:45  jimg
// Removed string class; replaced with the GNU String class. It seems those
// don't mix well.
// Switched to simpler method names.
// Added the date_format enumerated type.
//
// Revision 1.2  1998/12/30 02:01:12  jimg
// Added class invariant.
//
// Revision 1.1  1998/12/28 19:08:26  jimg
// Initial version of the DODS_Date object
//

#ifndef _dods_date_h
#define _dods_date_h


#include <time.h>

#include <string>

#include "BaseType.h"
#include "date_proc.h"

using namespace libdap ;

/** Useful constant values. */

const double seconds_per_day = 86400.0;
const double seconds_per_hour = 3600.0;
const double seconds_per_minute = 60.0;

/** Constants used to denote different supported date formats. #ymd# is a
    year---month---day format, #yd# is a year---year-day format. */

enum date_format {
    unknown_format,
    ymd,
    yd,
    ym,
    decimal,
    iso8601
};

/** The DODS Date object. This provides a way to translate between local
    representations of dates and the DODS standard representation(s). The
    DODS\_Date object provides constructors, accessors and comparison
    operations; DODS servers which support the DODS standard representation
    of dates must implement CE functions that make use of this object.
    
    @author James Gallagher */

class DODS_Date {
private:
    long _julian_day;
    int _year;
    int _month;
    int _day;
    int _day_number;
    date_format _format;

    void parse_fractional_time(string date);
    void parse_integer_time(string date);
    void parse_iso8601_time(string date);

public:
    /** @name Constructors */

    //@{
    /** Create an empty date. Set the date using one of the #set_date#
	mfuncs.

	@see set_date() */
    DODS_Date();
    
    /** Build a DODS\_Date by parsing the string #date_str#. If #date_str# is
	of the form `yyyy/ddd' assume that it is in year and day-number
	format. If it is of the form `yyyy/mm/dd' assume it is in year, month
	and day format. Note that if `yyyy' is `98' that means year 98 A.D.,
	not 1998.

	@param date_str A string containing the date. */
    DODS_Date(string date_str);

    /** Build a DODS\_Date by parsing the DODS string contained in #arg#.

	Throws Error if #arg# is not a DODS Str object.

	@param arg A DODS string containing the date.
	@see DODS_Date(string). */
    DODS_Date(BaseType *arg);

    /** Build a DODS\_Date using year and day-number values. This constructor
	assumes that the two integers are the year and day-number,
	respectively.

	@param year The year. `98' is 98 A.D., not 1998. 
	@param day_num The day-number, 1 Jan is day 1. */
    DODS_Date(int year, int day_num);

    /** Build a DODS\_Date using year, month and day values. 

	@param year The year. As with the other constructors, does not prefix
	1900 to two digit years.
	@param month The month of the year; 1 == January, ..., 12 == December.
	@param day The day of the month; 1, ..., \{31, 30, 29, 28\}. */
    DODS_Date(int year, int month, int day);
    DODS_Date(int year, int month, int day, date_format format);
    //@}

    /** @name Assignment */
    //@{
    /** Parse the string and assign the value to this object. 
	@see DODS_Date(string) */
    void set(string date);

    /** Parse the DODS string and assign the value to this object. 
	@see DODS_Date(BaseType *arg) */
    void set(BaseType *arg);

    /** Assign the date using the two integers.
	@see DODS_Date(int year, int day_number) */
    void set(int year, int day_number);

    /** Assign the date using three integers.
	@see DODS_Date(int year, int month, int day) */
    void set(int year, int month, int day);

    /** Assign the date using three integers and a date_format
	enumeration.
	@see DODS_Date(int year, int month, int day, date_format format) */
    void set(int year, int month, int day, date_format format);

    //@}

    /** @name Access */
    //@{
    /** Get the string representation for this date. By default the y/m/d
	format is used. To get the year/year-day format use #yd# for the
	value of #format#.
	
	Throws Error if #format# is not #ymd# or #yd#.

	@param format The format of the date.
	@see date_format.
	@return The date's string representation. */
    string get(date_format format = ymd) const;

    /** @return The year in years A.D. */
    int year() const;

    /** @return The month of the year (1 == January, ..., 12 == December). */
    int month() const;

    /** @return The day of the month (1, ... \{28, 29, 30, 31\}). */
    int day() const;

    /** @return The day-number of the year (1 == 1 Jan). */
    int day_number() const;

    /** @return The Julian day number for this date. */
    long julian_day() const;

    /** @return The format type. */
    date_format format() const;

    /** Return the number of seconds since 00:00:00 UTC 1 Jan 1970. If the
	date is before 1 Jan 1970, return DODS\_UINT\_MAX. If the date is too
	late to represent as seconds since 1 Jan 1970, return
	DODS\_UINT\_MAX. Each day starts at 00:00:00 UTC.

	@return The date in seconds since 1 Jan 1970.
        @see dods-limits.h
	@see time.h
 	@see mktime(3) */
    time_t unix_time() const;

    /* Get the date as a real number. The year is the whole number and days
       are as fractions of a year. E.G.: 1998.5 is approximately June, 1998.

       @return The date (year, month and day) as a real number. */
    double fraction() const;
    //@}

    /** @name Relational operators */
    //@{
    /// Equality
    friend int operator==(DODS_Date &d1, DODS_Date &d2);

    /// Inequality
    friend int operator!=(DODS_Date &d1, DODS_Date &d2);

    /// Less than
    friend int operator<(DODS_Date &d1, DODS_Date &d2);

    /// Greater than
    friend int operator>(DODS_Date &d1, DODS_Date &d2);

    /// Less than or equal
    friend int operator<=(DODS_Date &d1, DODS_Date &d2);

    /// Greater than or equal
    friend int operator>=(DODS_Date &d1, DODS_Date &d2);
    //@}

    /** Class invariant.

	@return True for a valid instance, otherwise false. */
    bool OK() const;
};

#endif // _dods_date_h
