// BESFileContainer.cc

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

#include "BESFileContainer.h"
#include "TheBESKeys.h"

// New cache system
#include "BESUncompressCache.h"
#include "BESUncompressManager3.h"

#include "BESForbiddenError.h"

#include "BESDebug.h"

using std::endl;
using std::ostream;
using std::string;

#define MODULE "cache2"
#define prolog std::string("BESFileContainer::").append(__func__).append("() - ")

/** @brief construct a container representing a file
 *
 * @param sym_name symbolic name of the container
 * @param real_name real name of the container, a file name in this case
 * @param type type of the data represented in the file
 */
BESFileContainer::BESFileContainer(const string &sym_name, const string &real_name, const string &type)
    : BESContainer(sym_name, real_name, type), _cached(false), _target("") {
    string::size_type dotdot = real_name.find("..");
    if (dotdot != string::npos) {
        string s = prolog + "'../' not allowed in container real name " + real_name;
        throw BESForbiddenError(s, __FILE__, __LINE__);
    }
}

/** @brief make a copy of the container
 *
 * @param copy_from The container to copy
 */
BESFileContainer::BESFileContainer(const BESFileContainer &copy_from)
     = default;

void BESFileContainer::_duplicate(BESContainer &copy_to) { BESContainer::_duplicate(copy_to); }

/** @brief duplicate this instances of BESFileContainer
 *
 * @return a copy of this instance
 */
BESContainer *BESFileContainer::ptr_duplicate() {
    BESContainer *container = new BESFileContainer;
    BESContainer::_duplicate(*container);
    return container;
}

/** @brief returns the name of a file to access for this container,
 * uncompressing if necessary.
 *
 * @return name of file to access
 */
string BESFileContainer::access() {
    BESDEBUG(MODULE, prolog << "BEGIN real_name: " << get_real_name() << endl);

    // Get a pointer to the singleton cache instance for this process.
    BESUncompressCache *cache = BESUncompressCache::get_instance();

    // If the file is in the cache, this is nearly a no-op; if the file is compressed,
    // uncompress it, add it to the class and return the name of the file in the cache.
    // In both of those cases, the file is cached, so we need to record that so that
    // the release() method will remove the lock on the cached file. If the file is not
    // a compressed file, the 'uncompress' function returns false and the contents of
    // the value-result parameter '_target' is undefined.
    _cached = BESUncompressManager3::TheManager()->uncompress(get_real_name(), _target, cache);
    if (_cached) {
        BESDEBUG(MODULE, prolog << "END Cached as: " << _target << endl);
        return _target;
    } else {
        BESDEBUG(MODULE, prolog << "END Not cached" << endl);
        return get_real_name();
    }
}

/** @brief release the file
 *
 * If the file was cached (uncompressed) then we need to release the lock on
 * the cached entry
 *
 * @todo Call this in the dtor? jhrg 7/25/18
 * @return if successfully released, returns true, otherwise returns false
 */
bool BESFileContainer::release() {
    BESDEBUG(MODULE, prolog << "_cached: " << _cached << ", _target: " << _target << endl);
    if (_cached)
        BESUncompressCache::get_instance()->unlock_and_close(_target);

    return true;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * this container.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESFileContainer::dump(ostream &strm) const {
    strm << BESIndent::LMarg << prolog << "(" << (void *)this << ")" << endl;
    BESIndent::Indent();
    BESContainer::dump(strm);
    BESIndent::UnIndent();
}
