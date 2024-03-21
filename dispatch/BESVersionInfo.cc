// BESVersionInfo.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
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
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include "BESVersionInfo.h"
#include "BESInfoList.h"
#include "BESInternalError.h"

using std::endl;
using std::list;
using std::string;
using std::map;
using std::ostream;

/** @brief constructs a basic text information response object to write version
 *         information
 *
 * @see BESXMLInfo
 * @see BESResponseObject
 */
BESVersionInfo::BESVersionInfo() :
    BESInfo(), _inbes(false), _inhandler(false), _info(0)
{
    _info = BESInfoList::TheList()->build_info();
}

BESVersionInfo::~BESVersionInfo()
{
    if (_info) delete _info;
}

void BESVersionInfo::add_library(const string &name, const string &vers)
{
    add_version("library", name, vers);
}

void BESVersionInfo::add_module(const string &name, const string &vers)
{
    add_version("module", name, vers);
}

void BESVersionInfo::add_service(const string &name, const list<string> &vers)
{
    map<string, string, std::less<>> props;
    props["name"] = name;
    begin_tag("serviceVersion", &props);
    list<string>::const_iterator i = vers.begin();
    list<string>::const_iterator e = vers.end();
    for (; i != e; i++) {
        add_tag("version", (*i));
    }
    end_tag("serviceVersion");
}

void BESVersionInfo::add_version(const string &type, const string &name, const string &vers)
{
    map<string, string, std::less<>> attrs;
    attrs["name"] = name;
    attrs["version"] = vers;
    add_tag(type, "", &attrs);
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * this version information object
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESVersionInfo::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESVersionInfo::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "in BES version? " << _inbes << endl;
    strm << BESIndent::LMarg << "in Handler version? " << _inhandler << endl;
    if (_info) {
        strm << BESIndent::LMarg << "redirection info object:" << endl;
        BESIndent::Indent();
        _info->dump(strm);
        BESIndent::UnIndent();
    }
    else {
        strm << BESIndent::LMarg << "redirection info object: null" << endl;
    }
    BESInfo::dump(strm);
    BESIndent::UnIndent();
}

