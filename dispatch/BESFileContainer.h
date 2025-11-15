// BESFileContainer.h

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

#ifndef BESFileContainer_h_
#define BESFileContainer_h_ 1

#include <list>
#include <string>

#include "BESContainer.h"

/** @brief Holds real data, container type and constraint for symbolic name
 * read from persistence.
 *
 * A symbolic name is a name that represents a certain set of data, usually
 * a file, and the type of data, such as cedar, netcdf, hdf, etc...
 * Associated with this symbolic name during run time is the constraint
 * associated with the name.
 *
 * The symbolic name is looked up in persistence, such as a MySQL database,
 * a file, or even in memory. The information retrieved from the persistent
 * source is saved in the BESFileContainer and is used to execute the request
 * from the client.
 *
 * @see BESFileContainerStorage
 */
class BESFileContainer: public BESContainer {
private:
    bool _cached;
    std::string _target;

    BESFileContainer() : BESContainer(), _cached(false), _target("")
    {
    }

protected:
    void _duplicate(BESContainer &copy_to) override;

public:
	static const std::string UNCOMPRESS_CACHE_DIR_KEY;
	static const std::string UNCOMPRESS_CACHE_PREFIX_KEY;
	static const std::string UNCOMPRESS_CACHE_SIZE_KEY;

	BESFileContainer(const std::string &sym_name, const std::string &real_name, const std::string &type);

    BESFileContainer(const BESFileContainer &copy_from);

    // TODO Make this destructor call the release() method? jhrg 7/25/18
    ~BESFileContainer() override = default;

    BESContainer * ptr_duplicate() override;

    std::string access() override;

    bool release() override;

    /** @brief Displays debug information about this object
     *
     * @param strm output stream to use to dump the contents of this object
     */
    void dump(std::ostream &strm) const override;
};

#endif // BESFileContainer_h_
