// -*- mode: c++; c-basic-offset:4 -*-
//
// DapFunctionsRequestHandler.cc
//
// This file is part of BES DAP Functions module
//
// Copyright (c) 2016 OPeNDAP, Inc.
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

#include "config.h"

#include "BESResponseHandler.h"     // Used to access the DataHandlerInterface
#include "BESResponseNames.h"       // for {HELP,VER}_RESPONSE constants
#include "BESDataHandlerInterface.h" // Used to get the Info object
#include "BESVersionInfo.h"         // Includes BESInfo too

#include "TheBESKeys.h"             // A BES Key can be used to supply help info

#include "DapFunctionsRequestHandler.h"

using std::endl;
using std::ostream;
using std::string;
using std::map;

/** @brief Constructor for FileOut NetCDF module
 *
 * This constructor adds functions to add to the build of a help request
 * and a version request to the BES.
 *
 * @param name The name of the request handler being added to the list
 * of request handlers
 */
DapFunctionsRequestHandler::DapFunctionsRequestHandler(const string &name) :
    BESRequestHandler(name)
{
    add_method( HELP_RESPONSE, DapFunctionsRequestHandler::build_help);
    add_method( VERS_RESPONSE, DapFunctionsRequestHandler::build_version);
}

/** @brief Provides information for the DAP functions help request
 *
 * @param dhi The data interface containing information for the current
 * request to the BES
 * @throws BESInternalError if the response object is not an
 * informational response object.
 */
bool DapFunctionsRequestHandler::build_help(BESDataHandlerInterface &dhi)
{
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESInfo *info = dynamic_cast<BESInfo *>(response);
    if (!info) throw BESInternalError("cast error", __FILE__, __LINE__);

    bool found = false;
    string key = "BES.functions.Reference";
    string ref;
    TheBESKeys::TheKeys()->get_value(key, ref, found);
    if (ref.empty()) ref = "https://docs.opendap.org/index.php/Server_Side_Processing_Functions";

    map<string, string, std::less<>> attrs;
    attrs["name"] = MODULE_NAME;
    attrs["version"] = MODULE_VERSION;
    attrs["reference"] = ref;

    info->begin_tag("module", &attrs);
    info->end_tag("module");

    return true;
}

/** @brief add version information to a version response
 *
 * Adds the version of this module to the version response.
 *
 * @param dhi The data interface containing information for the current
 * request to the BES
 */
bool DapFunctionsRequestHandler::build_version(BESDataHandlerInterface &dhi)
{
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESVersionInfo *info = dynamic_cast<BESVersionInfo *>(response);
    if (!info) throw BESInternalError("cast error", __FILE__, __LINE__);

    info->add_module(MODULE_NAME, MODULE_VERSION);

    return true;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void DapFunctionsRequestHandler::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "DapFunctionsRequestHandler::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESRequestHandler::dump(strm);
    BESIndent::UnIndent();
}

