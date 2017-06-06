
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

#ifndef _dods_time_h
#define _dods_time_h


#include <string>

#include "dods-datatypes.h"
#include "BaseType.h"

using namespace libdap ;

/** The DODS Time object. This provides a way to translate between various
    representations of time. This class is designed to be compatible with the
    DODS\_Date class so that dates and times may be used together in relational
    expressions. 

    NB: The seconds part of a DODS\_Time may contain fractional components.
    To test for equality of times, this class tests to see if the second time
    falls within a neighborhood around the first time the size of which is
    determined by an epsilon value (1.0e-6 by default). Thus seconds may have
    precision down to the micro-second (depending on the underlying
    hardware). See the #set_epsilon()# and #get_epsilon()# mfuncs. The
    epsilon value is a static class member; the same value is used for all
    instances of the class, and when changed, is changed at that time for all
    instances.

    @see DODS_Date
    @see DODS_Date_Time()
    @author James Gallagher */

class DODS_Time {
private:
    dods_uint32 _hours;		// 0--23; regular wall time
    dods_uint32 _minutes;	// 0--59
    double _seconds;		// 0--59.9...
    double _sec_since_midnight;
    bool _gmt;

    static double _eps;		// defined as 1.0e-6 in DODS_Time.cc

protected:

public:
    /** @name Constructors */

    //@{
    /** Build a DODS\_Time by parsing the String #time#. The string may be
	either of the form `hh:mm:ss' or `hh:mm'. In the later case the
	seconds are assumed to be zero. In addition, the string may have the
	suffix `GMT' or `UTC' indicating that the time is in Greenwich Mean
	Time.

	@param time The time string. */
    DODS_Time(string time);

    /** Build a DODS\_Time by parsing the DODS Str #arg#.

	@see DODS_Time(string).
	@param arg A DODS Str variable, passed as a BaseType pointer. */
    DODS_Time(BaseType *arg);

    /** Build a DODS\_Time.

	@param hh The hours, 0-23.
	@param mm The minutes, 0-59.
	@param gmt True if the time is a GMT time, false otherwise. */
    DODS_Time(dods_uint32 hh, dods_uint32 mm, bool gmt = false);

    /** Build a DODS\_Time.

	@param hh The hours, 0--23.
	@param mm The minutes, 0--59.
	@param ss The seconds, 0--59. May contain a fractional component.
	@param gmt True if the time is a GMT time, false otherwise. */
    DODS_Time(dods_uint32 hh, dods_uint32 mm, double ss, bool gmt = false);

    /** Build an empty DODS\_Time.

	NB: This won't pass the class invariant. */
    DODS_Time();
    //@}

    /** @name Assignment */

    //@{
    /** Set the value by parsing the string #time#.

	@param time The time string.
	@see DODS_Time(string). */
    void set(string time);

    /** Set the value by parsing the DODS Str #arg#.

	@param arg The time string wrapped in a DODS string.
	@see DODS_Time(BaseType *). */
    void set(BaseType *arg);

    /** Set the value using the given numeric values.

	@param hh The hours, 0-23.
	@param mm The minutes, 0-59.
	@param gmt True if the time is a GMT time, false otherwise.
	@see DODS_Time(int, int, bool). */
    void set(int hh, int mm, bool gmt = false);

    /** Set the value using the given numeric values.

	@param hh The hours, 0--23.
	@param mm The minutes, 0--59.
	@param ss The seconds, 0--59. May contain a fractional component.
	@param gmt True if the time is a GMT time, false otherwise. 
	@see DODS_Time(int, int, double, bool). */
    void set(int hh, int mm, double ss, bool gmt = false);
    //@}

    /** @name Access */

    //@{
    /** Get the string representation of time.

	@param gmt If true append the suffix `GMT' to the time if it a GMT
	time. If false, ignore gmt. True by default.
	@return The string representation for this time. */
    string get(bool gmt = true) const;

    /** @return The number of hours. */
    int hours() const;

    /** @return The number of minutes. */
    int minutes() const;

    /** @return The number of seconds. */
    double seconds() const;

    /** @return True if the time is a GMT time, false otherwise. */
    bool gmt() const;

    /** Get the number of seconds since midnight.
	@return The number of seconds since midnight. */
    double seconds_since_midnight() const;

    /** Get the time as a fraction of a day.

	@return The daytime as a fraction. */
    double fraction() const;
    //@}

    /** @name Relational operators */
    //@{
    /// Equality
    friend int operator==(DODS_Time &t1, DODS_Time &t2);

    /// Inequality
    friend int operator!=(DODS_Time &t1, DODS_Time &t2);

    /// Less-than
    friend int operator<(DODS_Time &t1, DODS_Time &t2);

    /// Greater-than
    friend int operator>(DODS_Time &t1, DODS_Time &t2);

    /// Less-than or Equal-to
    friend int operator<=(DODS_Time &t1, DODS_Time &t2);

    /// Greater-than or Equal-to
    friend int operator>=(DODS_Time &t1, DODS_Time &t2);
    //@}

    /** Class invariant.

	@return True for a valid instance, otherwise false. */
    bool OK() const;

    /** Get the value of epsilon used for equality tests. */
    double get_epsilon() const;

    /** Set the value of epsilon used for equality tests. By default the
	value is 0.000001 (10e-6). */
    void set_epsilon(double eps);

};

// $Log: DODS_Time.h,v $
// Revision 1.7  2003/02/10 23:01:52  jimg
// Merged with 3.2.5
//
// Revision 1.6.2.1  2002/01/22 02:19:35  jimg
// Fixed bug 62. Users built fmt files that used types other than int32
// for date and time components (e.g. int16). I fixed the factory classes
// so that DODS_Date and DODS_Time objects will be built correctly when
// any of the integer (or in the case of seconds, float) data types are
// used. In so doing I also refactored the factory classes so that code
// duplication was reduced (by using inhertiance).
// Added two tests for the new capabilities (see date_time.1.exp, the last
// two tests).
//
// Revision 1.6  2000/10/11 19:37:55  jimg
// Moved the CVS log entries to the end of files.
// Changed the definition of the read method to match the dap library.
// Added exception handling.
// Added exceptions to the read methods.
//
// Revision 1.5  1999/07/22 21:28:09  jimg
// Merged changes from the release-3-0-2 branch
//
// Revision 1.4.6.1  1999/06/01 15:38:06  jimg
// Added code to parse and return floating point dates.
//
// Revision 1.4  1999/05/04 02:55:35  jimg
// Merge with no-gnu
//
// Revision 1.3.6.1  1999/05/01 04:40:30  brent
// converted old String.h to the new std C++ <string> code
//
// Revision 1.3  1999/01/08 22:08:19  jimg
// Fixed doc++ comments.
//
// Revision 1.2  1999/01/05 00:37:28  jimg
// Removed string class; replaced with the GNU String class. It seems those
// don't mix well.
// Switched to simpler method names.
// Added DOC++ Comments.
//
// Revision 1.1  1998/12/28 19:07:33  jimg
// Initial version of the DODS_Time object
//

#endif // _dods_time_h
