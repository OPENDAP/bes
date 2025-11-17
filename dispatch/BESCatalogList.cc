// BESCatalogList.cc

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

#include "config.h"

#include <cstdlib>
#include <sstream>

#include "BESCatalog.h"
#include "BESCatalogDirectory.h"
#include "BESCatalogEntry.h"
#include "BESCatalogList.h"
#include "BESInfo.h"

#include "TheBESKeys.h"

using namespace std;

BESCatalogList *BESCatalogList::d_instance = 0;

/** @brief Get the singleton BESCatalogList instance.
 *
 * This static method returns the instance of this singleton class. It
 * uses the protected constructor below to read the name of the default
 * catalog from the BES's configuration file, using the key "BES.Catalog.Default".
 * If the key is not found or the key lookup fails for any reason, it
 * uses the the value of BES_DEFAULT_CATALOG as defined in this class'
 * header file (currently the confusing name "catalog").
 *
 * The implementation will only build one instance of CatalogList and
 * thereafter simple return that pointer.
 *
 * For this code, the default catalog is implemented suing CatalogDirectory,
 * which exposes the BES's local POSIX file system, rooted at a place set in
 * the BES configuration file.
 *
 * @return A pointer to the CatalogList singleton
 */
BESCatalogList *BESCatalogList::TheCatalogList() {
    static BESCatalogList catalog;
    return &catalog;
}

/** @brief construct a catalog list
 *
 * @see BESCatalog
 */
BESCatalogList::BESCatalogList() {
    bool found = false;
    string key = "BES.Catalog.Default";

    // The only way get_value() throws is when a single key has multiple values.
    // However, TheKeys() throws if the bes.conf file cannot be found.
    // This code should probably allow that to be logged and the server to fail
    // to start, not hide the error. jhrg 7/22/18
    TheBESKeys::TheKeys()->get_value(key, d_default_catalog_name, found);

    if (!found || d_default_catalog_name.empty()) {
        d_default_catalog_name = BES_DEFAULT_CATALOG;
    }

    // Build the default catalog and add it to the map of catalogs. jhrg 7/21/18
    d_default_catalog = new BESCatalogDirectory(d_default_catalog_name);
    add_catalog(d_default_catalog);
}

/** @brief adds the specified catalog to the list
 *
 * Add a catalog to the list of catalogs. If a catalog with the same
 * name already exists, don't add the BESCatalog instance (the test
 * is limited to the BESCatalog object's name) and signal that by returning
 * false. If the catalog was added, return true.
 *
 * @param catalog New catalog to add to the list
 * @return false If a catalog with the given catalog's name
 * already exists. Returns true otherwise.
 * @see BESCatalog
 */
bool BESCatalogList::add_catalog(BESCatalog *catalog) {
    bool result = false;
    if (catalog) {
        if (find_catalog(catalog->get_catalog_name()) == 0) {
            // TODO I have no idea why this code was re-written. jhrg 2.25.18
#if 0
            d_catalogs[catalog->get_catalog_name()] = catalog;
#endif
            string name = catalog->get_catalog_name();
            pair<const string, BESCatalog *> p = make_pair(name, catalog);
            result = d_catalogs.insert(p).second;
#if 0
            result = true;
#endif
        }
    }

    return result;
}

// Modules that call ref_catalog: csv, dap, dmrpp, ff, fits, gdal, hdf4,
// hdf5, ncml, nc, sql. jhrg 2.25.18

/**
 *  @brief reference the specified catalog
 *
 * Search the list for the catalog with the given name. If the
 * catalog exists, reference it and return true. If not found then
 * return false.
 *
 * @note The general use pattern for this method is:
 * <pre>
 *  if (!BESCatalogList::TheCatalogList()->ref_catalog("catalog")) {
 *      BESCatalogList::TheCatalogList()->add_catalog(new BESCatalogDirectory("catalog"));
 *  }
 *  <pre>
 *  If "catalog" cannot be found, it's added. If it is found, its reference
 *  count is incremented. This call is generally made in a Module::initialize()
 *  method (and the matching deref_catalog() call is made in the Module::terminate()
 *  method.
 *
 * @note This is part of a system that lets modules 'reference' a particular
 * catalog so that when all references to it are gone, it can be deleted.
 * Given that Catalog instances are pretty small and only get used when called,
 * I don't think we need this. The destructor for this class, which gets
 * called by at_exit(), will remove all the Catalog instances. The argument
 * for this scheme is that each handler should be managing its use of the BES
 * so that its terminate() method cleans up any resources allocated. Using
 * this won't break anything, so it's easiest to leave it in place. However,
 * if it is used 100% by the handlers, then there should be nothing for the
 * class' dtor to actually delete, as noted there.
 *
 * @param catalog_name name of the catalog to reference
 * @return true if successfully found and referenced, false otherwise
 * @see BESCatalog
 */
bool BESCatalogList::ref_catalog(const string &catalog_name) {
    bool ret = false;
    BESCatalog *cat = 0;
    BESCatalogList::catalog_iter i;
    i = d_catalogs.find(catalog_name);
    if (i != d_catalogs.end()) {
        cat = (*i).second;
        cat->reference_catalog();
        ret = true;
    }
    return ret;
}

// Modules that call deref_catalog: csv, dap, dmrpp, ff, fits, gdal, hdf4,
// hdf5, ncml, nc, sql. jhrg 2.25.18

/**
 * @brief de-reference the specified catalog and remove from list
 * if no longer referenced
 *
 * Search the list for the catalog with the given name. If the
 * catalog exists, de-reference it. If there are no more references
 * then remove the catalog from the list and delete it.
 *
 * @note See the note for BESCatalogList::ref_catalog()
 *
 * @param catalog_name name of the catalog to de-reference
 * @return true if successfully de-referenced, false otherwise
 * @see BESCatalog
 */
bool BESCatalogList::deref_catalog(const string &catalog_name) {
    bool ret = false;
    BESCatalog *cat = 0;
    BESCatalogList::catalog_iter i;
    i = d_catalogs.find(catalog_name);
    if (i != d_catalogs.end()) {
        cat = (*i).second;
        if (!cat->dereference_catalog()) {
            d_catalogs.erase(i);
            delete cat;
        }
        ret = true;
    }
    return ret;
}

/** @brief find the catalog in the list with the specified name
 *
 * @param catalog_name name of the catalog to find
 * @return a BESCatalog with the given name if found, 0 otherwise
 * @see BESCatalog
 */
BESCatalog *BESCatalogList::find_catalog(const string &catalog_name) const {
    BESCatalogList::catalog_citer i = d_catalogs.find(catalog_name);
    if (i != d_catalogs.end()) {
        return (*i).second;
    }
    return 0;
}

/**
 * @brief Return a CatalogEntry object listing the BES's catalogs
 *
 * Build a CatalogEntry object that lists all of the registered catalogs. This
 * enables a BES with more than just the default catalog to present them as group
 * 'rooted' at "/".
 *
 * @note This method is currently called only by BESCatalogResponseHandler::execute()
 * when the 'showCatalog' command is responding to information about "/" and
 * there is more than just a single catalog configured for the BES. Note that there
 * is always a default catalog in the BES.
 *
 * @note The original documentation for this method claimed it never threw an exception,
 * but BESCatalogDirectory (one - the only? - implementation of BESCatalog::show_catalog)
 * throws exceptions for several conditions.
 *
 * @todo This is only used in BESCatalogResposeHandler::execute() for a condition that
 * the OLFS will never trigger. When the showCatalog command is replaces by showNode, this
 * method can be removed. jhrg 7/22/18
 *
 * @param entry If null, make a new entry and use it to return the information, otherwise
 * add the information about the catalogs to the passed instance.
 * @param show_default If true, include information about the default catalog, if false,
 * don't. True by default.
 * @return A BESCatalogEntry instance with information about all of the catalogs. If
 * the 'entry' parameter was null, the caller is responsible for deleting the
 * returned object.
 */
BESCatalogEntry *BESCatalogList::show_catalogs(BESCatalogEntry *entry, bool show_default) {
    BESCatalogEntry *myentry = entry;
    if (!myentry) {
        myentry = new BESCatalogEntry("/", "");
    }
    catalog_citer i = d_catalogs.begin();
    catalog_citer e = d_catalogs.end();
    for (; i != e; i++) {
        // if show_default is true then display all catalogs
        // if !show_default but this current catalog is not the default
        // then display
        if (show_default || (*i).first != default_catalog_name()) {
            BESCatalog *catalog = (*i).second;
            catalog->show_catalog("", myentry);
        }
    }

    return myentry;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with the catalogs
 * registered in this list.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESCatalogList::dump(ostream &strm) const {
    strm << BESIndent::LMarg << "BESCatalogList::dump - (" << (void *)this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "default catalog: " << d_default_catalog_name << endl;
    if (d_catalogs.size()) {
        strm << BESIndent::LMarg << "catalog list:" << endl;
        BESIndent::Indent();
        catalog_citer i = d_catalogs.begin();
        catalog_citer e = d_catalogs.end();
        for (; i != e; i++) {
            BESCatalog *catalog = (*i).second;
            strm << BESIndent::LMarg << (*i).first << catalog << endl;
        }
        BESIndent::UnIndent();
    } else {
        strm << BESIndent::LMarg << "catalog list: empty" << endl;
    }
    BESIndent::UnIndent();
}
