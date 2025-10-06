// BESXMLDapCommandModule.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the DAP Data Access Protocol.

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

#include "config.h"

#include <iostream>

#include "BESResponseHandlerList.h"
#include "BESXMLDapCommandModule.h"
#include "BESDapNames.h"
#include "BESNames.h"
#include "BESDebug.h"
#include "BESXMLCatalogCommand.h"
// #include "SiteMapCommandNames.h"
#include "SiteMapCommand.h"     // Uses NullResponseHandler
#include "SiteMapResponseHandler.h"
#include "ShowNodeCommand.h"    // Could use NullResponseHandler. jhrg 7/23/18

using namespace bes;
using std::endl;
using std::cout;
using std::ostream;
using std::string;

/** @brief Adds catalog and site map commands to the BES
 *
 * Add to the request commands this BES can handle. The showCatalog and showNode
 * commands provide a way for a client of the BES to learn about a catalog. The
 * buildSiteMap command returns a site map of the BES's catalogs. For the
 * buildSiteMap command, this method also adds a ResponseHandler. The showCatalog
 * and showNode commands use the existing
 *
 * @param modname The name of the module being loaded and initialized
 */
void BESXMLDapCommandModule::initialize(const string &/*modname*/)
{
    BESDEBUG("dap", "Initializing DAP Commands:" << endl);

    BESXMLCommand::add_command(CATALOG_RESPONSE_STR, BESXMLCatalogCommand::CommandBuilder);
    BESXMLCommand::add_command(NODE_RESPONSE_STR, ShowNodeCommand::CommandBuilder);

    // Build a site map. Originally used the Null Response Handler, now
    // it returns a response to the caller. jhrg 11/26/18
    BESXMLCommand::add_command(SITE_MAP_RESPONSE_STR, SiteMapCommand::CommandBuilder);
    BESResponseHandlerList::TheList()->add_handler(SITE_MAP_RESPONSE, SiteMapResponseHandler::SiteMapResponseBuilder);

    BESDEBUG("dap", "Done Initializing DAP Commands:" << endl);
}

/** @brief Cleans up the DAP XML commands from the list of possible commands
 *
 * When the BES is being shut down, each dynamically loaded module is
 * allowed to clean up after itself before the module is unloaded. This
 * function is called to do the cleanup work for the DAP XML command module
 *
 * @param modname The name of the DAP XML command module.
 */
void BESXMLDapCommandModule::terminate(const string &/*modname*/)
{
    BESDEBUG("dap", "Removing DAP Commands" << endl);

    BESXMLCommand::del_command(CATALOG_RESPONSE_STR);
    BESXMLCommand::del_command(SITE_MAP_RESPONSE_STR);

    BESResponseHandlerList::TheList()->remove_handler(SITE_MAP_RESPONSE);

    BESDEBUG("dap", "Done Removing DAP Commands" << endl);
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this class
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESXMLDapCommandModule::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESXMLDapCommandModule::dump - (" << (void *) this << ")" << endl;
}

extern "C" {

BESAbstractModule *maker()
{
    return new BESXMLDapCommandModule;
}

}

