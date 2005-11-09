// DODSRequesthandlerList.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org>
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

#include "DODSRequestHandlerList.h"
#include "DODSRequestHandler.h"
#include "DODSHandlerException.h"
#include "OPeNDAPDataNames.h"

DODSRequestHandlerList *DODSRequestHandlerList::_instance = 0 ;

/** @brief add a request handler to the list of registered handlers for this
 * server
 *
 * @param handler_name name of the data type handled by this request handler
 * @param handler_object the request handler object that knows how to fill in
 * specific response objects
 * @return true if successfully added, false if already exists
 * @see DODSRequestHandler
 */
bool
DODSRequestHandlerList::add_handler( string handler_name,
			      DODSRequestHandler *handler_object )
{
    if( find_handler( handler_name ) == 0 )
    {
	_handler_list[handler_name] = handler_object ;
	return true ;
    }
    return false ;
}

/** @brief remove and return the specified request handler
 *
 * Finds, removes and returns the specified request handler. if the handler
 * exists then it is removed from the list, but not deleted. Deleting the
 * request handler is the responsability of the caller. The request handler is
 * then returned to the caller. If not found, NULL is returned
 *
 * @param handler_name name of the data type request handler to be removed and
 * returned
 * @return returns the request handler if found, NULL otherwise
 * @see DODSRequestHandler
 */
DODSRequestHandler *
DODSRequestHandlerList::remove_handler( string handler_name )
{
    DODSRequestHandler *ret = 0 ;
    DODSRequestHandlerList::Handler_iter i ;
    i = _handler_list.find( handler_name ) ;
    if( i != _handler_list.end() )
    {
	ret = (*i).second;
	_handler_list.erase( i ) ;
    }
    return ret ;
}

/** @brief find and return the specified request handler
 *
 * @param handler_name name of the data type request handler
 * @return the request handler for the specified data type, NULL if not found
 * @see DODSRequestHandler
 */
DODSRequestHandler *
DODSRequestHandlerList::find_handler( string handler_name )
{
    DODSRequestHandlerList::Handler_citer i ;
    i = _handler_list.find( handler_name ) ;
    if( i != _handler_list.end() )
    {
	return (*i).second;
    }
    return 0 ;
}

/** @brief return an iterator pointing to the first request handler in the
 * list
 *
 * @return a constant iterator pointing to the first request handler in the
 * list
 * @see DODSRequestHandler
 */
DODSRequestHandlerList::Handler_citer
DODSRequestHandlerList::get_first_handler()
{
    return _handler_list.begin() ;
}

/** @brief return a constant iterator pointing to the end of the list
 *
 * @return a constant iterator pointing to the end of the list
 * @see DODSRequestHandler
 */
DODSRequestHandlerList::Handler_citer
DODSRequestHandlerList::get_last_handler()
{
    return _handler_list.end() ;
}

/** @brief Returns a comma separated string of request handlers registered
 * with the server
 *
 * @return comma separated string of request handler names registered with the
 * server.
 * @see DODSRequestHandler
 */
string
DODSRequestHandlerList::get_handler_names()
{
    string ret ;
    bool first_name = true ;
    DODSRequestHandlerList::Handler_citer i = _handler_list.begin() ;
    for( ; i != _handler_list.end(); i++ )
    {
	if( !first_name )
	    ret += ", " ;
	ret += (*i).first ;
	first_name = false ;
    }
    return ret ;
}

/** @brief for each container in the given data handler interface, execute the
 * given request
 *
 * For some response objects it is necessary to iterate over all of the
 * containers listed in the specified data handler interface. For each
 * container, get the type of data represented by that container, find the
 * request handler for that data type, find the method within that request
 * handler that knows how to handle the response object to be filled in, and
 * execute that method.
 *
 * @param dhi the data handler interface that contains the list of containers
 * to be iterated over
 * @throws DODSHandlerException if any one of the request handlers does not
 * know how to fill in the specified response object or if any one of the
 * request handlers does not exist.
 * @see _DODSDataHandlerInterface
 * @see DODSContainer
 * @see DODSRequestHandler
 * @see DODSResponseObject
 */
void
DODSRequestHandlerList::execute_each( DODSDataHandlerInterface &dhi )
{
     dhi.first_container() ;
     while( dhi.container )
     {
       if( dhi.container->is_valid() )
       {
           DODSRequestHandler *rh = find_handler( (dhi.container->get_container_type()).c_str() ) ;
           if( rh )
           {
               p_request_handler p = rh->find_handler( dhi.action ) ;
               if( p )
               {
                   p( dhi ) ;
		   string c_list = dhi.data[REAL_NAME_LIST] ;
                   if( c_list != "" )
                       c_list += ", " ;
                   c_list += dhi.container->get_real_name() ;
		   dhi.data[REAL_NAME_LIST] = c_list ;
               } else {
                   DODSHandlerException he ;
                   string se = "Request handler \""
                               + dhi.container->get_container_type()
                               + "\" does not handle the response type \""
                               + dhi.action + "\"" ;
                   he.set_error_description( se ) ;
                   throw he;
               }
           } else {
               DODSHandlerException he ;
               string se = "The request handler \""
                           + dhi.container->get_container_type()
                           + "\" does not exist" ;
               he.set_error_description( se ) ;
               throw he;
           }
       }
       dhi.next_container() ;
     }
}

/** @brief for all of the registered request handlers, execute the given
 * request
 *
 * In some cases, such as a version or help request, it is necessary to
 * iterate over all of the registered request handlers to fill in the response
 * object. If a request handler does not know how to fill in the response
 * object, i.e. doesn't handle the response type, then simply move on to the
 * next. No exception is thrown in this case.
 *
 * @param dhi data handler interface that contains the necessary information
 * to fill in the response object.
 * @see _DODSDataHandlerInterface
 * @see DODSRequestHandler
 * @see DODSResponseObject
 */
void
DODSRequestHandlerList::execute_all( DODSDataHandlerInterface &dhi )
{
    DODSRequestHandlerList::Handler_citer i = get_first_handler() ;
    DODSRequestHandlerList::Handler_citer ie = get_last_handler() ;
    for( ; i != ie; i++ ) 
    {
	DODSRequestHandler *rh = (*i).second ;
	p_request_handler p = rh->find_handler( dhi.action ) ;
	if( p )
	{
	    p( dhi ) ;
	}
    }
}

/** @brief Execute a single method that will fill in the response object
 * rather than iterating over the list of containers or request handlers.
 *
 * This method is for requests of a single type of data. The request is passed
 * off to the request handler for the first container in the data handler
 * interface. It is up to this request handlers method for the specified
 * response object type to fill in the response object. It can iterate over
 * the containers in the data handler interface, for example.
 *
 * @param dhi data handler interface that contains the necessary information
 * to fill in the response object
 * @throws DODSHandlerException if the request handler cannot be found for the
 * first containers data type or if the request handler cannot fill in the
 * specified response object.
 * @see _DODSDataHandlerInterface
 * @see DODSContainer
 * @see DODSResponseObject
 */
void
DODSRequestHandlerList::execute_once( DODSDataHandlerInterface &dhi )
{
    dhi.first_container() ;
    if( dhi.container->is_valid() )
    {
	DODSRequestHandler *rh = find_handler( (dhi.container->get_container_type()).c_str() ) ;
	if( rh )
	{
	    p_request_handler p = rh->find_handler( dhi.action ) ;
	    if( p )
	    {
		p( dhi ) ;
	    } else {
		DODSHandlerException he ;
		string se = "Request handler \""
			    + dhi.container->get_container_type()
			    + "\" does not handle the response type \""
			    + dhi.action + "\"" ;
		he.set_error_description( se ) ;
		throw he;
	    }
	} else {
	    DODSHandlerException he ;
	    string se = "The request handler \""
			+ dhi.container->get_container_type()
			+ "\" does not exist" ;
	    he.set_error_description( se ) ;
	    throw he;
	}
    }
}

DODSRequestHandlerList *
DODSRequestHandlerList::TheList()
{
    if( _instance == 0 )
    {
	_instance = new DODSRequestHandlerList ;
    }
    return _instance ;
}

// $Log: DODSRequestHandlerList.cc,v $
// Revision 1.4  2005/03/15 20:00:14  pwest
// added execute_once so that a single function can execute the request using all the containers instead of executing a function for each container. This is for requests that are handled by the same request type, for example, all containers are of type nc
//
// Revision 1.3  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
