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

#ifndef I_CMRCatalog_h
#define I_CMRCatalog_h 1

#include <list>
#include <string>

using std::list;
using std::string;

#include "BESCatalog.h"

class BESCatalogEntry;
class BESCatalogUtils;

namespace bes {
    class CatalogNode;
}

namespace cmr {
/** @brief builds catalogs from a directory structure
 */
class CMRCatalog: public BESCatalog {
private:
    BESCatalogUtils * d_utils;

public:
    CMRCatalog(const string &name);
    virtual ~CMRCatalog();

    virtual BESCatalogEntry * show_catalog(const string &container, BESCatalogEntry *entry);

    virtual std::string get_root() const;

    virtual bes::CatalogNode *get_node(const std::string &path) const;

    virtual void get_site_map(const string &prefix, const string &node_suffix, const string &leaf_suffix, ostream &out,
        const string &path = "/") const = 0;

    virtual void dump(ostream &strm) const;
};
} // namespace cmr

#endif // I_CMRCatalog_h

