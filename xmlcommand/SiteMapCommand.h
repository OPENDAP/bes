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
 * This command emulates the behavior of showNode in that it builds a site
 * map that shows all of the BES's catalogs and inserts the non-default catalogs
 * as top-level 'directories' in the default catalog. The site map is returned
 * to the caller as a text document, with one item per line.
 *
 * The command syntax is
 * ~~~{.xml}
 * <buildSiteMap prefix="..." nodeSuffix="..." leafSuffix="..." catalog="..."/>
 * ~~~
 * where _catalog_ defaults to returning a site map for all of the catalogs
 * using the same convention as the showNode command (as described above). If
 * a catalog name is given, only the information for that catalog will be included,
 * and this includes the default catalog (thus, it's possible to get a site map
 * for the default catalog without touching the other catalogs).
 *
 * If nodeSuffix or leafSuffix are the empty string,
 * nodes or leaves will not be added to the site map (resp.). If you want these
 * in the site map without a suffix, use a space (e.g., " ") as the attribute
 * value. Otherwise, these strings are appended to the values of either Nodes
 * (e.g., directories) or Leafs (e.g., data files).
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
        return true;
    }

    void dump(std::ostream &strm) const override;

    static BESXMLCommand *CommandBuilder(const BESDataHandlerInterface &base_dhi);
};

#endif // A_SiteMapCommand_h

