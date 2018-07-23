// -*- mode: c++; c-basic-offset:4 -*-
//
// CMRCatalog.cc
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

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <cstring>
#include <cerrno>

#include <sstream>
#include <cassert>

#include <memory>
#include <algorithm>

#include "BESUtil.h"
#include "CmrCatalog.h"
#include "BESCatalogUtils.h"
#include "BESCatalogEntry.h"

#include "CatalogNode.h"
#include "CatalogItem.h"

#include "BESInfo.h"
#include "BESContainerStorageList.h"
#include "BESContainerStorageCatalog.h"
#include "BESLog.h"

#include "BESInternalError.h"
#include "BESForbiddenError.h"
#include "BESNotFoundError.h"

#include "BESDebug.h"

using namespace bes;
using namespace std;
using namespace cmr;
/**
 * @brief A catalog for POSIX file systems
 *
 * BESCatalgDirectory is BESCatalog specialized for POSIX file systems.
 * The default catalog is an instance of this class.
 *
 * @note Access to the host's file system is made using BESCatalogUtils,
 * which is initialized using the catalog name.
 *
 * @param name The name of the catalog.
 * @see BESCatalogUtils
 */
CMRCatalog::CMRCatalog(const string &name) :
    BESCatalog(name)
{
    d_utils = BESCatalogUtils::Utils(name);
}

CMRCatalog::~CMRCatalog()
{
}



/**
 * Copied from BESLog, where that code writes to an internal object, not a string.
 *
 * @todo Make this part of a collection of Utility functions
 * @param the_time A time_t value
 * @param use_local_time True to use the local time, false (default) to use GMT
 * @return The time, either local or GMT/UTC as an ISO8601 string
 */
static string get_time(time_t the_time, bool use_local_time = false)
{
    char buf[sizeof "YYYY-MM-DDTHH:MM:SSzone"];
    int status = 0;

    // From StackOverflow:
    // This will work too, if your compiler doesn't support %F or %T:
    // strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M:%S%Z", gmtime(&now));
    //
    // Apologies for the twisted logic - UTC is the default. Override to
    // local time using BES.LogTimeLocal=yes in bes.conf. jhrg 11/15/17
    if (!use_local_time)
        status = strftime(buf, sizeof buf, "%FT%T%Z", gmtime(&the_time));
    else
        status = strftime(buf, sizeof buf, "%FT%T%Z", localtime(&the_time));

    if (!status)
    LOG("Error getting last modified time time for a leaf item in CMRCatalog.");

    return buf;
}

// path must start with a '/'. By this class it will be interpreted as a
// starting at the CatalogDirectory instance's root directory. It may either
// end in a '/' or not.
//
// If it is not a directory - that is an error. (return null or throw?)
//
// Item names are relative

/**
 * @brief Get a CatalogNode for the given path in the current catalog
 *
 * This is similar to show_catalog() but returns a simpler response. The
 * \arg path must start with a slash and is used as a suffix to the Catalog's
 * root directory.
 *
 * @param path The pathname for the node; must start with a slash
 * @return A CatalogNode instance or null if there is no such path in the
 * current catalog.
 * @throw BESInternalError If the \arg path is not a directory
 * @throw BESForbiddenError If the \arg path is explicitly excluded by the
 * bes.conf file
 */
CatalogNode *
CMRCatalog::get_node(const string &path) const
{
    if (path[0] != '/') throw BESInternalError("Catalog paths must start with a slash (/)", __FILE__, __LINE__);

    string rootdir = d_utils->get_root_dir();

    // This will throw the appropriate exception (Forbidden or Not Found).
    // Checks to make sure the different elements of the path are not
    // symbolic links if follow_sym_links is set to false, and checks to
    // make sure have permission to access node and the node exists.
    BESUtil::check_path(path, rootdir, d_utils->follow_sym_links());

    string fullpath = rootdir + path;

    DIR *dip = opendir(fullpath.c_str());
    if (!dip)
        throw BESInternalError(
            "A CMRCatalog can only return nodes for directory. The path '" + path
                + "' is not a directory for BESCatalog '" + get_catalog_name() + "'.", __FILE__, __LINE__);

    try {
        // The node is a directory

        // Based on other code (show_catalogs()), use BESCatalogUtils::exclude() on
        // a directory, but BESCatalogUtils::include() on a file.
        if (d_utils->exclude(path))
            throw BESForbiddenError(
                string("The path '") + path + "' is not included in the catalog '" + get_catalog_name() + "'.",
                __FILE__, __LINE__);

        CatalogNode *node = new CatalogNode(path);

        node->set_catalog_name(get_catalog_name());
        struct stat buf;
        int statret = stat(fullpath.c_str(), &buf);
        if (statret == 0 /* && S_ISDIR(buf.st_mode) */)
            node->set_lmt(get_time(buf.st_mtime));

        struct dirent *dit;
        while ((dit = readdir(dip)) != NULL) {
            string item = dit->d_name;
            if (item == "." || item == "..") continue;

            string item_path = fullpath + "/" + item;

            // TODO add a test in configure for the readdir macro(s) DT_REG, DT_LNK
            // and DT_DIR and use those, if present, to determine if the name is a
            // link, directory or regular file. These are not present on all systems.
            // Also, since we need mtime, this is not a huge time saver. But if we
            // decide not to use the mtime, using these macros could save lots of system
            // calls. jhrg 3/9/18

            // Skip this dir entry if it is a sym link and follow links is false
            if (d_utils->follow_sym_links() == false) {
                struct stat lbuf;
                (void) lstat(item_path.c_str(), &lbuf);
                if (S_ISLNK(lbuf.st_mode)) continue;
            }

            // Is this a directory or a file? Should it be excluded or included?
            statret = stat(item_path.c_str(), &buf);
            if (statret == 0 && S_ISDIR(buf.st_mode) && !d_utils->exclude(item)) {
#if 0
                // Add a new node; set the size to zero.
                node->add_item(new CatalogItem(item, 0, get_time(buf.st_mtime), CatalogItem::node));
#endif
                node->add_node(new CatalogItem(item, 0, get_time(buf.st_mtime), CatalogItem::node));
            }
            else if (statret == 0 && S_ISREG(buf.st_mode) && d_utils->include(item)) {
#if 0
                // Add a new leaf.
                node->add_item(new CatalogItem(item, buf.st_size, get_time(buf.st_mtime),
                    d_utils->is_data(item), CatalogItem::leaf));
#endif
                node->add_leaf(new CatalogItem(item, buf.st_size, get_time(buf.st_mtime),
                    d_utils->is_data(item), CatalogItem::leaf));
            }
            else {
                VERBOSE("Excluded the item '" << item_path << "' from the catalog '" << get_catalog_name() << "' node listing.");
            }
        } // end of the while loop

        closedir(dip);

        CatalogItem::CatalogItemAscending ordering;

        sort(node->nodes_begin(), node->nodes_end(), ordering);
        sort(node->leaves_begin(), node->leaves_end(), ordering);

        return node;
    }
    catch (...) {
        closedir(dip);
        throw;
    }
}


/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * this catalog directory.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void CMRCatalog::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "CMRCatalog::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();

    strm << BESIndent::LMarg << "catalog utilities: " << endl;
    BESIndent::Indent();
    d_utils->dump(strm);
    BESIndent::UnIndent();
    BESIndent::UnIndent();
}

