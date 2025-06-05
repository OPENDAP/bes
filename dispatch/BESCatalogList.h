// BESCatalogList.h

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

#ifndef BESCatalogList_h_
#define BESCatalogList_h_ 1

#include <map>
#include <string>

#include "BESObj.h"
#include "BESDataHandlerInterface.h"

class BESCatalog;
class BESCatalogEntry;

#define BES_DEFAULT_CATALOG "catalog"

// TODO Oddly, users of this class must register the default catalog
// just like any other catalog. I think this is a design bug - since
// the default is _required_ it should be registered by the BESCatalogList
// constructor. If that change is made, then handlers that call add_catlog()
// and ref_catalog() should be examined and all those that do so should be
// modified.
//
// One way to make this change and not leave the whole ref/deref model
// looking odd would be to make a single BESCatalog instance that is
// the default catalog and have that be separate from the list of added
// catalogs. jhrg 2.25.18


/** @brief List of all registered catalogs
 *
 * Catalogs are a way of organizing data into a tree.
 * Every BES daemon has at least one catalog, the default catalog
 * (confusingly named 'catalog.') In general, this is the
 * daemon's local file system.
 *
 * Multiple BESCatalog objects can be registered with this list.
 * However, most installations have a single catalog registered
 * (the default catalog) which provides access to files on the host
 * computer's local file system.
 *
 * Each catalog in the list must have a unique name.
 *
 * The BESCatalogList class is a singleton. The catalogs (represented
 * by specializations of the BESCatalog class) are held in a
 * reference-counted list. Handlers typically try to add the catalog
 * they use, and if that fails because the catalog is already in the
 * list, they increment its reference. See add_catalog() and ref_catalog().
 * When the singleton's instance is deleted, so are all of the catalogs,
 * regardless of their reference count.
 *
 * @see BESCatalog
 */
class BESCatalogList: public BESObj {
private:
    std::map<std::string, BESCatalog *> d_catalogs;

    // Record the default catalog name and hold a pointer to the object.
    // The BESCatalog* should also be in the d_catalogs map (above) so
    // that clients which search for it will find it that way.
    std::string d_default_catalog_name;
    BESCatalog *d_default_catalog;

    static BESCatalogList * d_instance;

    static void initialize_instance();  // originally used with pthread_once(). jhrg 7/22/18
    static void delete_instance();

    friend class BESCatalogListTest;

    BESCatalogList();

public:
    using catalog_iter = std::map<std::string, BESCatalog *>::iterator;
    using catalog_citer = std::map<std::string, BESCatalog *>::const_iterator;

    BESCatalogList(const BESCatalogList&) = delete;
    BESCatalogList& operator=(const BESCatalogList&) = delete;

    static BESCatalogList * TheCatalogList();

    ~BESCatalogList() override = default;

    /// @brief The number of non-default catalogs
    /// @todo Change this to include the default!
    virtual int num_catalogs() const { return d_catalogs.size();  }
    virtual int empty() const { return d_catalogs.empty();  }

    /// @brief The name of the default catalog
    virtual std::string default_catalog_name() const { return d_default_catalog_name; }
    /// @brief The  the default catalog
    virtual BESCatalog *default_catalog() const { return d_default_catalog; }

    virtual bool add_catalog(BESCatalog *catalog);
    virtual bool ref_catalog(const std::string &catalog_name);
    virtual bool deref_catalog(const std::string &catalog_name);

    virtual BESCatalog * find_catalog(const std::string &catalog_name) const;

    // TODO Remove this ASAP. jhrg 7/22/18
    virtual BESCatalogEntry * show_catalogs(BESCatalogEntry *entry, bool show_default = true);

    /// @brief Iterator to the first catalog
    virtual catalog_citer first_catalog() const { return d_catalogs.begin(); }
    
    /// @brief Iterator to the last catalog
    virtual catalog_citer end_catalog() const { return d_catalogs.end();  }

    virtual void dump(std::ostream &strm) const;
};

#endif // BESCatalogList_h_

