// BESXDRequestHandler.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
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

// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include "config.h"

#include <BESResponseHandler.h>
#include <BESResponseNames.h>
#include <BESVersionInfo.h>
#include <BESDataNames.h>

#include "BESXDRequestHandler.h"

using std::endl;
using std::ostream;
using std::string;
using std::map;

BESXDRequestHandler::BESXDRequestHandler(const string &name) :
        BESRequestHandler(name)
{
    add_method(HELP_RESPONSE, BESXDRequestHandler::dap_build_help);
    add_method(VERS_RESPONSE, BESXDRequestHandler::dap_build_version);
}

BESXDRequestHandler::~BESXDRequestHandler()
{
}

bool BESXDRequestHandler::dap_build_help(BESDataHandlerInterface &dhi)
{
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESInfo *info = dynamic_cast<BESInfo *>(response);
    if (!info)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    map < string, string, std::less<> > attrs;
    attrs["name"] = MODULE_NAME ;
    attrs["version"] = MODULE_VERSION ;
#if 0
    attrs["name"] = PACKAGE_NAME;
    attrs["version"] = PACKAGE_VERSION;
#endif
    info->begin_tag("module", &attrs);
    info->end_tag("module");

    return true;
}

bool BESXDRequestHandler::dap_build_version(BESDataHandlerInterface &dhi)
{
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESVersionInfo *info = dynamic_cast<BESVersionInfo *>(response);
    if (!info)
        throw BESInternalError("cast error", __FILE__, __LINE__);

#if 0
    info->add_module(PACKAGE_NAME, PACKAGE_VERSION);
#endif
    info->add_module(MODULE_NAME, MODULE_VERSION);

    return true;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESXDRequestHandler::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESXDRequestHandler::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESRequestHandler::dump(strm);
    BESIndent::UnIndent();
}

