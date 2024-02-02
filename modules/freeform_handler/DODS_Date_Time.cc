
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

//
// Implementation of the DODS Date/Time class

#include "config_ff.h"

static char rcsid[] not_used ="$Id$";


#include <time.h>

#include <cassert>
#include <cstdlib>
#include <sstream>
#include <string>

#include <libdap/Error.h>
#include "DODS_Date_Time.h"
#include "date_proc.h"
#include <libdap/debug.h> 

using namespace std;

#define seconds_per_day 86400.0

using namespace std;

static string
extract_argument(BaseType *arg)
{
#ifndef TEST
    if (arg->type() != dods_str_c)
	throw Error(malformed_expr, 
	      "The Projection function requires a DODS string-type argument.");
    
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

// Public mfuncs

bool
DODS_Date_Time::OK() const
{
    return _time.OK() && _date.OK();
}

DODS_Date_Time::DODS_Date_Time()
{
}


DODS_Date_Time::DODS_Date_Time(DODS_Date d, DODS_Time t) : _date(d), _time(t)
{
}

DODS_Date_Time::DODS_Date_Time(string date_time)
{
    set(date_time);
}

DODS_Date_Time::DODS_Date_Time(BaseType *date_time)
{
    set(date_time);
}

DODS_Date_Time::DODS_Date_Time(int y, int m, int d, int hh, int mm, 
			       double ss, bool gmt)
{
    set(y, m, d, hh, mm, ss, gmt);
}

DODS_Date_Time::DODS_Date_Time(int y, int yd, int hh, int mm, double ss, 
			       bool gmt)
{
    set(y, yd, hh, mm, ss, gmt);
}

void
DODS_Date_Time::set(DODS_Date d, DODS_Time t)
{
    _date = d;
    _time = t;
    
    assert(OK());
}

// Ripped off from Dan's DODS_Decimal_Year code. 5/29/99 jhrg

void
DODS_Date_Time::parse_fractional_time(string dec_year)
{
    double secs_in_year;
    double d_year_day,  d_hr_day, d_min_day, d_sec_day;
    int i_year, i_year_day, i_hr_day, i_min_day, i_sec_day;
    
    // The format for the decimal-year string is <year part>.<fraction part>.

    double d_year = strtod(dec_year.c_str(), 0);

    i_year = (int)d_year;
    double year_fraction = d_year - i_year;

    secs_in_year = days_in_year(i_year) * seconds_per_day;

    //
    // Recreate the 'day' in the year.
    //
    d_year_day = (secs_in_year * year_fraction)/seconds_per_day + 1;
    i_year_day = (int)d_year_day;

    //
    // Recreate the 'hour' in the day.
    //
    d_hr_day = ((d_year_day - i_year_day)*seconds_per_day) / seconds_per_hour;
    i_hr_day = (int)d_hr_day;

    //
    // Recreate the 'minute' in the hour.
    //
    d_min_day = ((d_hr_day - i_hr_day)*seconds_per_hour) / seconds_per_minute;
    i_min_day = (int)d_min_day;

    //
    // Recreate the 'second' in the minute.
    //
    d_sec_day = (d_min_day - i_min_day)*seconds_per_minute;
    i_sec_day = (int)d_sec_day;

    //
    // Round-off second to nearest value, handle condition
    // where seconds/minutes roll over modulo values.
    //
    if ((d_sec_day - i_sec_day) >= .5) i_sec_day++;

    if ( i_sec_day == 60 ) {
	i_sec_day = 0;
	i_min_day++;
	if ( i_min_day == 60 ) {
	    i_min_day = 0;
	    i_hr_day++;
	    if ( i_hr_day == 24 ) {
		i_hr_day = 0;
		i_year_day++;
		if ( i_year_day == (days_in_year(i_year) + 1)) {
		    i_year_day = 1;
		    i_year++;
		}
	    }
	}
    }

    _date.set((int)i_year, (int)i_year_day);
    _time.set((int)i_hr_day, (int)i_min_day, (double)i_sec_day);

    assert(OK());
}



void
DODS_Date_Time::set(string date_time)
{
    // Check for a fractional-date string and parse it if needed.
    if (date_time.find(".") != string::npos) {
	parse_fractional_time(date_time);
    }
    else {
	// The format for the date-time string is <date part>:<time part>.
	size_t i = date_time.find(":");
	string date_part = date_time.substr(0, i); 
	string time_part = date_time.substr(i+1, date_time.size());
    
	_date.set(date_part);
	_time.set(time_part);
    }

    assert(OK());
}

void
DODS_Date_Time::set(BaseType *date_time)
{
    set(extract_argument(date_time));
}

void
DODS_Date_Time::set(int y, int m, int d, int hh, int mm, double ss, bool gmt)
{
    _date.set(y, m, d);
    _time.set(hh, mm, ss, gmt);

    assert(OK());
}

void
DODS_Date_Time::set(int y, int yd, int hh, int mm, double ss, bool gmt)
{
    _date.set(y, yd);
    _time.set(hh, mm, ss, gmt);

    assert(OK());
}

int
DODS_Date_Time::year() const
{
    return _date.year();
}

int
DODS_Date_Time::month() const
{
    return _date.month();
}

int
DODS_Date_Time::day() const
{
    return _date.day();
}

int
DODS_Date_Time::day_number() const
{
    return _date.day_number();
}

int
DODS_Date_Time::hours() const
{
    return _time.hours();
}

int
DODS_Date_Time::minutes() const
{
    return _time.minutes();
}

double
DODS_Date_Time::seconds() const
{
    return _time.seconds();
}

bool
DODS_Date_Time::gmt() const
{
    return _time.gmt();
}

string 
DODS_Date_Time::get(date_format format, bool gmt) const
{
    switch (format) {
      case ymd:
	return _date.get() + ":" + _time.get(gmt);
      case yd:
	return _date.get(yd) + ":" + _time.get(gmt);
      case decimal: {
	  ostringstream oss;
	  oss.precision(14);

	  double decday = (_date.fraction() 
			   + _time.fraction()/days_in_year(_date.year()));

	  oss << decday;

	  return oss.str();
      }
      default:
#ifndef TEST
	throw Error(unknown_error, "Invalid date format");
#else
	assert("Invalid date format" && false);
#endif
    }
}

double
DODS_Date_Time::julian_day() const
{
    return _date.julian_day() + _time.seconds_since_midnight()/seconds_per_day;
}

time_t 
DODS_Date_Time::unix_time() const
{
    struct tm tm_rec{};
    tm_rec.tm_mday = _date.day();
    tm_rec.tm_mon = _date.month() - 1; // zero-based 
    tm_rec.tm_year = _date.year() - 1900; // years since 1900
    tm_rec.tm_hour = _time.hours();
    tm_rec.tm_min = _time.minutes();
    tm_rec.tm_sec = (int)_time.seconds();
    tm_rec.tm_isdst = -1;

    return mktime(&tm_rec);
}

double
DODS_Date_Time::get_epsilon() const
{
    return _time.get_epsilon();
}

void
DODS_Date_Time::set_epsilon(double eps)
{
    _time.set_epsilon(eps);
}

int
operator==(DODS_Date_Time &t1, DODS_Date_Time &t2)
{
    return t1._date == t2._date && t1._time == t2._time;
}

int
operator!=(DODS_Date_Time &t1, DODS_Date_Time &t2)
{
    return t1._date != t2._date || t1._time != t2._time;
}

int
operator<(DODS_Date_Time &t1, DODS_Date_Time &t2)
{
    return t1._date < t2._date 
	|| (t1._date == t2._date && t1._time < t2._time);
}

int
operator>(DODS_Date_Time &t1, DODS_Date_Time &t2)
{
    return t1._date > t2._date 
	|| (t1._date == t2._date && t1._time > t2._time);
}

int
operator<=(DODS_Date_Time &t1, DODS_Date_Time &t2)
{
    return t1 == t2 || t1 < t2;
}

int
operator>=(DODS_Date_Time &t1, DODS_Date_Time &t2)
{
    return t1 == t2 || t1 > t2;
}

#ifdef DATE_TIME_TEST

/* Input args: 1 string,
    2 Two strings, 
    5 y, yd, hh, mm, ss,
    6 y, m, d, hh, mm, ss 
    
    Compile using: g++ -g -I../../include -DHAVE_CONFIG_H -DTEST
    -DDATE_TIME_TEST DODS_Date.cc DODS_Time.cc DODS_Date_Time.cc date_proc.cc
    -lg++ 
*/

int
main(int argc, char *argv[])
{
    DODS_Date_Time dt;
    DODS_Date_Time dt2("1970/1/1:0:0:0");

    argc--;
    switch(argc) {
      case 1:
	dt.set_date_time(argv[1]);
	break;
      case 2:
	dt.set_date_time(argv[1]);
	dt2.set_date_time(argv[2]);
	break;
      case 5:
	dt.set_date_time(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), 
			 atoi(argv[4]), atof(argv[5]));
	break;
      case 6:
	dt.set_date_time(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), 
			 atoi(argv[4]), atoi(argv[5]), atof(argv[6]));
	break;
      default:
	cerr << "Wrong number of arguments!" << endl;
	exit(1);
    }
	
    if (dt < dt2)
	cout << "True: dt < dt2" << endl;
    else
	cout << "False: dt < dt2" << endl;

    if (dt > dt2)
	cout << "True: dt > dt2" << endl;
    else
	cout << "False: dt > dt2" << endl;
    
    if (dt <= dt2)
	cout << "True: dt <= dt2" << endl;
    else
	cout << "False: dt <= dt2" << endl;

    if (dt >= dt2)
	cout << "True: dt >= dt2" << endl;
    else
	cout << "False: dt >= dt2" << endl;

    if (dt == dt2)
	cout << "True: dt == dt2" << endl;
    else
	cout << "False: dt == dt2" << endl;

    if (dt != dt2)
	cout << "True: dt != dt2" << endl;
    else
	cout << "False: dt != dt2" << endl;

    cout << "YMD: " << dt.ymd_date_time() << endl;
    cout << "YD: " << dt.yd_date_time() << endl;
    cout << "Julian day: " << dt.julian_day() << endl;
    cout << "Seconds: " << dt.unix_time() << endl;
}
#endif // TEST_DATE

// $Log: DODS_Date_Time.cc,v $
// Revision 1.10  2004/02/04 20:50:08  jimg
// Build fixes. No longer uses Pix.
//
// Revision 1.9  2003/12/08 21:59:52  edavis
// Merge release-3-4 into trunk
//
// Revision 1.7.4.1  2003/06/29 05:37:32  rmorris
// Include standard template libraries appropriately and add missing usage
// statements.
//
// Revision 1.8  2003/05/14 19:23:13  jimg
// Changed from strstream to sstream.
//
// Revision 1.7  2003/02/10 23:01:52  jimg
// Merged with 3.2.5
//
// Revision 1.6.2.2  2002/12/18 23:30:42  pwest
// gcc3.2 compile corrections, mainly regarding the using statement
//
// Revision 1.6.2.1  2002/11/13 05:58:05  dan
// Fixed return variable name in get method, changed
// from 'yd' to 'dateString'.  'yd' is also a value in
// the enumeration type date_format.
//
// Revision 1.6  2000/10/11 19:37:55  jimg
// Moved the CVS log entries to the end of files.
// Changed the definition of the read method to match the dap library.
// Added exception handling.
// Added exceptions to the read methods.
//
// Revision 1.5  2000/08/31 22:16:53  jimg
// Merged with 3.1.7
//
// Revision 1.4.2.1  2000/08/03 20:18:57  jimg
// Removed config_dap.h and replaced it with config_ff.h (in *.cc files;
// neither should be included in a header file).
// Changed code that calculated leap year information so that it uses the
// functions in date_proc.c/h.
//
// Revision 1.4  1999/07/22 21:28:09  jimg
// Merged changes from the release-3-0-2 branch
//
// Revision 1.3.6.1  1999/06/01 15:38:06  jimg
// Added code to parse and return floating point dates.
//
// Revision 1.3  1999/05/04 02:55:35  jimg
// Merge with no-gnu
//
// Revision 1.2.8.1  1999/05/01 04:40:28  brent
// converted old String.h to the new std C++ <string> code
//
// Revision 1.2  1999/01/05 00:35:35  jimg
// Removed string class; replaced with the GNU string class. It seems those
// don't mix well.
// Switched to simpler method names.
//
// Revision 1.1  1998/12/30 06:40:39  jimg
// Initial version
//
