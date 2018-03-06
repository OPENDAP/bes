// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the OPeNDAP Back-End Server (BES)

// Copyright (c) 2018 OPeNDAP, Inc.
// Author: James Gallagehr <jgallagehr@opendap.org>
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
#include <ostream>

#include "BESIndent.h"

#include "BESCatalogList.h"
#include "CatalogNode.h"
#include "CatalogItem.h"

using namespace bes;
using namespace std;

#if 0
/**
 * @brief Get information about a node in the named catalog
 *
 * @param name
 * @param catalog
 */
CatalogNode::CatalogNode(const string &name, const string &catalog) : d_name(name), d_catalog_name(catalog)
{

}
#endif

CatalogNode::~CatalogNode()
{
    for (std::vector<CatalogItem*>::iterator i = d_items.begin(), e = d_items.end(); i != e; ++i)
        delete *i;
    d_items.clear();
}

/**
 * Dump out information about this object
 * @param strm Write to this stream
 */
void CatalogNode::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "CatalogNode::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();

    strm << BESIndent::LMarg << "name: " << d_name << endl;
    strm << BESIndent::LMarg << "catalog_name: " << d_catalog_name << endl;
    strm << BESIndent::LMarg << "last modified time: " << d_lmt<< endl;
    strm << BESIndent::LMarg << "item count: " << d_items.size() << endl;
    strm << BESIndent::LMarg << "items: ";
    if (d_items.size()) {
        strm << endl;
        BESIndent::Indent();
        vector<CatalogItem*>::const_iterator i = d_items.begin();
        vector<CatalogItem*>::const_iterator e = d_items.end();
        for (; i != e; ++i) {
            strm << BESIndent::LMarg << (*i) << endl;
        }
        BESIndent::UnIndent();
    }
}
