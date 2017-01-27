// UgridFunctions.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2013 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include <iostream>

using std::endl;

#include "UgridFunctions.h"
#include "ServerFunctionsList.h"
#include "BESDebug.h"
#include "ugrid_restrict.h"

static string getFunctionNames()
{
    vector<string> names;
    libdap::ServerFunctionsList::TheList()->getFunctionNames(&names);

    string msg;
    for (std::vector<string>::iterator it = names.begin(); it != names.end(); ++it) {
        if (!msg.empty()) msg += ", ";

        msg += *it;
    }

    return msg;
}

void UgridFunctions::initialize(const string &/*modname*/)
{
    BESDEBUG("UgridFunctions", "initialize() - BEGIN" << endl);
    BESDEBUG("UgridFunctions", "initialize() - function names: " << getFunctionNames() << endl);

    ugrid::UGNR *ugnr = new ugrid::UGNR();
    libdap::ServerFunctionsList::TheList()->add_function(ugnr);

    BESDEBUG("UgridFunctions", "initialize() - function names: " << getFunctionNames() << endl);

    ugrid::UGER *uger = new ugrid::UGER();
    libdap::ServerFunctionsList::TheList()->add_function(uger);

    BESDEBUG("UgridFunctions", "initialize() - function names: " << getFunctionNames() << endl);

    ugrid::UGFR *ugfr = new ugrid::UGFR();
    libdap::ServerFunctionsList::TheList()->add_function(ugfr);

    BESDEBUG("UgridFunctions", "initialize() - function names: " << getFunctionNames() << endl);

    BESDEBUG("UgridFunctions", "initialize() - END" << endl);
}

void UgridFunctions::terminate(const string &/*modname*/)
{
    BESDEBUG("UgridFunctions", "Removing UgridFunctions Modules (this does nothing)." << endl);
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void UgridFunctions::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "UgridFunctions::dump - (" << (void *) this << ")" << endl;
}

extern "C" {
BESAbstractModule *maker()
{
    return new UgridFunctions;
}
}
