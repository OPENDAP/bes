// SiteMapCommand.h

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

#ifndef A_SiteMapCommand_h
#define A_SiteMapCommand_h 1

#include "BESXMLCommand.h"

class BESDataHandlerInterface;

/**
 * @brief Build a site map.
 *
 * The buildSiteMap command builds a site map for a particular BES catalog.
 * By default the command uses the default catalog and writes the site map
 * text file (not XML) to site_map.txt in the root directory of the catalog.
 * The site map is not returned to the caller - it is stored by the BES and
 * can be accessed using the OLFS and streaming access to files in the catalog.
 *
 * The command syntax is
 * ~~~{.xml}
 * <buildSiteMap prefix="..." nodeSuffix="..." leafSuffix="..." catalog="..." filename="..."/>
 * ~~~
 * where _catalog_ defaults to the default catalog and _filename_
 * defaults to `site_map.txt`. If nodeSuffix or leafSuffix are the empty string,
 * nodes or leaves will not be added to the site map (resp.). If you want these
 * in the site map without a suffix, use a space (e.g., " ") as the attribute
 * value.
 */
class SiteMapCommand: public BESXMLCommand {
public:
    SiteMapCommand(const BESDataHandlerInterface &base_dhi);

    virtual ~SiteMapCommand()
    {
    }

    virtual void parse_request(xmlNode *node);

    /// @brief This command does not return a response, unless its an error
    virtual bool has_response()
    {
        return false;
    }

    virtual void dump(ostream &strm) const;

    static BESXMLCommand *CommandBuilder(const BESDataHandlerInterface &base_dhi);
};

#endif // A_SiteMapCommand_h

