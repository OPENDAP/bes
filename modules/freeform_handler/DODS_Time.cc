
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

// Implementation of the DODS Time class

#include "config_ff.h"

static char rcsid[] not_used ="$Id$";

#include <string> 
#include <sstream>
#include <iomanip>

#include "BaseType.h"
#include "DODS_Time.h"
#include "debug.h" 
#include "Error.h"

using namespace std;

double DODS_Time::_eps = 1.0e-6;

static string time_syntax_string = \
"Invalid time: times must be given as hh:mm or hh:mm:ss with an optional\n\
suffix of GMT or UTC. In addition, 0 <= hh <=23, 0 <= mm <= 59 and\n\
0 <= ss <= 59.999999";

static inline double
compute_ssm(int hh, int mm, double ss)
{
    return ((hh * 60 + mm) * 60) + ss;
}

static string
extract_argument(BaseType *arg)
{
#ifndef TEST
    if (arg->type() != dods_str_c)
	throw Error(malformed_expr, "A DODS string argument is required.");
    
    // Use String until conversion of String to string is complete. 9/3/98
    // jhrg
    string *sp = NULL;
    arg->buf2val((void **)&sp);
    string s = sp->c_str();
    delete sp;

    DBG(cerr << "s: " << s << endl);

    return s;
#else
    return "";
#endif
}

bool
DODS_Time::OK() const
{
    // _hours and _minutes are unsigned.
    return /* _hours >= 0 && */ _hours <= 23
	/* && _minutes >= 0 */ && _minutes <= 59
	&& _seconds >= 0.0 && _seconds < 60.0;
}

double
DODS_Time::fraction() const
{
    return ((_hours + ((_minutes + (_seconds / 60.0)) / 60.0)) / 24.0);
}

// Public mfincs.

// Don't test this ctor with OK since a null Time is not OK for use but we
// want to be able to make one and then call set_time(). 12/16/98 jhrg

DODS_Time::DODS_Time(): _hours(0), _minutes(0), _seconds(-1),
    _sec_since_midnight(-1), _gmt(false)
{
}

DODS_Time::DODS_Time(string time_str)
{
    set(time_str);
}

DODS_Time::DODS_Time(BaseType *arg)
{
    set(extract_argument(arg));
}

DODS_Time::DODS_Time(dods_uint32 hh, dods_uint32 mm, bool gmt):
    _hours(hh), _minutes(mm), _seconds(0), _gmt(gmt)
{
    _sec_since_midnight = compute_ssm(hh, mm, 0);
#ifndef TEST
    if (!OK())
	throw Error(malformed_expr, time_syntax_string);
#endif
}

DODS_Time::DODS_Time(dods_uint32 hh, dods_uint32 mm, double ss, bool gmt):
    _hours(hh), _minutes(mm), _seconds(ss), _gmt(gmt)
{
    _sec_since_midnight = compute_ssm(hh, mm, ss);
#ifndef TEST
    if (!OK())
	throw Error(malformed_expr, time_syntax_string);
#endif
}

void
DODS_Time::set(string time)
{
        // Parse the date_str.
    istringstream iss(time.c_str());
    char c;
    size_t pos1, pos2;
    iss >> _hours;

    pos1 = time.find(":");
    // If there is at least one colon, assume hours:minutes
    if (pos1 != time.npos) {
      iss >> c;
      iss >> _minutes;

      // If there are two colons, assume hours:minutes:seconds.
      pos2 = time.rfind(":");
      if ((pos2 != time.npos) && (pos1 != pos2)) {
	iss >> c;
	iss >> _seconds;
      }  
      else _seconds = 0;
    }
    else {
      // If there are no colons, assume hours only, set others to 0.
      _minutes = 0;
      _seconds = 0;
    }
    _sec_since_midnight = compute_ssm(_hours, _minutes, _seconds);

    string gmt;
    iss >> gmt;
    if (gmt == "GMT" || gmt == "gmt" || gmt == "UTC" 
	|| gmt == "utc")
	_gmt = true;
    else
	_gmt = false;

#ifndef TEST
    if (!OK())
	throw Error(malformed_expr, time_syntax_string);
#endif
}

void
DODS_Time::set(BaseType *arg)
{
    set(extract_argument(arg));
}    

void 
DODS_Time::set(int hh, int mm, bool gmt)
{
    set(hh, mm, 0, gmt);
}

void 
DODS_Time::set(int hh, int mm, double ss, bool gmt)
{
   _hours = hh;
   _minutes = mm;
   _seconds = ss;
   _gmt = gmt;
   _sec_since_midnight = compute_ssm(hh, mm, ss);

#ifndef TEST
    if (!OK())
	throw Error(malformed_expr, time_syntax_string);
#endif
}

double
DODS_Time::get_epsilon() const
{
    return _eps;
}

void
DODS_Time::set_epsilon(double eps)
{
    _eps = eps;
}

int
operator==(DODS_Time &t1, DODS_Time &t2)
{
    return t1.seconds_since_midnight()+t1._eps >= t2.seconds_since_midnight()
    && t1.seconds_since_midnight()-t1._eps <= t2.seconds_since_midnight();
}

int
operator!=(DODS_Time &t1, DODS_Time &t2)
{
    return !(t1 == t2);
}

// The relational ops > and < are possibly flaky. Note that the Intel machines
// and the Sparc machines *do* represent floating point numbers slightly
// differently. 

int
operator>(DODS_Time &t1, DODS_Time &t2)
{
    return t1.seconds_since_midnight() > t2.seconds_since_midnight();
}

int
operator>=(DODS_Time &t1, DODS_Time &t2)
{
    return t1 > t2 || t1 == t2;
}

int
operator<(DODS_Time &t1, DODS_Time &t2)
{
    return t1.seconds_since_midnight() < t2.seconds_since_midnight();
}

int
operator<=(DODS_Time &t1, DODS_Time &t2)
{
    return t1 < t2 || t1 == t2;
}

double
DODS_Time::seconds_since_midnight() const
{
    return _sec_since_midnight;
}

int
DODS_Time::hours() const
{
    return _hours;
}

int
DODS_Time::minutes() const
{
    return _minutes;
}

double
DODS_Time::seconds() const
{
    return _seconds;
}

bool
DODS_Time::gmt() const
{
    return _gmt;
}

string
DODS_Time::get(bool) const
{
    ostringstream oss;
    // Pad with leading zeros and use fixed fields of two chars for hours and
    // minutes. Make sure that seconds < 10 have a leading zero but don't
    // require their filed to have `precision' digits if they are all zero.
    oss << setfill('0') << setw(2) << _hours << ":" 
	<< setfill('0') << setw(2) << _minutes << ":" 
	<< setfill('0') << setw(2) << setprecision(6) << _seconds;

    if (_gmt)
	oss << " GMT";
    
    return oss.str();
}

// $Log: DODS_Time.cc,v $
// Revision 1.15  2004/07/09 17:54:24  jimg
// Merged with release-3-4-3FCS.
//
// Revision 1.11.4.2  2004/03/07 22:05:51  rmorris
// Final code changes to port the freeform server to win32.
//
// Revision 1.14  2004/02/04 20:50:08  jimg
// Build fixes. No longer uses Pix.
//
// Revision 1.13  2003/12/08 21:58:52  edavis
// Merge release-3-4 into trunk
//
// Revision 1.11.4.1  2003/06/29 05:35:10  rmorris
// Use standard template library headers correctly and add missing usage
// statements.
//
// Revision 1.12  2003/05/14 19:23:13  jimg
// Changed from strstream to sstream.
//
// Revision 1.11  2003/02/10 23:01:52  jimg
// Merged with 3.2.5
//
// Revision 1.10  2001/09/28 23:19:43  jimg
// Merged with 3.2.3.
//
// Revision 1.9.2.3  2002/12/18 23:30:42  pwest
// gcc3.2 compile corrections, mainly regarding the using statement
//
// Revision 1.9.2.2  2002/01/22 02:19:35  jimg
// Fixed bug 62. Users built fmt files that used types other than int32
// for date and time components (e.g. int16). I fixed the factory classes
// so that DODS_Date and DODS_Time objects will be built correctly when
// any of the integer (or in the case of seconds, float) data types are
// used. In so doing I also refactored the factory classes so that code
// duplication was reduced (by using inhertiance).
// Added two tests for the new capabilities (see date_time.1.exp, the last
// two tests).
//
// Revision 1.9.2.1  2001/05/23 18:26:31  dan
// Modified to support year/month date representations,
// and to support ISO8601 output formats.
//
// Revision 1.9  2000/10/11 19:37:55  jimg
// Moved the CVS log entries to the end of files.
// Changed the definition of the read method to match the dap library.
// Added exception handling.
// Added exceptions to the read methods.
//
// Revision 1.8  2000/08/31 22:16:54  jimg
// Merged with 3.1.7
//
// Revision 1.6.2.2  2000/08/31 21:31:43  jimg
// Merged changes from the trunk (rev 1.7).
//
// Revision 1.7  2000/08/31 02:53:18  dan
// Modified DODS_Time::set(time_str) to handle hours only
// time strings.  This is part of ISO8601 time specifications
// and how JPL stores their daily 9KM Pathfinder archives.
//
// Revision 1.6.2.1  2000/08/03 20:18:57  jimg
// Removed config_dap.h and replaced it with config_ff.h (in *.cc files;
// neither should be included in a header file).
// Changed code that calculated leap year information so that it uses the
// functions in date_proc.c/h.
//
// Revision 1.6  1999/07/22 21:28:09  jimg
// Merged changes from the release-3-0-2 branch
//
// Revision 1.5.2.2  1999/06/07 17:33:06  edavis
// Changed 'data()' to 'c_str()'.
//
// Revision 1.5.2.1  1999/06/01 15:38:06  jimg
// Added code to parse and return floating point dates.
//
// Revision 1.5  1999/05/27 17:02:22  jimg
// Merge with alpha-3-0-0
//
// Revision 1.4.2.1  1999/05/20 21:38:08  edavis
// Fix spelling of COPYRIGHT and remove some #if 0 stuff.
//
// Revision 1.4  1999/05/04 02:55:35  jimg
// Merge with no-gnu
//
// Revision 1.3.8.1  1999/05/01 04:40:28  brent
// converted old String.h to the new std C++ <string> code
//
// Revision 1.3  1999/01/05 00:35:55  jimg
// Removed string class; replaced with the GNU String class. It seems those
// don't mix well.
// Switched to simpler method names.
//
// Revision 1.2  1998/12/30 06:38:26  jimg
// Define TEST to use this without the dap++ library (e.g., when testing
// DODS_Date_Time).
//
// Revision 1.1  1998/12/28 19:07:33  jimg
// Initial version of the DODS_Time object
