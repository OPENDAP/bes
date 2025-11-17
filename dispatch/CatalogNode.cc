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

#include <cassert>

#include <ostream>
#include <sstream>
#include <string>

#include "BESIndent.h"
#include "BESUtil.h"

#include "BESCatalogList.h"
#include "BESInfo.h"
#include "CatalogItem.h"
#include "CatalogNode.h"

using namespace bes;
using namespace std;

CatalogNode::~CatalogNode() {
#if ITEMS
    for (std::vector<CatalogItem *>::iterator i = d_items.begin(), e = d_items.end(); i != e; ++i)
        delete *i;
    d_items.clear();
#endif

#if NODES_AND_LEAVES
    for (std::vector<CatalogItem *>::iterator i = d_nodes.begin(), e = d_nodes.end(); i != e; ++i)
        delete *i;
    d_nodes.clear();

    for (std::vector<CatalogItem *>::iterator i = d_leaves.begin(), e = d_leaves.end(); i != e; ++i)
        delete *i;
    d_leaves.clear();
#endif
    delete d_no_really_im_a_leaf;
    d_no_really_im_a_leaf = 0;
    ;
}

/**
 * @brief Encode this CatalogNode in an info object
 *
 * A CatalogNode is encoded as XML using the following grammar, where
 * XML attributes in square brackets are optional.
 * ~~~{.xml}
 * <node name="path" catalog="catalog name" lastModified="date T time"
 *       count="number of child nodes" >
 * ~~~
 * The <node> element may contain zero or more <leaf> elements.
 *
 * @param info Add information to this instance of BESInfo.
 * @see CatalogItem::encode_item()
 */
void CatalogNode::encode_node(BESInfo *info) {
    map<string, string, std::less<>> props;

    // The node may actually be a leaf. Check and act accordingly.
    CatalogItem *im_a_leaf = get_leaf();
    if (im_a_leaf) {
        im_a_leaf->encode_item(info);
    } else { // It's a node. Do the node dance...
        if (get_catalog_name() != BESCatalogList::TheCatalogList()->default_catalog_name())
            props["name"] = BESUtil::assemblePath(get_catalog_name(), get_name(), true);
        else
            props["name"] = get_name();

        // props["catalog"] = get_catalog_name();
        props["lastModified"] = get_lmt();
        ostringstream oss;
        oss << get_item_count();
        props["count"] = oss.str();

        info->begin_tag("node", &props);

        // Depth-first node traversal. Assume the nodes and leaves are sorted.
        // Write the nodes first.
        for (CatalogNode::item_citer i = nodes_begin(), e = nodes_end(); i != e; ++i) {
            assert((*i)->get_type() == CatalogItem::node);
            (*i)->encode_item(info);
        }

        // then leaves
        for (CatalogNode::item_citer i = leaves_begin(), e = leaves_end(); i != e; ++i) {
            assert((*i)->get_type() == CatalogItem::leaf);
            (*i)->encode_item(info);
        }
        info->end_tag("node");
    }
}

/**
 * Dump out information about this object
 * @param strm Write to this stream
 */
void CatalogNode::dump(ostream &strm) const {
    strm << BESIndent::LMarg << "CatalogNode::dump - (" << (void *)this << ")" << endl;
    BESIndent::Indent();

    strm << BESIndent::LMarg << "name: " << d_name << endl;
    strm << BESIndent::LMarg << "catalog_name: " << d_catalog_name << endl;
    strm << BESIndent::LMarg << "last modified time: " << d_lmt << endl;

    if (d_no_really_im_a_leaf) {
        d_no_really_im_a_leaf->dump(strm);
    } else {

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
            vector<CatalogItem *>::const_iterator i = d_items.begin();
            vector<CatalogItem *>::const_iterator e = d_items.end();
            for (; i != e; ++i) {
                strm << BESIndent::LMarg << (*i) << endl;
            }
#endif
#if NODES_AND_LEAVES
            vector<CatalogItem *>::const_iterator i = d_nodes.begin();
            vector<CatalogItem *>::const_iterator e = d_nodes.end();
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
    }
    BESIndent::UnIndent();
}
