// BESCatalogDirectory.h

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

#ifndef I_BESCatalogDirectory_h
#define I_BESCatalogDirectory_h 1

#include <list>
#include <string>
#include <ostream>

#include "BESCatalog.h"
#include "CatalogItem.h"

class BESCatalogEntry;

namespace bes {
    class CatalogNode;
}

/**
 * @brief Catalogs from a directory structure
 */
class BESCatalogDirectory: public BESCatalog {
private:
    bes::CatalogItem *make_item(std::string item, std::string fullpath) const;
    bes::CatalogItem *make_item(std::string item) const;


public:
    BESCatalogDirectory(const std::string &name);
    virtual ~BESCatalogDirectory();

    virtual BESCatalogEntry * show_catalog(const std::string &container, BESCatalogEntry *entry);

    virtual std::string get_root() const;

    virtual bes::CatalogNode *get_node(const std::string &path) const;

    virtual void get_site_map(const std::string &prefix, const std::string &node_suffix, const std::string &leaf_suffix, std::ostream &out,
        const std::string &path = "/") const;

    void dump(std::ostream &strm) const override;
};

#endif // I_BESCatalogDirectory_h

