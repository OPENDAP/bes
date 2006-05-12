// ContainerStorageCatalog.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org> and Jose Garcia <jgarcia@ucar.org>
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

#include "ContainerStorageCatalog.h"
#include "DODSContainer.h"
#include "ContainerStorageException.h"
#include "TheDODSKeys.h"
#include "GNURegex.h"
#include "DODSInfo.h"

/** @brief create an instance of this persistent store with the given name.
 *
 * Creates an instances of ContainerStorageCatalog with the given name.
 * Looks up the base directory and regular expressions in the dods
 * initialization file using TheDODSKeys. THrows an exception if either of
 * these cannot be determined or if the regular expressions are incorrectly
 * formed.
 *
 * &lt;data type&gt;:&lt;reg exp&gt;;&lt;data type&gt;:&lt;reg exp&gt;;
 *
 * each type/reg expression pair is separated by a semicolon and ends with a
 * semicolon. The data type/expression pair itself is separated by a
 * semicolon.
 *
 * @param n name of this persistent store
 * @throws ContainerStorageException if unable to find the base
 * directory or regular expressions in the dods initialization file. Also
 * thrown if the type matching expressions are malformed.
 * @see DODSKeys
 * @see DODSContainer
 */
ContainerStorageCatalog::ContainerStorageCatalog( const string &n )
    : ContainerStorageVolatile( n )
{
    string base_key = "Catalog." + n + ".RootDirectory" ;
    bool found = false ;
    _root_dir = TheDODSKeys::TheKeys()->get_key( base_key, found ) ;
    if( _root_dir == "" )
    {
	string s = base_key + " not defined in key file" ;
	ContainerStorageException pe ;
	pe.set_error_description( s ) ;
	throw pe;
    }

    string key = "Catalog." + n + ".TypeMatch" ;
    string curr_str = TheDODSKeys::TheKeys()->get_key( key, found ) ;
    if( curr_str == "" )
    {
	string s = key + " not defined in key file" ;
	ContainerStorageException pe ;
	pe.set_error_description( s ) ;
	throw pe;
    }

    string::size_type str_begin = 0 ;
    string::size_type str_end = curr_str.length() ;
    string::size_type semi = 0 ;
    bool done = false ;
    while( done == false )
    {
	semi = curr_str.find( ";", str_begin ) ;
	if( semi == -1 )
	{
	    string s = (string)"Catalog type match malformed, no semicolon, "
		       "looking for type:regexp;[type:regexp;]" ;
	    ContainerStorageException pe ;
	    pe.set_error_description( s ) ;
	    throw pe;
	}
	else
	{
	    string a_pair = curr_str.substr( str_begin, semi-str_begin ) ;
	    str_begin = semi+1 ;
	    if( semi == str_end-1 )
	    {
		done = true ;
	    }

	    string::size_type col = a_pair.find( ":" ) ;
	    if( col == -1 )
	    {
		string s = (string)"Catalog type match malformed, no colon, "
			   + "looking for type:regexp;[type:regexp;]" ;
		ContainerStorageException pe ;
		pe.set_error_description( s ) ;
		throw pe;
	    }
	    else
	    {
		string name = a_pair.substr( 0, col ) ;
		string val = a_pair.substr( col+1, a_pair.length()-col ) ;
		_match_list[name] = val ;
	    }
	}
    }
}

ContainerStorageCatalog::~ContainerStorageCatalog()
{ 
}

/** @brief adds a container with the provided information
 *
 * If a match is made with the real name passed then the type is set.
 *
 * The real name of the container (the file name) is constructed using the
 * root directory from the initialization file with the passed real name
 * appended to it.
 *
 * The information is then passed to the add_container method in the parent
 * class.
 *
 * @param s_name symbolic name for the container
 * @param r_name real name (full path to the file) for the container
 * @param type type of data represented by this container
 */
void
ContainerStorageCatalog::add_container( const string &s_name,
					const string &r_name,
					const string &type )
{
    string new_r_name = _root_dir + "/" + r_name ;
    string new_type = type ;
    if( new_type == "" )
    {
	ContainerStorageCatalog::Match_list_citer i = _match_list.begin() ;
	ContainerStorageCatalog::Match_list_citer ie = _match_list.end() ;
	for( ; i != ie; i++ )
	{
	    string reg = (*i).second ;
	    Regex reg_expr( reg.c_str() ) ;
	    if( reg_expr.match( r_name.c_str(), r_name.length() ) != -1 )
	    {
		new_type = (*i).first ;
	    }
	}
    }
    ContainerStorageVolatile::add_container( s_name, new_r_name, new_type ) ;
}

// $Log: ContainerStorageCatalog.cc,v $
// Revision 1.8  2005/03/17 20:37:50  pwest
// added documentation for rem_container and show_containers
//
// Revision 1.7  2005/03/17 19:23:58  pwest
// deleting the container in rem_container instead of returning the removed container, returning true if successfully removed and false otherwise
//
// Revision 1.6  2005/03/15 19:55:36  pwest
// show containers and show definitions
//
// Revision 1.5  2005/02/02 00:03:13  pwest
// ability to replace containers and definitions
//
// Revision 1.4  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
// Revision 1.3  2004/12/15 17:36:01  pwest
//
// Changed the way in which the parser retrieves container information, going
// instead to ThePersistenceList, which goes through the list of container
// persistence instances it has.
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
