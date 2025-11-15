// HttpCatalog.h
// -*- mode: c++; c-basic-offset:4 -*-
//
// This file is part of BES httpd_catalog_module
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

#ifndef _HttpdCatalog_h_
#define _HttpdCatalog_h_ 1

#include <list>
#include <string>
#include <map>

#include "BESCatalog.h"
#include "BESInternalError.h"
#include "HttpdCatalogNames.h"

class BESCatalogEntry;
class BESCatalogUtils;

namespace bes {
    class CatalogNode;
}

namespace httpd_catalog {

/**
 * @brief builds catalogs from a directory structure exposed by Apache httpd
 */
class HttpdCatalog: public BESCatalog {
private:
    std::map<std::string,std::string> d_httpd_catalogs;

public:
    HttpdCatalog(const std::string &catalog_name = HTTPD_CATALOG_NAME);

    virtual ~HttpdCatalog() { }

    /**
     * @Deprecated
     */
    virtual BESCatalogEntry * show_catalog(const std::string &container, BESCatalogEntry */*entry*/){
        throw BESInternalError("The HttpdCatalog::show_catalog() method is not supported. (container: '" + container + "')",__FILE__,__LINE__);
    }

    /**
     * This is a meaningless method for a remote catalog so it returns empty string
     */
    virtual std::string get_root() const { return ""; }

    /**
     * Maybe someday...
     */
    virtual void get_site_map(
        const std::string &/*prefix*/,
        const std::string &/*node_suffix*/,
        const std::string &/*leaf_suffix*/,
		std::ostream &/*out*/,
        const std::string &/*path = "/"*/) const {
        BESDEBUG(MODULE, "The HttpdCatalog::get_site_map() method is not currently supported. SKIPPING. file: " << __FILE__ << " line: "  << __LINE__ << std::endl);
    }

    virtual bes::CatalogNode *get_node(const std::string &path) const;

    virtual std::string path_to_access_url(const std::string &path) const;

    void dump(std::ostream &strm) const override;
};

} // namespace httpd_catalog

#endif // _HttpdCatalog_h_

