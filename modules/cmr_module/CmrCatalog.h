// -*- mode: c++; c-basic-offset:4 -*-
//
// CMRCatalog.h
//
// This file is part of BES cmr_module
//
// Copyright (c) 2018 OPeNDAP, Inc.
// Author: Nathan Potter <ndp@opendap.org>
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
// Please read the full copyright statement in the file COPYRIGHT_URI.
//

#ifndef I_CmrCatalog_h
#define I_CmrCatalog_h 1

#include <list>
#include <string>
#include <vector>

#include "BESCatalog.h"
#include "BESInternalError.h"
#include "CmrNames.h"

class BESCatalogEntry;
class BESCatalogUtils;

namespace bes {
    class CatalogNode;
}


namespace cmr {
/** @brief builds catalogs from a directory structure
 */
class CmrCatalog: public BESCatalog {
private:
    std::vector<std::string> d_collections;
    std::vector<std::string> d_facets;


public:
    CmrCatalog(const std::string &name = CMR_CATALOG_NAME);
    virtual ~CmrCatalog();

    /**
     * @Deprecated
     */
    virtual BESCatalogEntry * show_catalog(const std::string &container, BESCatalogEntry */*entry*/){
        throw BESInternalError("The CMRCatalog::show_catalog() method is not supported. (container: '" + container + "')",__FILE__,__LINE__);
    }

    /**
     * This is a meaningless method for CMR so it returns empty string
     */
    virtual std::string get_root() const { return ""; }

    /**
     * Maybe someday...
     */
    virtual void get_site_map(const std::string &/*prefix*/, const std::string &/*node_suffix*/, const std::string &/*leaf_suffix*/, std::ostream &/*out*/,
        const std::string &/*path = "/"*/) const {
        BESDEBUG(MODULE, "The CMRCatalog::get_site_map() method is not currently supported. SKIPPING. file: " << __FILE__ << " line: "  << __LINE__ << std::endl);
        // throw BESInternalError("The CMRCatalog::get_site_map() method is not currently supported.",__FILE__,__LINE__);
    }


    virtual bes::CatalogNode *get_node(const std::string &path) const;
    virtual bes::CatalogNode *get_node_OLD(const std::string &path) const;
    virtual bes::CatalogNode *get_node_NEW(const std::string &path) const;


    virtual void dump(std::ostream &strm) const;

};
} // namespace cmr

#endif // I_CmrCatalog_h

