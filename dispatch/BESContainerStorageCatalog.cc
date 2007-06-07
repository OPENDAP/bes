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
 
// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include "BESContainerStorageCatalog.h"
#include "BESContainer.h"
#include "BESCatalogUtils.h"
#include "BESContainerStorageException.h"
#include "BESInfo.h"
#include "GNURegex.h"

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
    try
    {
	_utils = BESCatalogUtils::Utils( n ) ;
    }
    catch( BESException &e )
    {
	throw BESContainerStorageException( e.get_message(), e.get_file(), e.get_line() ) ;
    }
    _root_dir = _utils->get_root_dir() ;
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
    // make sure that the real name passed in is not oon the exclude list
    // for the catalog. First, remove any trailing slashes. Then find the
    // basename of the remaining real name. The make sure it's not on the
    // exclude list.
    string::size_type stopat = r_name.length() - 1 ;
    while( r_name[stopat] == '/' )
    {
	stopat-- ;
    }
    string new_name = r_name.substr( 0, stopat + 1 ) ;

    string basename ;
    string::size_type slash = new_name.rfind( "/" ) ;
    if( slash != string::npos )
    {
	basename = new_name.substr( slash+1, new_name.length() - slash ) ;
    }
    else
    {
	basename = new_name ;
    }
    if( !_utils->include( basename ) || _utils->exclude( basename ) )
    {
	string s = "Attempting to create a container with real name "
	           + r_name + " which is on the exclude list" ;
	throw BESContainerStorageException( s, __FILE__, __LINE__ ) ;
    }

    // If the type is specified, then just pass that on. If not, then match
    // it against the types in the type list.
    string new_type = type ;
    if( new_type == "" )
    {
	BESCatalogUtils::match_citer i = _utils->match_list_begin() ;
	BESCatalogUtils::match_citer ie = _utils->match_list_end() ;
	bool done = false ;
	for( ; i != ie && !done; i++ )
	{
	    BESCatalogUtils::type_reg match = (*i) ;
	    // FIXME: Should we create the Regex and put it in the type_reg
	    // structure list instead of compiling it each time? Could this
	    // improve performance? pcw 09/08/06
	    Regex reg_expr( match.reg.c_str() ) ;
	    if( reg_expr.match( r_name.c_str(), r_name.length() ) != -1 )
	    {
		new_type = match.type ;
		done = true ;
	    }
	}
    }
    BESContainerStorageVolatile::add_container( s_name, r_name, new_type ) ;
}

/** @brief is the specified node in question served by a request handler
 *
 * Determine if the node in question is served by a request handler (provides
 * data) and what the request handler serves for the node
 *
 * @param inQuestion node to look up
 * @param provides what is provided for the node by the node types request handler
 * return true if a request hanlder serves the specified node, false otherwise
 */
bool
BESContainerStorageCatalog::isData( const string &inQuestion,
				    list<string> &provides )
{
    string node_type = "" ;
    BESCatalogUtils::match_citer i = _utils->match_list_begin() ;
    BESCatalogUtils::match_citer ie = _utils->match_list_end() ;
    bool done = false ;
    for( ; i != ie && !done; i++ )
    {
	BESCatalogUtils::type_reg match = (*i) ;
	// FIXME: Should we create the Regex and put it in the type_reg
	// structure list instead of compiling it each time? Could this
	// improve performance? pcw 09/08/06
	Regex reg_expr( match.reg.c_str() ) ;
	if( reg_expr.match( inQuestion.c_str(), inQuestion.length() ) != -1 )
	{
	    node_type = match.type ;
	    done = true ;
	}
    }
    // Now that we have the type, go find the request handler and ask what it
    // provides (das, dds, ddx, data, etc...)
    return done ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * the "storage" of containers in a catalog.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESContainerStorageCatalog::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESContainerStorageCatalog::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "name: " << get_name() << endl ;
    strm << BESIndent::LMarg << "utils: " << get_name() << endl ;
    BESIndent::Indent() ;
    _utils->dump( strm ) ;
    BESIndent::UnIndent() ;
    BESIndent::UnIndent() ;
}

