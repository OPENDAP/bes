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

#ifndef I_CatalogNode_h
#define I_CatalogNode_h 1

#include <string>
#include <vector>
#include <ostream>

#include "BESObj.h"

#define ITEMS 0
#define NODES_AND_LEAVES 1

namespace bes {

class CatalogItem;

class CatalogNode: public BESObj {
private:
    std::string d_name;
    std::string d_catalog_name;
    std::string d_lmt;

#if ITEMS
    std::vector<CatalogItem*> d_items;
#endif

#if NODES_AND_LEAVES
    // Separate the child nodes and leaves so they can be treated as
    // a group more easily (all the nodes listed first, e.g.).
    std::vector<CatalogItem*> d_nodes;
    std::vector<CatalogItem*> d_leaves;
#endif

    CatalogNode(const CatalogNode &rhs);
    CatalogNode &operator=(const CatalogNode &rhs);

public:
    /// @brief Make an empty instance.
    CatalogNode() : d_name(""), d_catalog_name(""), d_lmt("") { }

    CatalogNode(const std::string &name) : d_name(name), d_catalog_name(""), d_lmt("") { }

    virtual ~CatalogNode();

    /// @brief The name of this node in the catalog
    std::string get_name() const { return d_name; }
    /// @brief Set the name of the catalog's node
    void set_name(std::string n) { d_name = n; }

    /// @brief The name of the catalog
    std::string get_catalog_name() const { return d_catalog_name; }
    /// @brief Set the name of the catalog
    void set_catalog_name(std::string cn) { d_catalog_name = cn; }

    /// @brief Get the last modified time for this node
    std::string get_lmt() const { return d_lmt; }
    /// @brief Set the LMT for this node.
    void set_lmt(std::string lmt) { d_lmt = lmt; }

    typedef std::vector<CatalogItem*>::const_iterator item_citer;
    typedef std::vector<CatalogItem*>::iterator item_iter;

#if ITEMS
    item_citer items_begin() { return d_items.begin(); }
    item_citer items_end() { return d_items.end(); }

    /// @brief How many items are in this node of the catalog?
    size_t get_item_count() const { return d_items.size(); }
    /// @brief Add information about an item that is in  this node of the catalog
    void add_item(CatalogItem *item) { d_items.push_back(item); }
#endif

#if NODES_AND_LEAVES
    item_iter nodes_begin() { return d_nodes.begin(); }
    item_iter nodes_end() { return d_nodes.end(); }

    size_t get_item_count() const { return d_nodes.size() + d_leaves.size(); }

    /// @brief How many nodes are in this node of the catalog?
    size_t get_node_count() const { return d_nodes.size(); }
    /// @brief Add information about an node that is in  this node of the catalog
    void add_node(CatalogItem *node) { d_nodes.push_back(node); }

    item_iter leaves_begin() { return d_leaves.begin(); }
    item_iter leaves_end() { return d_leaves.end(); }

    /// @brief How many leaves are in this node of the catalog?
    size_t get_leaf_count() const { return d_leaves.size(); }
    /// @brief Add information about an leaf that is in  this node of the catalog
    void add_leaf(CatalogItem *leaf) { d_leaves.push_back(leaf); }
#endif

    virtual void dump(ostream &strm) const;
};

} // namespace bes

#endif // I_BESCatalogNode_h
