// BESRequestHandler.cc

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

#include "BESRequestHandler.h"

/** @brief add a handler method to the request handler that knows how to fill
 * in a specific response object
 *
 * Add a handler method for a specific response object to the request handler.
 * The handler method takes a reference to a BESDataHandlerInterface and
 * returns bool, true if the response object is filled in successfully by the
 * method, false otherwise.
 *
 * @param handler_name name of the response object this method can fill in
 * @param handler_method a function pointer to the method that can fill in the
 * specified response object
 * @return true if the handler is added, false if it already exists
 * @see BESResponseObject
 * @see BESResponseNames
 */
bool
BESRequestHandler::add_handler( const string &handler_name,
			        p_request_handler handler_method )
{
    if( find_handler( handler_name ) == 0 )
    {
	_handler_list[handler_name] = handler_method ;
	return true ;
    }
    return false ;
}

/** @brief remove the specified handler method from this request handler
 *
 * @param handler_name name of the method to be removed, same as the name of
 * the response object
 * @return true if successfully removed, false if not found
 * @see BESResponseNames
 */
bool
BESRequestHandler::remove_handler( const string &handler_name )
{
    BESRequestHandler::Handler_iter i ;
    i = _handler_list.find( handler_name ) ;
    if( i != _handler_list.end() )
    {
	_handler_list.erase( i ) ;
	return true ;
    }
    return false ;
}

/** @brief find the method that can handle the specified response object type
 *
 * Find the method that can handle the specified response object type. The
 * response object type is the same as the handler name.
 *
 * @param handler_name name of the method that can fill in the response object type 
 * @return the method that can fill in the specified response object type
 * @see BESResponseObject
 * @see BESResponseNames
 */
p_request_handler
BESRequestHandler::find_handler( const string &handler_name )
{
    BESRequestHandler::Handler_citer i ;
    i = _handler_list.find( handler_name ) ;
    if( i != _handler_list.end() )
    {
	return (*i).second;
    }
    return 0 ;
}

/** @brief return a comma separated list of response object types handled by
 * this request handler
 *
 * @return the comma separated list of response object types
 * @see BESResponseObject
 * @see BESResponseNames
 */
string
BESRequestHandler::get_handler_names()
{
    string ret ;
    bool first_name = true ;
    BESRequestHandler::Handler_citer i = _handler_list.begin() ;
    for( ; i != _handler_list.end(); i++ )
    {
	if( !first_name )
	    ret += ", " ;
	ret += (*i).first ;
	first_name = false ;
    }
    return ret ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance, the name of the request
 * handler, and the names of all registered handler functions
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESRequestHandler::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESRequestHandler::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "name: " << _name << endl ;
    if( _handler_list.size() )
    {
	strm << BESIndent::LMarg << "registered handler functions:" << endl ;
	BESIndent::Indent() ;
	BESRequestHandler::Handler_citer i = _handler_list.begin() ;
	BESRequestHandler::Handler_citer ie = _handler_list.end() ;
	for( ; i != ie; i++ )
	{
	    strm << BESIndent::LMarg << (*i).first << endl ;
	}
	BESIndent::UnIndent() ;
    }
    else
    {
	strm << BESIndent::LMarg << "registered handler functions: none" << endl ;
    }
    BESIndent::UnIndent() ;
}

