// BESAggFactory.cc

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

#include "BESAggFactory.h"

BESAggFactory *BESAggFactory::_instance = 0 ;

/** @brief add an aggregation handler to the list
 *
 * This method actually adds to the list a method that knows how to build an
 * aggregation handler.
 *
 * @param handler_name name of the handler to add to the list
 * @param handler_method method that knows how to build the named agg handler
 * @return true if successfully added, false if it already exists
 * @see BESAggregationServer
 */
bool
BESAggFactory::add_handler( const string &handler_name,
			    p_agg_handler handler_method )
{
    BESAggFactory::Handler_citer i ;
    i = _handler_list.find( handler_name ) ;
    if( i == _handler_list.end() )
    {
	_handler_list[handler_name] = handler_method ;
	return true ;
    }
    return false ;
}

/** @brief removes a response handler from the list
 *
 * The method that knows how to build the specified agg handler is
 * removed from the list.
 *
 * @param handler_name name of the handler build method to remove from the list
 * @return true if successfully removed, false if it doesn't exist in the list
 * @see BESAggregationServer
 */
bool
BESAggFactory::remove_handler( const string &handler_name )
{
    BESAggFactory::Handler_iter i ;
    i = _handler_list.find( handler_name ) ;
    if( i != _handler_list.end() )
    {
	_handler_list.erase( i ) ;
	return true ;
    }
    return false ;
}

/** @brief returns the aggregation handler with the given name in the list
 *
 * This method looks up the build method with the given name in the list. If
 * it is found then the build method is invoked with the given handler name
 * and the agg handler built with the build method is returned. If the
 * handler build method does not exist in the list then NULL is returned.
 *
 * @param handler_name name of the handler to build and return
 * @return a BESAggregationServer using the specified build method, or NULL if
 * it doesn't exist in the list.
 * @see BESAggregationServer
 */
BESAggregationServer *
BESAggFactory::find_handler( const string &handler_name )
{
    BESAggFactory::Handler_citer i ;
    i = _handler_list.find( handler_name ) ;
    if( i != _handler_list.end() )
    {
	p_agg_handler p = (*i).second ;
	if( p )
	{
	    return p( handler_name ) ;
	}
    }
    return 0 ;
}

/** @brief returns the list of all agg handlers currently registered with
 * this server.
 *
 * Builds a comma separated list of agg handlers registered with this
 * server.
 *
 * @return comma separated list of agg handler names
 */
string
BESAggFactory::get_handler_names()
{
    string ret = "";
    bool first_name = true ;
    BESAggFactory::Handler_citer i = _handler_list.begin() ;
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
 * Displays the pointer value of this instance along with the name of all
 * registered aggrecation servers
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESAggFactory::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESAggFactory::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    if( _handler_list.size() )
    {
	strm << BESIndent::LMarg << "registered agg handlers:" << endl ;
	BESIndent::Indent() ;
	BESAggFactory::Handler_citer i = _handler_list.begin() ;
	for( ; i != _handler_list.end(); i++ )
	{
	    strm << BESIndent::LMarg << (*i).first << endl ;
	}
	BESIndent::UnIndent() ;
    }
    else
    {
	strm << BESIndent::LMarg << "registered agg handlers: none" << endl ;
    }
    BESIndent::UnIndent() ;
}

BESAggFactory *
BESAggFactory::TheFactory()
{
    if( _instance == 0 )
    {
	_instance = new BESAggFactory ;
    }
    return _instance ;
}

