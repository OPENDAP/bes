// BESContainer.h

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

#ifndef BESContainer_h_
#define BESContainer_h_ 1

#include <list>
#include <string>

using std::list ;
using std::string ;

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
 * source is saved in the BESContainer and is used to execute the request
 * from the client.
 *
 * @see BESContainerStorage
 */
class BESContainer
{
private:
    bool 			_valid ;
    string 			_real_name ;
    string 			_constraint ;
    string 			_symbolic_name ;
    string 			_container_type ;
    string			_attributes ;
    bool			_compressed ;
    bool			_compression_determined ;

    static string		_cacheDir ;
    static list<string>		_compressedExtensions ;
    static string		_script ;
    static string		_cacheSize ;

    void			build_list( const string &ext_list ) ;
public:
    /** @brief construct a container with the given sumbolic name
     *
     * @param s symbolic name
     */
    				BESContainer( const string &s ) ;

    /** @brief make a copy of the container
     *
     * @param copy_from The container to copy
     */
				BESContainer( const BESContainer &copy_from ) ;

    virtual			~BESContainer() {}

    /** @brief set the constraint for this symbolic name during this * execution
     *
     * @param s constraint
     */
    void 			set_constraint( const string &s )
				{
				    _constraint = s ;
				}

    /** @brief set the real name for this symbolic name, such as a file name
     * if reading a data file.
     *
     * @param s real name, such as file name
     */
    void 			set_real_name( const string &s )
				{
				    _real_name = s ;
				}
    /** @brief set the type of data that this symbolic name represents, such
     * as cedar or netcdf.
     *
     * @param s type of data, such as cedar or netcdf
     */
    void 			set_container_type( const string &s )
				{
				    _container_type = s ;
				}

    /** @brief set attributes for this container
     *
     * @param s attributes for this container
     */
    void 			set_attributes( const string &s )
				{
				    _attributes = s ;
				}

    /** @brief set whether this container is valid or not
     *
     * Set to true of the information provided is accurate, or false if
     * there was a problem retrieving the information for this symbolic name
     *
     * @param b true if information valid, false otherwise
     */
    void 			set_valid_flag( bool b )
				{
				    _valid = b ;
				}

    /** @brief retreive the real name for this symbolic name, such as the
     * file name.
     *
     * @return real name, such as file name
     */
    string 			get_real_name() const
				{
				    return _real_name ;
				}
    /** @brief retrieve the constraint for this execution for the symbolic
     * name.
     *
     * @return constraint for this execution for the symbolic name
     */
    string 			get_constraint() const
				{
				    return _constraint ;
				}

    /** @brief retrieve the symbolic name for this container
     *
     * @return symbolic name for this container
     */
    string 			get_symbolic_name() const
				{
				    return _symbolic_name ;
				}

    /** @brief retrieve the type of data this symbolic name is for, such as
     * cedar or netcdf.
     *
     * @return container type for this symbolic name, such as cedar or
     * netcdf
     */
    string 			get_container_type() const
				{
				    return _container_type ;
				}

    /** @brief retrieve the attributes for this container
     *
     * @return attributes for this container
     */
    string 			get_attributes() const
				{
				    return _attributes ;
				}

    /** @brief returns whether the information provided in this container is
     * accurate or not.
     *
     * @return true if information in container is accurate, false otherwise
     */
    bool 			is_valid() const {return _valid;}

    /** @brief returns the name of a file to access for this container,
     * uncompressing if neccessary.
     *
     * @return name of file to access
     */
     virtual string		access() ;
};

#endif // BESContainer_h_

