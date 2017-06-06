//////////////////////////////////////////////////////////////////////////////
// This file is part of the "NcML Module" project, a BES module designed
// to allow NcML files to be used to be used as a wrapper to add
// AIS to existing datasets of any format.
//
// Copyright (c) 2009 OPeNDAP, Inc.
// Author: Michael Johnson  <m.johnson@opendap.org>
//
// For more information, please also see the main website: http://opendap.org/
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// Please see the files COPYING and COPYRIGHT for more information on the GLPL.
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
/////////////////////////////////////////////////////////////////////////////

#ifndef __AGG_UTIL__SIMPLE_TIME_PARSER_H__
#define __AGG_UTIL__SIMPLE_TIME_PARSER_H__

#include <map>
#include <string>

namespace agg_util {

/**
 * Helper class to parse in very simple string
 * specifications of times and return it as an
 * (approximate) duration in seconds.
 * By approximate, we use a month to be 31 days
 * and a year to be 365 days for purposes of
 * converting to seconds.
 *
 * We only can parse strings of the form "%d %unit"
 * where %d stands for a number and %unit stands
 * for a single basic time unit in this list.
 * We give the unit and then the strings that can be
 * used to represent it.
 *
 * seconds: { s, second, seconds }
 * minutes: { m, min, mins }
 * hours: { h, hour, hours }
 * days: { day, days }
 * weeks: { week, weeks}
 * months: { month, months }  [note: month considered 31 days!]
 * years: { year, years }
 *
 * For example:
 *
 * "1 min"
 * "3 s"
 * "5 hours"
 * "3 days"
 * "10 seconds"
 * "5 years"
 * "1 month"
 *
 *
 * TODO This probably should be tested using CPPUnit
 */
class SimpleTimeParser {
public:
    SimpleTimeParser();
    ~SimpleTimeParser();

    /** Parse the string in duration and to calculate
     * the (approximate) number of seconds it represents.
     * By approximate, we mean that a month is considered 31 days
     * and a year as 365 days.
     * @param seconds  the result will be placed in here on success
     * @param duration  time duration as specified in class docs.
     * @return whether the parse was successful.  if true, seconds is valid.
     *         if false, seconds will also be -1.
     */
    static bool parseIntoSeconds(long& seconds, const std::string& duration);

private:

    /** helper to fill in the _sParseTable with parse entries */
    static void initParseTable();

    // Constants for use in the table.
    static const long _sSecsInMin;
    static const long _sSecsInHour;
    static const long _sSecsInDay;
    static const long _sSecsInWeek;
    static const long _sSecsInMonth; // we use 31 days to calc this one
    static const long _sSecsInYear; // and 365 days this one

    static std::map<std::string, long> _sParseTable; // Map from units string to secs
    static bool _sInited;  // has the table been created yet?
};

}

#endif /* __AGG_UTIL__SIMPLE_TIME_PARSER_H__ */
