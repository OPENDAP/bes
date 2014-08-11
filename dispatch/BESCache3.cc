// BESCache3.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2012 OPeNDAP, Inc
// Author: James Gallagher <jgallagher@opendap.org>
// Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
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

#include "config.h"

#include <string>
#include <sstream>

#include "BESCache3.h"

#include "BESSyntaxUserError.h"
#include "BESInternalError.h"

#include "TheBESKeys.h"
#include "BESDebug.h"
#include "BESLog.h"

using namespace std;

BESCache3 *BESCache3::d_instance = 0;

// The BESCache3 code is a singleton that assumes it's running in the absence of threads but that
// the cache is shared by several processes, each of which have their own instance of BESCache3.
/** Get an instance of the BESCache3 object. This class is a singleton, so the
 * first call to any of three 'get_instance()' methods makes an instance and subsequent call
 * return a pointer to that instance.
 *
 * @param keys The BESKeys object (hold various configuration parameters)
 * @param cache_dir_key Key to use to get the value of the cache directory
 * @param prefix_key Key for the item/file prefix. Each file added to the cache uses this
 * as a prefix so cached items can be easily identified when /tmp is used for the cache.
 * @param size_key How big should the cache be, in megabytes
 * @return A pointer to a BESCache3 object
 */
BESCache3 *
BESCache3::get_instance(BESKeys *keys, const string &cache_dir_key, const string &prefix_key, const string &size_key)
{
    if (d_instance == 0)
        d_instance = new BESCache3(keys, cache_dir_key, prefix_key, size_key);

    return d_instance;
}

/** Get an instance of the BESCache3 object. This version is used for testing; it does
 * not use the BESKeys object but takes values for the parameters directly.
 */
BESCache3 *
BESCache3::get_instance(const string &cache_dir, const string &prefix, unsigned long size)
{
    if (d_instance == 0)
        d_instance = new BESCache3(cache_dir, prefix, size);

    return d_instance;
}

/** Get an instance of the BESCache3 object. This version is used when there's no
 * question that the cache has been instantiated.
 */
BESCache3 *
BESCache3::get_instance()
{
    if (d_instance == 0)
        throw BESInternalError("Tried to get the BESCache3 instance, but it hasn't been created yet", __FILE__, __LINE__);

    return d_instance;
}

/** @brief Private constructor that takes as arguments keys to the cache directory,
 * file prefix, and size of the cache to be looked up a configuration file
 *
 * The keys specified are looked up in the specified keys object. If not
 * found or not set correctly then an exception is thrown. I.E., if the
 * cache directory is empty, the size is zero, or the prefix is empty.
 *
 * @param keys BESKeys object used to look up the keys
 * @param cache_dir_key key to look up in the keys file to find cache dir
 * @param prefix_key key to look up in the keys file to find the cache prefix
 * @param size_key key to look up in the keys file to find the cache size (in MBytes)
 * @throws BESSyntaxUserError if keys not set, cache dir or prefix empty,
 * size is 0, or if cache dir does not exist.
 */
BESCache3::BESCache3(BESKeys *keys, const string &cache_dir_key, const string &prefix_key, const string &size_key)
{
    bool found = false;
    string cache_dir;
    keys->get_value(cache_dir_key, cache_dir, found);
    if (!found)
        throw BESSyntaxUserError("The cache directory key " + cache_dir_key + " was not found in the BES configuration file", __FILE__, __LINE__);

    found = false;
    string prefix;
    keys->get_value(prefix_key, prefix, found);
    if (!found)
        throw BESSyntaxUserError("The prefix key " + prefix_key + " was not found in the BES configuration file", __FILE__, __LINE__);

    found = false;
    string cache_size_str;
    keys->get_value(size_key, cache_size_str, found);
    if (!found)
        throw BESSyntaxUserError("The size key " + size_key + " was not found in the BES configuration file", __FILE__, __LINE__);

    std::istringstream is(cache_size_str);
    unsigned long long max_cache_size_in_bytes;
    is >> max_cache_size_in_bytes;

    initialize(cache_dir, prefix, max_cache_size_in_bytes);
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * this cache.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESCache3::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESCache3::dump - (" << (void *) this << ")" << endl;
}

