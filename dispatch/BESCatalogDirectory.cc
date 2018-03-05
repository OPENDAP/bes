// BESCatalogDirectory.cc

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

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <cstring>
#include <cerrno>
#include <sstream>

using std::stringstream;
using std::endl;

#include "BESUtil.h"
#include "BESCatalogDirectory.h"
#include "BESCatalogUtils.h"
#include "BESCatalogEntry.h"
#include "BESInfo.h"
#include "BESCatalogUtils.h"
#include "BESContainerStorageList.h"
#include "BESContainerStorageCatalog.h"
#include "BESLog.h"
#include "BESForbiddenError.h"
#include "BESNotFoundError.h"
#include "BESDebug.h"

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
BESCatalogDirectory::BESCatalogDirectory(const string &name) :
        BESCatalog(name)
{
    d_utils = BESCatalogUtils::Utils(name);
}

BESCatalogDirectory::~BESCatalogDirectory()
{
}

/**
 *
 * @param node
 * @param coi Either the string "show.Info" or "show.Catalog"
 * @param entry
 * @return
 */
BESCatalogEntry *
BESCatalogDirectory::show_catalog(const string &node, BESCatalogEntry *entry)
{
    string use_node = node;
    // use_node should only end in '/' if that's the only character in which
    // case there's no need to call find()
    if (!node.empty() && node != "/") {
        string::size_type pos = use_node.find_last_not_of("/");
        use_node = use_node.substr(0, pos + 1);
    }

    // This takes care of bizarre cases like "///" where use_node would be
    // empty after the substring call.
    if (use_node.empty())
        use_node = "/";

    string rootdir = d_utils->get_root_dir();
    string fullnode = rootdir;
    if (!use_node.empty()) {
        // TODO It's hard to know just what this code is supposed to do, but I
        // think the following can be an error. Above, if use_node is empty(), the use_node becomes
        // "/" and then it's not empty() and fullnode becomes "<stuff>//" but we just
        // jumped through all kinds of hoops to make sure there was either zero
        // or one trailing slash. jhrg 2.26.18
        fullnode = fullnode + "/" + use_node;
    }

    string basename;
    string::size_type slash = fullnode.rfind("/");
    if (slash != string::npos) {
        basename = fullnode.substr(slash + 1, fullnode.length() - slash);
    }
    else {
        basename = fullnode;
    }

    // fullnode is the full pathname of the node, including the 'root' pathanme
    // basename is the last component of fullnode

    BESDEBUG( "bes", "BESCatalogDirectory::show_catalog: "
            << "use_node = " << use_node << endl
            << "rootdir = " << rootdir << endl
            << "fullnode = " << fullnode << endl
            << "basename = " << basename << endl );

    // This will throw the appropriate exception (Forbidden or Not Found).
    // Checks to make sure the different elements of the path are not
    // symbolic links if follow_sym_links is set to false, and checks to
    // make sure have permission to access node and the node exists.
    // TODO Move up; this canbe done once use_node is set. jhrg 2.26.18
    BESUtil::check_path(use_node, rootdir, d_utils->follow_sym_links());

    // If null is passed in, then return the new entry, else add the new entry to the
    // existing Entry object. jhrg 2.26.18
    BESCatalogEntry *myentry = new BESCatalogEntry(use_node, get_catalog_name());
    if (entry) {
        // if an entry was passed, then add this one to it
        entry->add_entry(myentry);
    }
    else {
        // else we want to return the new entry created
        entry = myentry;
    }

    // Is this node a directory?
    // TODO use stat() instead. jhrg 2.26.18
    DIR *dip = opendir(fullnode.c_str());
    if (dip != NULL) {
        try {
            // The node is a directory

            // if the directory requested is in the exclude list then we won't
            // let the user see it.
            if (d_utils->exclude(basename)) {
                string error = "You do not have permission to view the node " + use_node;
                throw BESForbiddenError(error, __FILE__, __LINE__);
            }

            // Now that we are ready to start building the response data we
            // cancel any pending timeout alarm according to the configuration.
            BESUtil::conditional_timeout_cancel();

            bool dirs_only = false;
            // TODO This is the only place in the code where get_entries() is called
            // jhrg 2.26.18
            d_utils->get_entries(dip, fullnode, use_node, myentry, dirs_only);
        } catch (... /*BESError &e */) {
            closedir(dip);
            throw /* e */;
        }
        closedir(dip);

        // TODO This is the only place this method is called. replace the static method
        // with an object call (i.e., d_utils)? jhrg 2.26.18
        BESCatalogUtils::bes_add_stat_info(myentry, fullnode);
    }
    else {
        // if the node is not in the include list then the requester does
        // not have access to that node
        if (d_utils->include(basename)) {
            struct stat buf;
            int statret = 0;
            if (d_utils->follow_sym_links() == false) {
                /*statret =*/(void) lstat(fullnode.c_str(), &buf);
                if (S_ISLNK(buf.st_mode)) {
                    string error = "You do not have permission to access node " + use_node;
                    throw BESForbiddenError(error, __FILE__, __LINE__);
                }
            }
            statret = stat(fullnode.c_str(), &buf);
            if (statret == 0 && S_ISREG(buf.st_mode)) {
                BESCatalogUtils::bes_add_stat_info(myentry, fullnode);

                list < string > services;
                BESCatalogUtils::isData(node, get_catalog_name(), services);
                myentry->set_service_list(services);
            }
            else if (statret == 0) {
                string error = "You do not have permission to access " + use_node;
                throw BESForbiddenError(error, __FILE__, __LINE__);
            }
            else {
                // ENOENT means that the path or part of the path does not
                // exist
                if (errno == ENOENT) {
                    string error = "Node " + use_node + " does not exist";
                    char *s_err = strerror(errno);
                    if (s_err) {
                        error = s_err;
                    }
                    throw BESNotFoundError(error, __FILE__, __LINE__);
                }
                // any other error means that access is denied for some reason
                else {
                    string error = "Access denied for node " + use_node;
                    char *s_err = strerror(errno);
                    if (s_err) {
                        error = error + s_err;
                    }
                    throw BESNotFoundError(error, __FILE__, __LINE__);
                }
            }
        }
        else {
            string error = "You do not have permission to access " + use_node;
            throw BESForbiddenError(error, __FILE__, __LINE__);
        }
    }

    return entry;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * this catalog directory.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESCatalogDirectory::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESCatalogDirectory::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();

    strm << BESIndent::LMarg << "catalog utilities: " << endl;
    BESIndent::Indent();
    d_utils->dump(strm);
    BESIndent::UnIndent();
    BESIndent::UnIndent();
}

