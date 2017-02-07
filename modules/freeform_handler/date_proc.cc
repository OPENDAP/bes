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

// These functions handle date translation between Julian and Gregorian
// formats. 8/12/93 jhrg
//
// This code was originally copied from one of the ACM's code libraries and
// used in the browsed image server. That version was written in C.


#include "config_ff.h"

static char rcsid[]not_used = {
        "$Id$" };

#include <cassert>

#include "Error.h"

using namespace libdap;

// You have to add one to days[1] if the year is a leap year. Since the month
// number in a Gregorian date is ones-based, fill element zero below to
// avoid gratuitous subtractions of the index value. Note that the fill
// value is never used. 9/4/98 jhrg
static int
        days_arr[13] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

/** Is the given Gregorian year a leap year?
 @return 1 if the year is a leap year, otherwise return 0. */
int is_leap(int year)
{
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

/** How many days are in the given Gregorian year?
 @return The number of days, as a double. */
double days_in_year(int year)
{
    return is_leap(year) ? 366.0 : 365.0;
}

static inline int days(int year, int month)
{
    if (!(year > 0) || !(month > 0 && month < 13))
        throw Error(malformed_expr, "Date year or month is bad.");

    if (month == 2 && is_leap(year))
        return 29;
    else
        return days_arr[month];
}

/**
 Given a gregorian date in year, month and day, return the Julian day
 number.

 NB: I am not sure this code deals correctly with dates before 1100 A.D.

 @param year The year. Must be A.D. and `fully qualified'
 @param month The month. Ones-based month number (Jan = 1, Dec = 12).
 @param day The day. Ones-based day number.
 @see ACM Collected algorithms.
 @return The Julian day number. */
long julian_day(int year, int month, int day)
{
    if (!(year > 0))
        throw Error(malformed_expr, "A date's year must be greater the zero.");
    if (!(month > 0 && month < 13))
        throw Error(malformed_expr,
                "A date's month must be between zero and thirteen.");

    if (!(day > 0 && day <= days(year, month)))
        throw Error(malformed_expr,
                "A date's day must be between zero and 28-31, depending on the month.");

    long jdn;

    jdn = (long) year * 367 + month * 275 / 9 - (year + (month > 2)) * 7 / 4
            - ((year - (month < 3)) / 100 + 1) * 3 / 4 + day + 1721029L;

    return (jdn);
}

/**
 Given a Julian day number, return the Gregorian date. The Julian day number
 is passed as a double while the gregorian date is returned using six
 `value-result' parameters:

 @see Algorithm 199 in Collected algorithms of the CACM Author: Robert G.
 Tantzen, Translator: Nat Howard
 @param jd The Julian date to convert.
 @param year The year fully-qualified (1977 is `1977' not 77, `77' is 77 A.D.)
 @param month The month. Ones-based.
 @param day The day.
 @param hours The hour. zero-based.
 @param minutes The minutes. zero-based.
 @param seconds The seconds. zero-based.
 @return void */
void gregorian_date(double jd, int *year, int *month, int *day, int *hours,
        int *minutes, double *seconds)
{
    assert(jd> 0.0);

    long j = (long) jd;
    double tmp, frac = jd - j;

    if (frac >= 0.5) {
        frac = frac - 0.5;
        j++;
    }
    else {
        frac = frac + 0.5;
    }

    j -= 1721119L;
    *year = (4L * j - 1L) / 146097L;
    j = 4L * j - 1L - 146097L * *year;
    *day = j / 4L;
    j = (4L * *day + 3L) / 1461L;
    *day = 4L * *day + 3L - 1461L * j;
    *day = (*day + 4L) / 4L;
    *month = (5L * *day - 3L) / 153L;
    *day = 5L * *day - 3 - 153L * *month;
    *day = (*day + 5L) / 5L;
    *year = 100L * *year + j;
    if (*month < 10) {
        *month += 3;
    }
    else {
        *month -= 9;
        *year += 1;
    }
    tmp = 3600.0 * (frac * 24.0);
    *hours = (int) (tmp / 3600.0);
    tmp = tmp - *hours * 3600.0;
    *minutes = (int) (tmp / 60.0);
    *seconds = tmp - *minutes * 60.0;
}

/** Given the month and day numbers, return the days since the first of the
 year.

 @param year The year.
 @param month The month of the year.
 @param day The days of the month.
 @return The days since the first of the year (ones-based). */
int month_day_to_days(int year, int month, int day)
{
    if (!(year > 0))
        throw Error(malformed_expr, "A date's year must be greater the zero.");
    if (!(month > 0 && month < 13))
        throw Error(malformed_expr,
                "A date's month must be between zero and thirteen.");

    if (!(day > 0 && day <= days(year, month)))
        throw Error(malformed_expr,
                "A date's day must be between zero and 28-31, depending on the month.");

    int ddd = day;

    while (--month)
        ddd += days(year, month);

    return ddd;
}

// Note this could be implemented using days_arr[] defined at the top of this
// file. 5/23/2001 jhrg
int days_in_month(int year, int month)
{
    //int daysInYear;
    int daysInMonth; // ... might want to remove the temporary
    // variable and just return the value from
    // the switch. Although the temp makes
    // debugging easier. 5/23/2001 jhrg

    switch (month) {
        case 1:
        case 3:
        case 5:
        case 7:
        case 8:
        case 10:
        case 12:
            daysInMonth = 31;
            break;
        case 2:
            //daysInYear = (year % 4) && !(year % 100) && (year % 400) ? 366 : 365;
            daysInMonth = (is_leap(year)) ? 29 : 28;
            break;
        case 4:
        case 6:
        case 9:
        case 11:
            daysInMonth = 30;
            break;
        default:
            // Added. 5/23/2001 jhrg
            throw Error("Months must be numbered between 1 and 12 inclusive.");
            break;
    }
    return daysInMonth;
}

/**
 Given the day number of the year, return the month and day of the month.

 @param year The year.
 @param ddd The day of the year (ones-based).
 @param month The correcponding month of the year (ones-based value-result
 parameter).
 @param day The day of the month (ones-based value-result parameter).
 @return void.
 */

void days_to_month_day(int year, int ddd, int *month, int *day)
{
    assert(year> 0);
    assert(ddd> 0 && ddd <= 365 + is_leap(year));

    *month = 1;

    while (ddd > days(year, *month))
        ddd -= days(year, (*month)++);

    *day = ddd;
}

/**
 Given a Julian day number, return the day of week for that date.

 @return Day of week (0-6)
 */

int dayofweek(double j)
{
    long jd = (long) j;
    if (j - jd >= 0.5)
        jd++;

    return (jd + 1) % 7;
}

