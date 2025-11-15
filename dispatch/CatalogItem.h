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

#ifndef I_CatalogItem_h
#define I_CatalogItem_h 1

#include <string>
#include <ostream>
#include <utility>

#include "BESObj.h"

class BESInfo;

namespace bes {

#if 0
Class Node {
    Name
    LMT
    Item_count (number of nodes or leaves it holds)
    Items (zero or more Item instances)

    Class Item {
        Name
        Size
        Type (node or leaf)
        Is_data
        LMT
    }
}
#endif

/**
 * An item that is part of a BES Catalog. Catalogs are a simple abstraction
 * for a hierarchical organization of data, equivalent to a tree where a node
 * can have zero or more leaves and zero or more child nodes. in that model,
 * the CatalogItem is a leaf or a node. However, the CatalogItem class does
 * not support building a tree. It will return information about the contents
 * of a node, but not about the _contents_ of that nodes child nodes. See
 * CatalogNode and BESCatalog for a more complete picture of how this class
 * is used.
 *
 * In the Catalog abstraction, a leaf may be either a simple file that the
 * BES will not process other than to stream its bytes to the client, or it
 * may be a data file that the BES can open, read, and interpret. A node is
 * always just a node. It may be empty, have only other nodes, have only
 * leaves or have a combination of the two.
 *
 * @see CatalogNode
 * @see BESCatalog
 */
class CatalogItem: public BESObj {
public:
    enum item_type {unknown, node, leaf};

private:
    std::string d_name;
    size_t d_size{0};
    std::string d_lmt;
    bool d_is_data{false};
    item_type d_type;
    std::string d_description;
    std::string d_dap_service_url;

    CatalogItem(const CatalogItem &rhs);
    CatalogItem &operator=(const CatalogItem &rhs);

public:
    /// @brief Make an empty instance.
    CatalogItem() : d_type(unknown) { }

    /**
     * @brief Hold information about an item in a BES Catalog
     *
     * Store information about an item. Sets the id_data() property to false.
     *
     * To determine if a leaf item is data, the name must match a regular
     * expression that is used to identify data objects (typically files).
     * See BESCatalogUtils for help in doing that.
     *
     * @param name
     * @param size
     * @param lmt
     * @param type
     */
    CatalogItem(const std::string &name, size_t size, const std::string &lmt, item_type type)
        : d_name(name), d_size(size), d_lmt(lmt), d_type(type) { }

    /**
     * @brief Hold information about an item in a BES Catalog
     *
     * Store information about an item. Sets the id_data() property to false.
     *
     * To determine if a leaf item is data, the name must match a regular
     * expression that is used to identify data objects (typically files).
     * See BESCatalogUtils for help in doing that.
     *
     * This const
     * @param name
     * @param size
     * @param lmt
     * @param is_data
     * @param type
     */
    CatalogItem(const std::string &name, size_t size, const std::string &lmt, bool is_data, item_type type)
        : d_name(name), d_size(size), d_lmt(lmt), d_is_data(is_data), d_type(type) { }

    virtual ~CatalogItem() { }

    struct CatalogItemAscending {
      bool operator() (CatalogItem *i,CatalogItem *j) { return (i->d_name < j->d_name); }
    };

    /// @brief The name of this item in the node
    std::string get_name() const { return d_name; }
    /// @brief Set the name of the item
    void set_name(const std::string &n) { d_name = n; }

    /// @brief The descrtiption of this item
    std::string get_description() const { return d_description; }
    /// @brief Set the name of the item
    void set_description(const std::string &n) { d_description = n; }

    /// @brief The size (bytes) of the item
    size_t get_size() const { return d_size; }
    /// @brief Set the size of the item
    void set_size(size_t s) { d_size = s; }

    /// @brief Get the last modified time for this item
    std::string get_lmt() const { return d_lmt; }
    /// @brief Set the LMT for this item.
    void set_lmt(const std::string &lmt) { d_lmt = lmt; }

    /// @brief Is this item recognized as data?
    bool is_data() const { return d_is_data; }
    /// @brief Is this item data that the BES should interpret?
    void set_is_data(bool id) { d_is_data = id; }

    /// @brief The DAP Dataset URL for an external DAP service.
    std::string get_dap_service_url() const { return d_dap_service_url; }
    /// @brief Is this item data that the BES should interpret?
    void set_dap_service_url( const std::string &url) { d_dap_service_url = url; }

    /// @brief Get the type of this item (unknown, node or leaf)
    item_type get_type() const { return d_type; }
    /// @brief Set the type for this item
    void set_type(item_type t) { d_type = t; }

    void encode_item(BESInfo *info) const;

    void dump(std::ostream &strm) const override;
};

} // namespace bes

#endif // I_BESCatalogItem_h
