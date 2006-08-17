// BESContainerStorageCatalog.cc

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

#include "BESContainerStorageCatalog.h"
#include "BESContainer.h"
#include "BESContainerStorageException.h"
#include "TheBESKeys.h"
#include "GNURegex.h"
#include "BESInfo.h"

/** @brief create an instance of this persistent store with the given name.
 *
 * Creates an instances of BESContainerStorageCatalog with the given name.
 * Looks up the base directory and regular expressions in the dods
 * initialization file using TheBESKeys. THrows an exception if either of
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
 * @throws BESContainerStorageException if unable to find the base
 * directory or regular expressions in the dods initialization file. Also
 * thrown if the type matching expressions are malformed.
 * @see BESKeys
 * @see BESContainer
 */
BESContainerStorageCatalog::BESContainerStorageCatalog( const string &n )
    : BESContainerStorageVolatile( n )
{
    string base_key = "BES.Catalog." + n + ".RootDirectory" ;
    bool found = false ;
    _root_dir = TheBESKeys::TheKeys()->get_key( base_key, found ) ;
    if( _root_dir == "" )
    {
	string s = base_key + " not defined in key file" ;
	throw BESContainerStorageException( s, __FILE__, __LINE__ ) ;
    }

    string key = "BES.Catalog." + n + ".TypeMatch" ;
    string curr_str = TheBESKeys::TheKeys()->get_key( key, found ) ;
    if( curr_str == "" )
    {
	string s = key + " not defined in key file" ;
	throw BESContainerStorageException( s, __FILE__, __LINE__ ) ;
    }

    string::size_type str_begin = 0 ;
    string::size_type str_end = curr_str.length() ;
    string::size_type semi = 0 ;
    bool done = false ;
    while( done == false )
    {
	semi = curr_str.find( ";", str_begin ) ;
	if( semi == string::npos )
	{
	    string s = (string)"Catalog type match malformed, no semicolon, "
		       "looking for type:regexp;[type:regexp;]" ;
	    throw BESContainerStorageException( s, __FILE__, __LINE__ ) ;
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
	    if( col == string::npos )
	    {
		string s = (string)"Catalog type match malformed, no colon, "
			   + "looking for type:regexp;[type:regexp;]" ;
		throw BESContainerStorageException( s, __FILE__, __LINE__ ) ;
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

BESContainerStorageCatalog::~BESContainerStorageCatalog()
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
BESContainerStorageCatalog::add_container( const string &s_name,
					   const string &r_name,
					   const string &type )
{
    string new_r_name = _root_dir + "/" + r_name ;
    string new_type = type ;
    if( new_type == "" )
    {
	BESContainerStorageCatalog::Match_list_citer i = _match_list.begin() ;
	BESContainerStorageCatalog::Match_list_citer ie = _match_list.end() ;
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
    BESContainerStorageVolatile::add_container( s_name, new_r_name, new_type ) ;
}

