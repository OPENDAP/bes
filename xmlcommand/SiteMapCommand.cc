// SiteMapCommand.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2018 OPeNDAP, Inc.
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

#include <string>
#include <fstream>
#include <memory>

#include "BESDataHandlerInterface.h"
#include "BESCatalog.h"
#include "BESCatalogList.h"

#include "BESXMLUtils.h"
#include "BESSyntaxUserError.h"
#include "BESDebug.h"
#include "BESNames.h"

#include "SiteMapCommand.h"

using namespace std;

SiteMapCommand::SiteMapCommand(const BESDataHandlerInterface &base_dhi) :
    BESXMLCommand(base_dhi)
{
}

/**
 * @brief Parse the build site map command
 *
 * ~~~{.xml}
 * <buildSiteMap prefix="..." nodeSuffix="..." leafSuffix="..." catalog="..." filename="..."/>
 * ~~~
 * See the class documentation for more information
 *
 * @param node xml2 element node pointer
 */
void SiteMapCommand::parse_request(xmlNode *node)
{
    string name;
    string value;
    map<string, string> props;
    BESXMLUtils::GetNodeInfo(node, name, value, props);
    if (name != SITE_MAP_RESPONSE_STR) {
        throw BESSyntaxUserError(string("The specified command ") + name + " is not a build site map command", __FILE__, __LINE__);
    }

    d_xmlcmd_dhi.data[SITE_MAP_RESPONSE] = SITE_MAP_RESPONSE;

    if (props[PREFIX].empty() || (props[NODE_SUFFIX].empty() && props[LEAF_SUFFIX].empty()))
        throw BESSyntaxUserError("Build site map must include the prefix and at least one of the nodeSuffix or leafSuffix attributes.", __FILE__, __LINE__);

    d_xmlcmd_dhi.data[PREFIX] = props[PREFIX];
    d_xmlcmd_dhi.data[NODE_SUFFIX] = props[NODE_SUFFIX];
    d_xmlcmd_dhi.data[LEAF_SUFFIX] = props[LEAF_SUFFIX];

    // TODO Emulate the new catalog behavior of showNode - where the code figures out the
    // catalog based on the path (which is called 'prefix' here. jhrg 11/27/18
    string catalog_name = props["catalog"].empty() ? BESCatalogList::TheCatalogList()->default_catalog_name(): props["catalog"];
    if (!BESCatalogList::TheCatalogList()->find_catalog(catalog_name))
        throw BESSyntaxUserError(string("Build site map could not find the catalog: ") + catalog_name, __FILE__, __LINE__);

    d_xmlcmd_dhi.data["catalog"] = catalog_name;

    d_cmd_log_info = string("show siteMap: build site map for catalog '").append(catalog_name).append("'.");

    // These values control the ResponseHandler set in the set_response() call below.
    // jhrg 11/26/18
    d_xmlcmd_dhi.action_name = SITE_MAP_RESPONSE_STR;
    d_xmlcmd_dhi.action = SITE_MAP_RESPONSE;

    // Set the response handler for the action in the command's DHI using the value of
    // the DHI.action field. And, as a bonus, copy the d_cmd_log_info into DHI.data[LOG_INFO]
    // and if VERBOSE logging is on, log that the command has been parsed.
    BESXMLCommand::set_response();
}

/**
 * @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void SiteMapCommand::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "SiteMapCommand::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESXMLCommand::dump(strm);
    BESIndent::UnIndent();
}

/**
 * @brief Factory for the SiteMapCommand
 *
 * @param base_dhi Used to pass the 'action' info to BESXMLCommand::set_response()
 * @return
 */
BESXMLCommand *
SiteMapCommand::CommandBuilder(const BESDataHandlerInterface &base_dhi)
{
    return new SiteMapCommand(base_dhi);
}

