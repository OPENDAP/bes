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

#include "SimpleLocationParser.h"

#include "BESDebug.h"
#include "NCMLUtil.h"
#include "SaxParserWrapper.h"

using namespace std;
using namespace ncml_module;

SimpleLocationParser::SimpleLocationParser() :
    _location("")
{
}

SimpleLocationParser::~SimpleLocationParser()
{
    _location = "";
}

string SimpleLocationParser::parseAndGetLocation(const string& filename)
{
    SaxParserWrapper parser(*this);
    parser.parse(filename);
    std::string ret = _location;
    _location = "";
    return ret;
}

void SimpleLocationParser::onStartElement(const string& name, const XMLAttributeMap& attrs)
{
    if (name == "netcdf") {
        _location = attrs.getValueForLocalNameOrDefault("location", "");
    }
}

void SimpleLocationParser::onParseWarning(std::string msg)
{
    BESDEBUG("ncml", "Parse Warning:" << msg << endl);
}

void SimpleLocationParser::onParseError(std::string msg)
{
    BESDEBUG("ncml", "Parse Error:" << msg << endl);
    // we'll just get an empty location out and handle it upstairs.
}
