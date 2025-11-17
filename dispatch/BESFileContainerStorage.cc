// BESFileContainerStorage.cc

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

#include "BESContainer.h"
#include "BESFileContainerStorage.h"

#include "BESCatalog.h"
#include "BESCatalogList.h"

#include "BESCatalogUtils.h"
#include "BESForbiddenError.h"
#include "BESInfo.h"
#include "BESInternalError.h"
#include "BESRegex.h"
#include "BESServiceRegistry.h"

#include "BESDebug.h"

using std::endl;
using std::list;
using std::ostream;
using std::string;

/** @brief create an instance of this persistent store with the given name.
 *
 * Creates an instances of BESFileContainerStorage with the given name.
 * Looks up the base directory and regular expressions in the bes
 * configuration file using TheBESKeys. Throws an exception if either of
 * these cannot be determined or if the regular expressions are incorrectly
 * formed.
 *
 * ~~~{.xml}
 * <data type>:<reg exp>; <data type>:<reg exp>;
 * ~~~
 *
 * each type/reg expression pair is separated by a semicolon and ends with a
 * semicolon. The data type/expression pair itself is separated by a
 * colon.
 *
 * @param n The name of the Catalog/ContainerStorage (they must be the same).
 * @throws BESForbiddenError if the resources requested is not accessible
 * @throws BESNotFoundError if the resources requested is not found
 * @throws BESInternalError if there is a problem determining the resource
 * @see BESKeys
 * @see BESContainer
 */
BESFileContainerStorage::BESFileContainerStorage(const string &n) : BESContainerStorageVolatile(n) {
    BESCatalog *catalog = BESCatalogList::TheCatalogList()->find_catalog(n);

    _utils = catalog->get_catalog_utils();

    _root_dir = _utils->get_root_dir();
    _follow_sym_links = _utils->follow_sym_links();
}

BESFileContainerStorage::~BESFileContainerStorage() {}

/** @brief adds a container with the provided information
 *
 * If a match is made with the real name passed against the list of regular
 * expressions representing the type of data, then the type is set.
 *
 * The real name of the container (the file name) is constructed using the
 * root directory from the initialization file with the passed real name
 * appended to it.
 *
 * Before adding the actual file name (catalog root directory + real_name
 * passed), the file name is compared against a list of regular expressions
 * representing files that can be included in the catalog and against a list
 * of regular expressions representing files to exclude from the catalog. If
 * the file name is in the include list and not in the exclude list, then it
 * is added to the storage.
 *
 * The information is then passed to the add_container method in the parent
 * class.
 *
 * @param sym_name symbolic name of the container
 * @param real_name real name (path to the file relative to the root
 * catalog's root directory)
 * @param type type of data represented by this container
 *
 * @throws BESForbiddenError if the resources requested is not accessible
 * @throws BESNotFoundError if the resources requested is not found
 * @throws BESInternalError if there is a problem determining the resource
 * determined using the regular expression extensions.
 */
void BESFileContainerStorage::add_container(const string &sym_name, const string &real_name, const string &type) {
    // make sure that the real name passed in is not on the exclude list
    // for the catalog. First, remove any trailing slashes. Then find the
    // basename of the remaining real name. The make sure it's not on the
    // exclude list.
    BESDEBUG("bes", "BESFileContainerStorage::add_container: " << "adding container with name \"" << sym_name
                                                               << "\", real name \"" << real_name << "\", type \""
                                                               << type << "\"" << endl);

    string::size_type stopat = real_name.size() - 1;
    while (real_name[stopat] == '/') {
        stopat--;
    }
    string new_name = real_name.substr(0, stopat + 1);

    string basename;
    string::size_type slash = new_name.rfind("/");
    if (slash != string::npos) {
        basename = new_name.substr(slash + 1, new_name.size() - slash);
    } else {
        basename = new_name;
    }

    // BESCatalogUtils::include method already calls exclude, so just call include
    if (!_utils->include(basename)) {
        string s = "Attempting to create a container with real name '" + real_name +
                   "' which is excluded from the server's catalog.";
        throw BESForbiddenError(s, __FILE__, __LINE__);
    }

    // If the type is specified, then just pass that on. If not, then match
    // it against the types in the type list.
    string new_type = type;
    if (new_type == "") {
        new_type = _utils->get_handler_name(real_name);
    }

    BESContainerStorageVolatile::add_container(sym_name, real_name, new_type);
}

/** @brief is the specified node in question served by a request handler
 *
 * Determine if the node in question is served by a request handler (provides
 * data) and what the request handler serves for the node
 *
 * @param inQuestion node to look up
 * @param provides what is provided for the node by the node type's request handler
 * @return true if a request handler serves the specified node, false otherwise
 */
bool BESFileContainerStorage::isData(const string &inQuestion, list<string> &provides) {
    string node_type = _utils->get_handler_name(inQuestion);

    BESServiceRegistry::TheRegistry()->services_handled(node_type, provides);

    return !node_type.empty(); // Return false if node_type is empty, true if a match is found.
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * the "storage" of containers in a catalog.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESFileContainerStorage::dump(ostream &strm) const {
    strm << BESIndent::LMarg << "BESFileContainerStorage::dump - (" << (void *)this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "name: " << get_name() << endl;
    strm << BESIndent::LMarg << "utils: " << get_name() << endl;
    BESIndent::Indent();
    _utils->dump(strm);
    BESIndent::UnIndent();
    BESIndent::UnIndent();
}
