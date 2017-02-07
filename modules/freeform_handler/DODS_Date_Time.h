
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

#ifndef _dods_date_time_h
#define _dods_date_time_h


#include "DODS_Date.h"
#include "DODS_Time.h"

/** The DODS Date/Time class is used to represent and compute with combined
    date and time values. 

    @see DODS_Date
    @see DODS_Time
    @author James Gallagher */

class DODS_Date_Time {
 private:
    DODS_Date _date;
    DODS_Time _time;

    void parse_fractional_time(string date_time);
 public:
    /** @name Constructors */
    //@{
    /** Create an empty instance. Will not pass the class invariant. */
    DODS_Date_Time();

    /** Create an instance using DODS\_Date and DODS\_Time objects. 

	@param d A DODS\_Date object.
	@param t A DODS\_Time object. */
    DODS_Date_Time(DODS_Date d, DODS_Time t);

    /** Create an instance using the string representation of a
	DODS\_Date\_Time.

	@param date_time The date/time string. */
    DODS_Date_Time(string date_time);

    /** Create an instance using the string representation of a
	DODS\_Date\_Time contained in a DODS Str.

	@param date_time The date/time string (in a DODS Str pointer). */
    DODS_Date_Time(BaseType *date_time);

    /** Create an instance using numeric values for the year, ..., seconds.

	@param y The year number, 1 is 1 A.D.
	@param m The month, 1 is January, ..., 12 is December.
	@param d The day.
	@param hh The hour, 0...23.
	@param mm The minute, 0...59.
	@param ss The seconds, 0.0, ..., 59.999...
	@param gmt True if the time is a GMT time, false otherwise. */
    DODS_Date_Time(int y, int m, int d, int hh, int mm = 0, double ss = 0.0,
		   bool gmt = false); 

    /** Create an instance using numeric values for the year, ..., seconds.

	@param y The year number, 1 is 1 A.D.
	@param yd The year-day, 1 is 1 January, 365/366 is 31 December.
	@param hh The hour, 0...23.
	@param mm The minute, 0...59.
	@param ss The seconds, 0.0, ..., 59.999... 
	@param gmt True if the time is a GMT time, false otherwise. */
    DODS_Date_Time(int y, int yd, int hh, int mm = 0, double ss = 0.0, 
		   bool gmt = false); 
    //@}

    /** @name Assignment */
    //@{
    /** Set an instance using DODS\_Date and DODS\_Time objects. 

	@param d A DODS\_Date object.
	@param t A DODS\_Time object. */
    void set(DODS_Date d, DODS_Time t);

    /** Set an instance using the string representation of a
	DODS\_Date\_Time. 

	@param date_time The date/time string. */
    void set(string date_time);

    /** Set an instance using the string representation of a
	DODS\_Date\_Time contained in a DODS Str.

	@param date_time The date/time string (in a DODS Str pointer). */
    void set(BaseType *date_time);

    /** Set an instance using numeric values for the year, ..., seconds.

	@param y The year number, 1 is 1 A.D.
	@param m The month, 1 is January, ..., 12 is December.
	@param d The day.
	@param hh The hour, 0...23.
	@param mm The minute, 0...59.
	@param ss The seconds, 0.0, ..., 59.999...
	@param gmt True if the time is a GMT time, false otherwise. */
    void set(int y, int m, int d, int hh, int mm = 0, 
		       double ss = 0.0, bool gmt = false);

    /** Set an instance using numeric values for the year, ..., seconds.

	@param y The year number, 1 is 1 A.D.
	@param yd The year-day, 1 is 1 January, 365/366 is 31 December.
	@param hh The hour, 0...23.
	@param mm The minute, 0...59.
	@param ss The seconds, 0.0, ..., 59.999... 
	@param gmt True if the time is a GMT time, false otherwise. */
    void set(int y, int yd, int hh, int mm = 0, double ss = 0.0, 
		       bool gmt = false);
    //@}

    /** @name Access */
    //@{
    /** @return The year in years A.D. */
    int year() const;

    /** @return The month of the year (1 == January, ..., 12 == December). */
    int month() const;

    /** @return The day of the month (1, ... {28, 29, 30, 31}. */
    int day() const;

    /** @return The day-number of the year (1 = 1 Jan). */
    int day_number() const;

    /** @return The number of hours. */
    int hours() const;

    /** @return The number of minutes. */
    int minutes() const;

    /** @return The number of seconds. */
    double seconds() const;

    /** @return True if the time is a GMT time, false otherwise. */
    bool gmt() const;

    /** Get the string representation of the date/time. If #format# is #ymd#,
	return the date-time using yyyy/mm/dd:hh:mm:ss notation, if it is #yd#,
	use yyyy/ddd... notation. If #gmt# is true, append GMT to times that
	are GMT times, if false, don't (ever) append the GMT suffix.

	@param format The format of the date-part.
	@param gmt Append the GMT suffix to GMT times.
	@return The string representation f the date-time. */ 
    string get(date_format format = ymd, bool gmt = true) const;

    /** @return The Julian day number for this date and time. The time part
	of the day is represented as a fraction. */
    double julian_day() const;

    /** Return the number of seconds since 00:00:00 UTC 1 Jan 1970. If the
	date is before 1 Jan 1970, return DODS\_UINT\_MAX. If the date is too
	late to represent as seconds since 1 Jan 1970, return
	DODS\_UINT\_MAX. Each day starts at 00:00:00 UTC.

	@return The date in seconds since 1 Jan 1970.
        @see dods-limits.h
	@see time.h
 	@see mktime(3) */
    time_t unix_time() const;
    //@}

    /** @name Relational operators */
    //@{
    /// Equality
    friend int operator==(DODS_Date_Time &t1, DODS_Date_Time &t2);

    /// Inequality
    friend int operator!=(DODS_Date_Time &t1, DODS_Date_Time &t2);

    /// Less-than
    friend int operator<(DODS_Date_Time &t1, DODS_Date_Time &t2);

    /// Greater-than
    friend int operator>(DODS_Date_Time &t1, DODS_Date_Time &t2);

    /// Less-than or Equal-to
    friend int operator<=(DODS_Date_Time &t1, DODS_Date_Time &t2);

    /// Greater-than or Equal-to
    friend int operator>=(DODS_Date_Time &t1, DODS_Date_Time &t2);
    //@}

    /** Get the value of epsilon used for equality tests of time. */
    double get_epsilon() const;

    /** Set the value of epsilon used for equality tests of time. By default
	the value is 0.000001 (10e-6). 

	NB: This should rarely be changed. */
    void set_epsilon(double eps);

    /** Class invariant. 

	@return True if the instance is valid, false otherwise. */
    bool OK() const;

};

// $Log: DODS_Date_Time.h,v $
// Revision 1.4  2000/10/11 19:37:55  jimg
// Moved the CVS log entries to the end of files.
// Changed the definition of the read method to match the dap library.
// Added exception handling.
// Added exceptions to the read methods.
//
// Revision 1.3  1999/07/22 21:28:09  jimg
// Merged changes from the release-3-0-2 branch
//
// Revision 1.2.6.1  1999/06/01 15:38:06  jimg
// Added code to parse and return floating point dates.
//
// Revision 1.2  1999/05/04 02:55:35  jimg
// Merge with no-gnu
//
// Revision 1.1.6.1  1999/05/01 04:40:30  brent
// converted old String.h to the new std C++ <string> code
//
// Revision 1.1  1999/01/08 22:08:18  jimg
// Fixed doc++ comments.
//

#endif // _dods_date_time_h
