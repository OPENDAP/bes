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
// Implementation of the DODS Date class

#include "config_ff.h"

static char rcsid[] not_used ="$Id$";

#include <cassert>
#include <cstdlib>
#include <sstream>
#include <string>
#include <iomanip>

#include "DODS_Date.h"
#include "date_proc.h"

#include <libdap/BaseType.h>
#include <libdap/Str.h>
#include <libdap/debug.h>

using namespace std;

// The Error class is defined in the core software. For testing we don't need
// this function and can supply a dummy version. That simplifies building the
// test code. 11/12/98 jhrg

// This function is repeated in DODS_Time, something that should be changed,
// at the least. However, the real problem is that it is pretty hard to read
// values from DODS types. 1/8/99 jhrg

#include <libdap/Error.h>

using namespace std;

static string extract_argument(BaseType *arg)
{
#ifndef TEST
	if (arg->type() != dods_str_c)
		throw Error(malformed_expr, "The Projection function requires a DODS string-type argument.");
#if 0
	// Use String until conversion of String to string is complete. 9/3/98
	// jhrg
	string *sp = NULL;
	arg->buf2val((void **) &sp);
	string s = sp->c_str();
	delete sp;

	DBG(cerr << "s: " << s << endl);

	return s;
#endif
	return static_cast<Str*>(arg)->value();
#else
	return "";
#endif
}

bool DODS_Date::OK() const
{
	return _year > 0 && _month > 0 && _day > 0 && _julian_day > 0 && _day_number > 0 && _format != unknown_format;
}

// Public member functions.

DODS_Date::DODS_Date() :
    _julian_day(0), _year(0), _month(0), _day(0), _day_number(0), _format(unknown_format)
{
}

DODS_Date::DODS_Date(BaseType *arg)
{
	string s = extract_argument(arg);
	set(s);
}

DODS_Date::DODS_Date(string date_str)
{
	set(date_str);
}

DODS_Date::DODS_Date(int year, int day_num)
{
	set(year, day_num);
}

DODS_Date::DODS_Date(int year, int month, int day)
{
	set(year, month, day);
}

DODS_Date::DODS_Date(int year, int month, int day, date_format format)
{
	set(year, month, day, format);
}

void DODS_Date::set(BaseType *arg)
{
	string s = extract_argument(arg);
	set(s);
}

// The software that parses data strings is pretty weak on error checking.
// This should be bolstered. For example, a real parser which flags invalid
// separators, etc. would improve error detection.

void DODS_Date::parse_integer_time(string date)
{
	// Parse the date_str.
	istringstream iss(date.c_str());
	char c;
	size_t pos1, pos2;
	iss >> _year;
	iss >> c;
	iss >> _month;

	// If there are two slashes, assume a yyyy/mm/dd date.
	pos1 = date.find("/");
	pos2 = date.rfind("/");
	if ((pos1 == date.npos) && (pos2 == date.npos)) {
		string msg = "I cannot understand the date string: ";
		msg += date + ". I expected a date formatted like yyyy/mm/dd or yyyy/ddd.";
		throw Error(malformed_expr, msg);
	}
	else if ((pos1 != pos2)) {
		iss >> c;
		iss >> _day;
		// Convert to julian day number and record year, month, ...
		_julian_day = ::julian_day(_year, _month, _day);
		_day_number = month_day_to_days(_year, _month, _day);
		_format = ymd; // jhrg 10/1/13
	}
	else {
		// Note that when a `yyyy/ddd' date is read in, the day-number winds
		// up in the `_month' member.
		_day_number = _month;
		days_to_month_day(_year, _day_number, &_month, &_day);
		_julian_day = ::julian_day(_year, _month, _day);
		_format = yd; // jhrg 10/1/13
	}
}

void DODS_Date::parse_iso8601_time(string date)
{
	// Parse the date_str.
	istringstream iss(date.c_str());
	char c;
	size_t pos1, pos2;
	iss >> _year;
	iss >> c;
	iss >> _month;

	// If there are two dashes, assume a ccyy-mm-dd date.
	pos1 = date.find("-");
	pos2 = date.rfind("-");
	if ((pos1 != date.npos) && (pos2 != date.npos) && (pos1 != pos2)) {
		iss >> c;
		iss >> _day;
		// Convert to julian day number and record year, month, ...
		_julian_day = ::julian_day(_year, _month, _day);
		_day_number = month_day_to_days(_year, _month, _day);
		_format = ymd;
	}
	// Added parens around the AND below. jhrg 9/26/13
	else if (((pos1 != date.npos) && (pos2 == date.npos)) || (pos1 == pos2)) {
		// There is one dash, assume a ccyy-mm date.
		_day = 1;
		_julian_day = ::julian_day(_year, _month, _day);
		_day_number = month_day_to_days(_year, _month, _day);
		_format = ym;
	}

	else if ((pos1 == date.npos) && (date.size() == 4)) {
		// There are no dashes, assume a ccyy date.
		_day = 1;
		_month = 1;
		_julian_day = ::julian_day(_year, _month, _day);
		_day_number = month_day_to_days(_year, _month, _day);
		_format = ym;
	}
	else {
		string msg = "I cannot understand the date string: ";
		msg += date + ". I expected an iso8601 date (ccyy-mm-dd, ccyy-mm or ccyy).";
		throw Error(malformed_expr, msg);
	}
}

// This parser was originally used to build both DODS_Date and DODS_Time
// objects in Dan's DODS_Decimal_Year class. I've left in the code that does
// stuff for time because the fractional part of the seconds might bump the
// day count (and because it was easy to use the code without changing it
// :-). 5/29/99 jhrg

void DODS_Date::parse_fractional_time(string dec_year)
{
	double secs_in_year;
	double d_year_day, d_hr_day, d_min_day, d_sec_day;
	int i_year, i_year_day, i_hr_day, i_min_day, i_sec_day;

	// The format for the decimal-year string is <year part>.<fraction part>.

	double d_year = strtod(dec_year.c_str(), 0);

	i_year = (int) d_year;
	double year_fraction = d_year - i_year;

	secs_in_year = days_in_year(_year) * seconds_per_day;

	//
	// Recreate the 'day' in the year.
	//
	d_year_day = (secs_in_year * year_fraction) / seconds_per_day + 1;
	i_year_day = (int) d_year_day;

	//
	// Recreate the 'hour' in the day.
	//
	d_hr_day = ((d_year_day - i_year_day) * seconds_per_day) / seconds_per_hour;
	i_hr_day = (int) d_hr_day;

	//
	// Recreate the 'minute' in the hour.
	//
	d_min_day = ((d_hr_day - i_hr_day) * seconds_per_hour) / seconds_per_minute;
	i_min_day = (int) d_min_day;

	//
	// Recreate the 'second' in the minute.
	//
	d_sec_day = (d_min_day - i_min_day) * seconds_per_minute;
	i_sec_day = (int) d_sec_day;

	//
	// Round-off second to nearest value, handle condition
	// where seconds/minutes roll over modulo values.
	//
	if ((d_sec_day - i_sec_day) >= .5) i_sec_day++;

	if (i_sec_day == 60) {
		// i_sec_day = 0;
		i_min_day++;
		if (i_min_day == 60) {
			// i_min_day = 0;
			i_hr_day++;
			if (i_hr_day == 24) {
				// i_hr_day = 0;
				i_year_day++;
				if (i_year_day == (days_in_year(_year) + 1)) {
					i_year_day = 1;
					i_year++;
				}
			}
		}
	}

	set(i_year, i_year_day);

	assert(OK());
}

void DODS_Date::set(string date)
{
	// Check for fractional date/time strings.
	if (date.find(".") != string::npos) {
		parse_fractional_time(date);
	}
	else if (date.find("/") != string::npos) {
		parse_integer_time(date);
	}
	else if (date.find("-") != string::npos) {
		parse_iso8601_time(date);
	}
	else if (date.size() == 4) {
		date += "-1-1";
		parse_iso8601_time(date);
	}
	else
		throw Error(malformed_expr, "Could not recognize date format");

	assert(OK());
}

void DODS_Date::set(int year, int day_num)
{
	_year = year;
	_day_number = day_num;
	days_to_month_day(_year, _day_number, &_month, &_day);
	_julian_day = ::julian_day(_year, _month, _day);

	_format = yd; // jhrg 10/1/13

	assert(OK());
}

void DODS_Date::set(int year, int month, int day)
{
	_year = year;
	_month = month;
	_day = day;
	_day_number = month_day_to_days(_year, _month, _day);
	_julian_day = ::julian_day(_year, _month, _day);

	_format = ymd; // jhrg 10/1/13

	assert(OK());
}

void DODS_Date::set(int year, int month, int day, date_format format)
{
	_year = year;
	_month = month;
	_day = day;
	_day_number = month_day_to_days(_year, _month, _day);
	_julian_day = ::julian_day(_year, _month, _day);
	_format = format;

	assert(OK());
}

int operator==(DODS_Date &d1, DODS_Date &d2)
{
	if (d2.format() == ym) {
		return ((d2._julian_day >= ::julian_day(d1.year(), d1.month(), 1))
				&& (d2._julian_day <= ::julian_day(d1.year(), d1.month(), days_in_month(d1.year(), d1.month())))) ?
				1 : 0;
	}
	else
		return d1._julian_day == d2._julian_day ? 1 : 0;
}

int operator!=(DODS_Date &d1, DODS_Date &d2)
{
	return d1._julian_day != d2._julian_day ? 1 : 0;
}

int operator<(DODS_Date &d1, DODS_Date &d2)
{
	return d1._julian_day < d2._julian_day ? 1 : 0;
}

int operator>(DODS_Date &d1, DODS_Date &d2)
{
	return d1._julian_day > d2._julian_day ? 1 : 0;
}

int operator<=(DODS_Date &d1, DODS_Date &d2)
{
	if (d2.format() == ym)
		return ((d2._julian_day >= ::julian_day(d1.year(), d1.month(), 1)) && true) ? 1 : 0;
	else
		return d1._julian_day <= d2._julian_day ? 1 : 0;
}

int operator>=(DODS_Date &d1, DODS_Date &d2)
{
	if (d2.format() == ym)
		return ((d2._julian_day <= ::julian_day(d1.year(), d1.month(), days_in_month(d1.year(), d1.month()))) && true) ?
				1 : 0;
	else
		return d1._julian_day >= d2._julian_day ? 1 : 0;
}

int DODS_Date::year() const
{
	return _year;
}

int DODS_Date::month() const
{
	return _month;
}

int DODS_Date::day() const
{
	return _day;
}

int DODS_Date::day_number() const
{
	return _day_number;
}

long DODS_Date::julian_day() const
{
	return _julian_day;
}

// Return the fractional part of the date. A private function.

date_format DODS_Date::format() const
{
	return _format;
}

double DODS_Date::fraction() const
{
	return _year + (_day_number - 1) / days_in_year(_year);
}

string DODS_Date::get(date_format format) const
{
	ostringstream oss;

	switch (format) {
	case yd:
		oss << _year << "/" << _day_number;
		break;
	case ymd:
		oss << _year << "/" << _month << "/" << _day;
		break;
	case iso8601:
		if (_format == ym) {
			oss << _year << "-" << setfill('0') << setw(2) << _month;
		}
		else {
			oss << _year << "-" << setfill('0') << setw(2) << _month << "-" << setfill('0') << setw(2) << _day;
		}
		break;
	case decimal:
		oss.precision(14);
		oss << fraction();
		break;
	default:
#ifndef TEST
		throw Error(unknown_error, "Invalid date format");
#else
		assert("Invalid date format" && false);
#endif
	}

	return oss.str();
}

time_t DODS_Date::unix_time() const
{
	struct tm tm_rec{};
	tm_rec.tm_mday = _day;
	tm_rec.tm_mon = _month - 1; // zero-based
	tm_rec.tm_year = _year - 1900; // years since 1900
	tm_rec.tm_hour = 0;
	tm_rec.tm_min = 0;
	tm_rec.tm_sec = 1;		// smallest time into the day
	tm_rec.tm_isdst = -1;

	return mktime(&tm_rec);
}

#ifdef TEST_DATE

// Call this with one, two or three args. If one arg, call the string ctor.
// If two or three args, use the yd or ymd ctor. Once built, compare to 1 Jan
// 1970 and then call the yd_date() and ymd_date() mfuncs. 11/4/98 jhrg

// Build with: `g++ -g -I../../include -DHAVE_CONFIG_H -DTEST_DATE -TEST
// DODS_Date.cc date_proc.o -lg++'. Add: `-ftest-coverage -fprofile-arcs' for
// test coverage. 

int main(int argc, char *argv[])
{
	DODS_Date epoc((string)"1970/1/1");
	DODS_Date d1;

	switch (--argc) {
		case 1:
		d1.set((string)argv[1]);
		break;
		case 2:
		d1.set(atoi(argv[1]), atoi(argv[2]));
		break;
		case 3:
		d1.set(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
		break;
		case 4:
		d1.set(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), (date_format)atoi(argv[4]));
		break;
		default:
		cerr << "Wrong number of args!" << endl;
		abort();
	}

	if (d1 < epoc)
	cout << "True: d1 < epoc" << endl;
	else
	cout << "False: d1 < epoc" << endl;

	if (d1 > epoc)
	cout << "True: d1 > epoc" << endl;
	else
	cout << "False: d1 > epoc" << endl;

	if (d1 <= epoc)
	cout << "True: d1 <= epoc" << endl;
	else
	cout << "False: d1 <= epoc" << endl;

	if (d1 >= epoc)
	cout << "True: d1 >= epoc" << endl;
	else
	cout << "False: d1 >= epoc" << endl;

	if (d1 == epoc)
	cout << "True: d1 == epoc" << endl;
	else
	cout << "False: d1 == epoc" << endl;

	if (d1 != epoc)
	cout << "True: d1 != epoc" << endl;
	else
	cout << "False: d1 != epoc" << endl;

	cout << "YMD: " << d1.get() << endl;
	cout << "ISO8601: " << d1.get(iso8601) << endl;
	cout << "YD: " << d1.get(yd) << endl;
	cout << "Julian day: " << d1.julian_day() << endl;
	cout << "Seconds: " << d1.unix_time() << endl;
}
#endif // TEST_DATE
// $Log: DODS_Date.cc,v $
// Revision 1.16  2004/07/09 17:54:24  jimg
// Merged with release-3-4-3FCS.
//
// Revision 1.12.4.2  2004/03/07 22:05:51  rmorris
// Final code changes to port the freeform server to win32.
//
// Revision 1.15  2004/02/04 20:50:08  jimg
// Build fixes. No longer uses Pix.
//
// Revision 1.14  2003/12/08 22:01:12  edavis
// Merge release-3-4 into trunk
//
// Revision 1.12.4.1  2003/06/29 05:35:10  rmorris
// Use standard template library headers correctly and add missing usage
// statements.
//
// Revision 1.13  2003/05/14 19:23:13  jimg
// Changed from strstream to sstream.
//
// Revision 1.12  2003/02/10 23:01:52  jimg
// Merged with 3.2.5
//
// Revision 1.11  2001/09/28 23:19:43  jimg
// Merged with 3.2.3.
//
// Revision 1.10.2.3  2002/11/13 05:51:57  dan
// Modified get(date_format format) method, renaming
// return variable from 'yd' to 'dateString'.  'yd' is
// a value of the enumeration date_format and in multi-threaded
// code this was causing a seg-fault in mutex-lock.
//
// Revision 1.10.2.2  2001/09/19 22:40:06  jimg
// Added simple error checking for malformed dates. Works sometimes... To do
// a thorough job will take at least a day.
//
// Revision 1.10.2.1  2001/05/23 18:25:29  dan
// Modified to support year/month date representations,
// and to support ISO8601 output formats.
//
// Revision 1.10  2000/10/11 19:37:55  jimg
// Moved the CVS log entries to the end of files.
// Changed the definition of the read method to match the dap library.
// Added exception handling.
// Added exceptions to the read methods.
//
// Revision 1.9  2000/08/31 22:16:53  jimg
// Merged with 3.1.7
//
// Revision 1.8.2.1  2000/08/03 20:18:57  jimg
// Removed config_dap.h and replaced it with config_ff.h (in *.cc files;
// neither should be included in a header file).
// Changed code that calculated leap year information so that it uses the
// functions in date_proc.c/h.
//
// Revision 1.8  1999/07/22 21:28:08  jimg
// Merged changes from the release-3-0-2 branch
//
// Revision 1.7.2.1  1999/06/01 15:38:05  jimg
// Added code to parse and return floating point dates.
//
// Revision 1.7  1999/05/27 17:02:21  jimg
// Merge with alpha-3-0-0
//
// Revision 1.6.2.1  1999/05/20 21:37:26  edavis
// Fix spelling of COPYRIGHT and remove some #if 0 stuff.
//
// Revision 1.6  1999/05/04 02:55:35  jimg
// Merge with no-gnu
//
// Revision 1.5.6.1  1999/05/01 04:40:28  brent
// converted old String.h to the new std C++ <string> code
//
// Revision 1.5  1999/01/08 22:09:01  jimg
// Added some comments about errors.
//
// Revision 1.4  1999/01/05 00:34:04  jimg
// Removed string class; replaced with the GNU String class. It seems those
// don't mix well.
// Switched to simpler method names.
//
// Revision 1.3  1998/12/30 06:39:18  jimg
// Define TEST when building the test version (also define DATE_TEST).
//
// Revision 1.2  1998/12/30 02:00:58  jimg
// Added class invariant.
//
// Revision 1.1  1998/12/28 19:08:25  jimg
// Initial version of the DODS_Date object
//
