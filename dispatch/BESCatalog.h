// BESCatalog.h

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

#ifndef I_BESCatalog_h
#define I_BESCatalog_h 1

#include <string>

#include "BESObj.h"

class BESCatalogEntry;

namespace bes {
    class CatalogNode;
}

/** @brief Catalogs provide a hierarchical organization for data.
 *
 */
class BESCatalog: public BESObj {
private:
    std::string d_catalog_name;
    unsigned int d_reference;

    BESCatalog();

public:
    BESCatalog(const std::string &catalog_name) : d_catalog_name(catalog_name), d_reference(0)
    {
    }

    virtual ~BESCatalog()
    {
    }

    virtual void reference_catalog()
    {
        d_reference++;
    }

    virtual unsigned int dereference_catalog()
    {
        if (!d_reference)
            return d_reference;
        return --d_reference;
    }

    virtual std::string get_catalog_name() const
    {
        return d_catalog_name;
    }

    virtual BESCatalogEntry * show_catalog(const std::string &container, BESCatalogEntry *entry) = 0;

    /**
     * The 'root prefix' for a catalog. For catalogs rooted in the file system,
     * this is the pathname to that directory. If the idea of a 'root prefix'
     * makes no sense for a particular kind of catalog, this should be the empty
     * string.
     *
     * @return The root prefix for the catalog.
     */
    virtual std::string get_root() const = 0;

    // Based on other code (show_catalogs()), use BESCatalogUtils::exclude() on
    // a directory, but BESCatalogUtils::include() on a file.
    virtual bes::CatalogNode *get_node(const std::string &path) const = 0;

    virtual void get_site_map(const string &prefix, const string &node_suffix, const string &leaf_suffix, ostream &out,
        const string &path = "/") const = 0;

    virtual void dump(ostream &strm) const = 0;
};

#endif // I_BESCatalog_h
