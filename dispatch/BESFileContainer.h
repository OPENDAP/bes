// BESFileContainer.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
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
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#ifndef BESFileContainer_h_
#define BESFileContainer_h_ 1

#include <list>
#include <string>

using std::list ;
using std::string ;

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
class BESFileContainer : public BESContainer
{
private:
				BESFileContainer() : BESContainer() {}
protected:
    virtual void		_duplicate( BESContainer &copy_to ) ;
public:
    /** @brief construct a container with the given sumbolic name
     *
     * @param s symbolic name
     */
    				BESFileContainer( const string &sym_name,
				                  const string &real_name,
						  const string &type ) ;

    /** @brief make a copy of the container
     *
     * @param copy_from The container to copy
     */
				BESFileContainer( const BESFileContainer &copy_from ) ;

    virtual			~BESFileContainer() {}

    /** @brief duplicate this instances of BESFileContainer
     */
    virtual BESContainer *	ptr_duplicate( ) ;

    /** @brief returns the name of a file to access for this container,
     * uncompressing if neccessary.
     *
     * @return name of file to access
     */
    virtual string		access() ;

    /** @brief Displays debug information about this object
     *
     * @param strm output stream to use to dump the contents of this object
     */
    virtual void		dump( ostream &strm ) const ;
};

#endif // BESFileContainer_h_

