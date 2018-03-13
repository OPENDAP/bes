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

#include "SiteMapCommand.h"
#include "SiteMapCommandNames.h"

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
    if (name != SITE_MAP_STR) {    // buildSiteMap
        throw BESSyntaxUserError(string("The specified command ") + name + " is not a build site map command", __FILE__, __LINE__);
    }

    string prefix = props["prefix"];
    string node_suffix = props["nodeSuffix"];
    string leaf_suffix = props["leafSuffix"];

    string catalog_name = props["catalog"].empty() ? BESCatalogList::TheCatalogList()->default_catalog_name(): props["catalog"];
    string filename = props["filename"].empty() ? "site_map.txt": props["filename"];

    BESDEBUG("besxml", "In SiteMapCommand::parse_request, command attributes: " << prefix << ", "
        << node_suffix << ", " << leaf_suffix << ", " << catalog_name << ", " << filename << endl);

    if (prefix.empty() || catalog_name.empty() || filename.empty() || (node_suffix.empty() && leaf_suffix.empty()))
        throw BESSyntaxUserError("Build site map must include the prefix and at least one of the nodeSuffix or leafSuffix attributes.", __FILE__, __LINE__);

    BESCatalog *catalog = BESCatalogList::TheCatalogList()->find_catalog(catalog_name);
    if (!catalog)
        throw BESSyntaxUserError(string("Build site map could not find the catalog: ") + catalog_name, __FILE__, __LINE__);

    filename.insert(0, catalog->get_root() + "/");  // add root as a prefix to filename
    BESDEBUG("besxml", "filename: " << filename << endl);

    ofstream ofs(filename.c_str(), ios::trunc|ios::binary);
    if (!ofs.is_open())
        throw BESSyntaxUserError(string("Build site map could not write to the site map file: ") + filename, __FILE__, __LINE__);

    catalog->get_site_map(prefix, node_suffix, leaf_suffix, ofs, "/");

    d_cmd_log_info = string("build site map for catalog '").append(catalog_name).append("' and write to '").append(filename);

    // Set action_name here because the NULLResponseHandler (aka NULL_ACTION) won't know
    // which command used it (current ResponseHandlers set this because there is a 1-to-1
    // correlation between XMLCommands and ResponseHanlders). jhrg 2/8/18
    d_xmlcmd_dhi.action_name = SITE_MAP;
    d_xmlcmd_dhi.action = NULL_ACTION;

    // Set the response handler for the action in the command's DHI using the value of
    // the DHI.action field. And, as a bonus, copy the d_cmd_log_info into DHI.data[LOG_INFO]
    // and if VERBOSE logging is on, log that the command has been parsed.
    BESXMLCommand::set_response();
}

/** @brief dumps information about this object
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

