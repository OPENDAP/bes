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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include "BESFileContainer.h"
#include "TheBESKeys.h"

// Use the new caching code?
#define NEW_CACHE 1

#if NEW_CACHE
// New cache system
#include "BESUncompressManager3.h"
#include "BESCache3.h"
#else
// Old cache system
#include "BESUncompressManager.h"
#include "BESCache.h"
#endif
#include "BESForbiddenError.h"

/** @brief construct a container representing a file
 *
 * @param sym_name symbolic name of the container
 * @param real_name real name of the container, a file name in this case
 * @param type type of the data represented in the file
 */
BESFileContainer::BESFileContainer(const string &sym_name, const string &real_name, const string &type) :
        BESContainer(sym_name, real_name, type)
{
    string::size_type dotdot = real_name.find("..");
    if (dotdot != string::npos) {
        string s = (string) "'../' not allowed in container real name " + real_name;
        throw BESForbiddenError(s, __FILE__, __LINE__);
    }
}

/** @brief make a copy of the container
 *
 * @param copy_from The container to copy
 */
BESFileContainer::BESFileContainer(const BESFileContainer &copy_from) :
        BESContainer(copy_from)
{
}

void BESFileContainer::_duplicate(BESContainer &copy_to)
{
    BESContainer::_duplicate(copy_to);
}

/** @brief duplicate this instances of BESFileContainer
 *
 * @return a copy of this instance
 */
BESContainer *
BESFileContainer::ptr_duplicate()
{
    BESContainer *container = new BESFileContainer;
    BESContainer::_duplicate(*container);
    return container;
}

/** @brief returns the name of a file to access for this container,
 * uncompressing if necessary.
 *
 * @return name of file to access
 */
string BESFileContainer::access()
{
#if NEW_CACHE
    // Get a pointer to the singleton cache instance for this process.
    BESCache3 *cache = BESCache3::get_instance(TheBESKeys::TheKeys(), (string) "BES.CacheDir",
            (string) "BES.CachePrefix", (string) "BES.CacheSize");

    // If the file is in the cache, this is nearly a no-op; if the file is compressed,
    // decompress it, add it to the class and return the name of the file in the cache.
    // In both of those cases, the file is cached, so we need to record that so that
    // the release() method will remove the lock on the cached file. If the file is not
    // a compressed file, the 'uncompress' function returns false and the contents of
    // the value-result parameter '_target' is undefined.
    _cached = BESUncompressManager3::TheManager()->uncompress(get_real_name(), _target, cache);
    if (_cached)
        return _target;
    else
        return get_real_name();

#else

    // This is easy ... create the cache using the different keys
    BESKeys *keys = TheBESKeys::TheKeys();
    BESCache cache( *keys, "BES.CacheDir", "BES.CachePrefix", "BES.CacheSize" );

    _cached = BESUncompressManager::TheManager()->uncompress( get_real_name(), _target, cache );
    if( _cached )
    return _target;

    return get_real_name();
#endif
}

/** @brief release the file
 *
 * If the file was cached (uncompressed) then we need to release the lock on
 * the cached entry
 *
 * @return if successfully released, returns true, otherwise returns false
 */
bool BESFileContainer::release()
{
#if NEW_CACHE
    if (_cached)
        BESCache3::get_instance()->unlock_and_close(_target);
#endif
    return true;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * this container.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESFileContainer::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESFileContainer::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESContainer::dump(strm);
    BESIndent::UnIndent();
}

