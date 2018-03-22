// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the OPeNDAP Back-End Server (BES)

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
#include <ostream>

#include "BESIndent.h"

#include "BESCatalogList.h"
#include "CatalogNode.h"
#include "CatalogItem.h"

using namespace bes;
using namespace std;

CatalogNode::~CatalogNode()
{
#if ITEMS
    for (std::vector<CatalogItem*>::iterator i = d_items.begin(), e = d_items.end(); i != e; ++i)
        delete *i;
    d_items.clear();
#endif

#if NODES_AND_LEAVES
    for (std::vector<CatalogItem*>::iterator i = d_nodes.begin(), e = d_nodes.end(); i != e; ++i)
        delete *i;
    d_nodes.clear();

    for (std::vector<CatalogItem*>::iterator i = d_leaves.begin(), e = d_leaves.end(); i != e; ++i)
        delete *i;
    d_leaves.clear();
#endif
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
#if ITEMS
    strm << BESIndent::LMarg << "item count: " << d_items.size() << endl;
#endif

#if NODES_AND_LEAVES
    strm << BESIndent::LMarg << "item count: " << d_nodes.size() + d_leaves.size() << endl;
#endif

    strm << BESIndent::LMarg << "items: ";
    if (d_nodes.size() + d_leaves.size()) {
        strm << endl;
        BESIndent::Indent();
#if ITEMS
        vector<CatalogItem*>::const_iterator i = d_items.begin();
        vector<CatalogItem*>::const_iterator e = d_items.end();
        for (; i != e; ++i) {
            strm << BESIndent::LMarg << (*i) << endl;
        }
#endif
#if NODES_AND_LEAVES
        vector<CatalogItem*>::const_iterator i = d_nodes.begin();
        vector<CatalogItem*>::const_iterator e = d_nodes.end();
        for (; i != e; ++i) {
            strm << BESIndent::LMarg << (*i) << endl;
        }

        i = d_leaves.begin();
        e = d_leaves.end();
        for (; i != e; ++i) {
            strm << BESIndent::LMarg << (*i) << endl;
        }
#endif
        BESIndent::UnIndent();
    }

    BESIndent::UnIndent();
}
