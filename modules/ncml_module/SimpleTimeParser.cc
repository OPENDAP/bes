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

#include "SimpleTimeParser.h"

#include <sstream>

using std::map;
using std::string;
using std::istringstream;

namespace agg_util {
const long SimpleTimeParser::_sSecsInMin = 60L;
const long SimpleTimeParser::_sSecsInHour = 60L * SimpleTimeParser::_sSecsInMin;
const long SimpleTimeParser::_sSecsInDay = 24L * SimpleTimeParser::_sSecsInHour;
const long SimpleTimeParser::_sSecsInWeek = 7L * SimpleTimeParser::_sSecsInDay;
const long SimpleTimeParser::_sSecsInMonth = 31L * SimpleTimeParser::_sSecsInDay;
const long SimpleTimeParser::_sSecsInYear = 365L * SimpleTimeParser::_sSecsInDay;

map<string, long> SimpleTimeParser::_sParseTable = std::map<string, long>();
bool SimpleTimeParser::_sInited = false;

SimpleTimeParser::SimpleTimeParser()
{
}

SimpleTimeParser::~SimpleTimeParser()
{
}

bool SimpleTimeParser::parseIntoSeconds(long& seconds, const string& duration)
{
    bool success = true;

    if (!_sInited) {
        initParseTable();
    }

    istringstream iss;
    iss.str(duration);
    iss >> seconds;
    if (iss.fail()) {
        success = false;
    }
    else // we got the numerical portion, now parse the units.
    {
        string units;
        iss >> units;
        if (iss.fail()) {
            success = false;
        }
        else {
            std::map<std::string, long>::iterator foundIt = _sParseTable.find(units);
            if (foundIt == _sParseTable.end()) {
                success = false;
            }
            else {
                seconds *= foundIt->second;
            }
        }
    }

    if (!success) {
        seconds = -1;
    }
    return success;
}

void SimpleTimeParser::initParseTable()
{
    /*
     * seconds: { s, sec, secs, second, seconds }
     * minutes: { m, min, mins, minute, minutes }
     * hours: { h, hour, hours }
     * days: { day, days }
     * months: { month, months }
     * years: { year, years }
     */

    _sParseTable["s"] = 1L;
    _sParseTable["sec"] = 1L;
    _sParseTable["secs"] = 1L;
    _sParseTable["second"] = 1L;
    _sParseTable["seconds"] = 1L;

    _sParseTable["m"] = _sSecsInMin;
    _sParseTable["min"] = _sSecsInMin;
    _sParseTable["mins"] = _sSecsInMin;
    _sParseTable["minute"] = _sSecsInMin;
    _sParseTable["minutes"] = _sSecsInMin;

    _sParseTable["h"] = _sSecsInHour;
    _sParseTable["hour"] = _sSecsInHour;
    _sParseTable["hours"] = _sSecsInHour;

    _sParseTable["day"] = _sSecsInDay;
    _sParseTable["days"] = _sSecsInDay;

    _sParseTable["week"] = _sSecsInWeek;
    _sParseTable["weeks"] = _sSecsInWeek;

    _sParseTable["month"] = _sSecsInMonth;
    _sParseTable["months"] = _sSecsInMonth;

    _sParseTable["year"] = _sSecsInYear;
    _sParseTable["years"] = _sSecsInYear;

    _sInited = true;
}

}
