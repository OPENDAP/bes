
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
//      danh            Dan Holloway (dholloway@gso.uri.edu)
//
// Implementation of the DODS Decimal_Year class

#include "config_ff.h"

static char rcsid[] not_used ="$Id$";


#include <time.h>

#include <cassert>
#include <cstdlib>
#include <sstream>

#include <libdap/Error.h>
#include "DODS_Decimal_Year.h"
#include "date_proc.h"
#include <libdap/debug.h>

#define seconds_per_day 86400.0
#define seconds_per_hour 3600.0
#define seconds_per_minute 60.0

static string
extract_argument(BaseType *arg)
{
#ifndef TEST
    if (arg->type() != dods_str_c)
	throw Error(malformed_expr,
	      "The Projection function requires a DODS string-type argument.");

    // Use string until conversion of string to string is complete. 9/3/98
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
DODS_Decimal_Year::OK() const
{
    return _date.OK();
}

DODS_Decimal_Year::DODS_Decimal_Year()
{
}

DODS_Decimal_Year::DODS_Decimal_Year(DODS_Date d, DODS_Time t) : _date(d), _time(t)
{
}

DODS_Decimal_Year::DODS_Decimal_Year(string date_time)
{
    set(date_time);
}

DODS_Decimal_Year::DODS_Decimal_Year(BaseType *date_time)
{
    set(date_time);
}

DODS_Decimal_Year::DODS_Decimal_Year(int y, int m, int d, int hh, int mm,
			       double ss, bool gmt)
{
    set(y, m, d, hh, mm, ss, gmt);
}

DODS_Decimal_Year::DODS_Decimal_Year(int y, int yd, int hh, int mm, double ss,
			       bool gmt)
{
    set(y, yd, hh, mm, ss, gmt);
}

void
DODS_Decimal_Year::set(DODS_Date d)
{
    _date = d;

    assert(OK());
}

void
DODS_Decimal_Year::set(DODS_Date d, DODS_Time t)
{
    _date = d;
    _time = t;

    assert(OK());
}

void
DODS_Decimal_Year::set(string dec_year)
{
    double secs_in_year, days_in_year;
    double d_year_day,  d_hr_day, d_min_day, d_sec_day;
    int i_year, i_year_day, i_hr_day, i_min_day, i_sec_day;

    // The format for the decimal-year string is <year part>.<fraction part>.

    double d_year = strtod(dec_year.c_str(), 0);

    i_year = d_year * 1;
    double year_fraction = d_year - i_year;

    days_in_year = days_in_year(i_year);
    secs_in_year = days_in_year * seconds_per_day;

    //
    // Recreate the 'day' in the year.
    //
    d_year_day = (secs_in_year * year_fraction)/seconds_per_day + 1;
    i_year_day = d_year_day * 1;

    //
    // Recreate the 'hour' in the day.
    //
    d_hr_day = ((d_year_day - i_year_day)*seconds_per_day) / seconds_per_hour;
    i_hr_day = d_hr_day * 1;

    //
    // Recreate the 'minute' in the hour.
    //
    d_min_day = ((d_hr_day - i_hr_day)*seconds_per_hour) / seconds_per_minute;
    i_min_day = d_min_day * 1;

    //
    // Recreate the 'second' in the minute.
    //
    d_sec_day = (d_min_day - i_min_day)*seconds_per_minute;
    i_sec_day = d_sec_day * 1;

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
	  if ( i_year_day == (days_in_year+1)) {
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
DODS_Decimal_Year::set(BaseType *date_time)
{
    set(extract_argument(date_time));
}

void
DODS_Decimal_Year::set(int y, int m, int d, int hh, int mm, double ss, bool gmt)
{
    _date.set(y, m, d);
    _time.set(hh, mm, ss, gmt);

    assert(OK());
}

void
DODS_Decimal_Year::set(int y, int yd, int hh, int mm, double ss, bool gmt)
{
    _date.set(y, yd);
    _time.set(hh, mm, ss, gmt);

    assert(OK());
}

int
DODS_Decimal_Year::year() const
{
    return _date.year();
}

int
DODS_Decimal_Year::days_in_year() const
{
    int yr = _date.year();

    if ( (yr % 4 == 0) && ((yr % 100 != 0) || (yr % 400 == 0)) ) return 366;
    else return 365;
}

double
DODS_Decimal_Year::fraction() const
{
    double decimal_day;
    double day_number, hrs, min, sec, days_in;

    day_number = (double)_date.day_number();
    hrs        = (double)_time.hours();
    min        = (double)_time.minutes();
    sec        = (double)_time.seconds();
    days_in    = (double)days_in_year();

    decimal_day =  (day_number-1+((hrs+((min+(sec/60.0))/60.0))/24.0))/days_in;

    return decimal_day;
}

bool
DODS_Decimal_Year::gmt() const
{
    return _time.gmt();
}

string
DODS_Decimal_Year::get(date_format format, bool gmt) const
{
    ostringstream oss;
    oss.precision(14);

    double decday = (double)_date.year() + fraction();

    oss << decday;

    return oss.str()
}

double
DODS_Decimal_Year::julian_day() const
{
    return _date.julian_day() + _time.seconds_since_midnight()/seconds_per_day;
}

time_t
DODS_Decimal_Year::unix_time() const
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
DODS_Decimal_Year::get_epsilon() const
{
    return _time.get_epsilon();
}

void
DODS_Decimal_Year::set_epsilon(double eps)
{
    _time.set_epsilon(eps);
}

int
operator==(DODS_Decimal_Year &t1, DODS_Decimal_Year &t2)
{
    return t1._date == t2._date && t1._time == t2._time;
}

int
operator!=(DODS_Decimal_Year &t1, DODS_Decimal_Year &t2)
{
    return t1._date != t2._date || t1._time != t2._time;
}

int
operator<(DODS_Decimal_Year &t1, DODS_Decimal_Year &t2)
{
    return t1._date < t2._date
	|| (t1._date == t2._date && t1._time < t2._time);
}

int
operator>(DODS_Decimal_Year &t1, DODS_Decimal_Year &t2)
{
    return t1._date > t2._date
	|| (t1._date == t2._date && t1._time > t2._time);
}

int
operator<=(DODS_Decimal_Year &t1, DODS_Decimal_Year &t2)
{
    return t1 == t2 || t1 < t2;
}

int
operator>=(DODS_Decimal_Year &t1, DODS_Decimal_Year &t2)
{
    return t1 == t2 || t1 > t2;
}

#ifdef DECIMAL_YEAR_TEST

/* Input args: 1 string,
    2 Two strings,
    5 y, yd, hh, mm, ss,
    6 y, m, d, hh, mm, ss

    Compile using: g++ -g -I../../include -DHAVE_CONFIG_H -DTEST
    -DDECIMAL_YEAR_TEST DODS_Date.cc DODS_Time.cc DODS_Decimal_Year.cc date_proc.cc
    -lg++
*/

int
main(int argc, char *argv[])
{

    DODS_Decimal_Year dt;
    DODS_Decimal_Year dt2("1970/1/1:0:0:0");
    argc--;
    switch(argc) {
      case 1:
	dt.set(argv[1]);
	break;
      case 2:
	dt.set(argv[1]);
	dt2.set(argv[2]);
	break;
      case 5:
	dt.set(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]),
			 atoi(argv[4]), atof(argv[5]));
	break;
      case 6:
	dt.set(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]),
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

    //    cout << "YMD: " << dt.ymd() << endl;
    //cout << "YD: " << dt.yd() << endl;
    cout << "Julian day: " << dt.julian_day() << endl;
    cout << "Seconds: " << dt.unix_time() << endl;
}
#endif // TEST_DATE

