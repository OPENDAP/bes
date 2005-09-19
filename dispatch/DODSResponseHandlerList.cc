// DODSResponseHandlerList.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "DODSResponseHandlerList.h"

DODSResponseHandlerList *DODSResponseHandlerList::_instance = 0 ;

/** @brief add a response handler to the list
 *
 * This method actually adds to the list a method that knows how to build a
 * response handler. For each request that comes in, the response name (such
 * as get or set or show) is looked up in this list and the method is used to
 * build a new response handler that knows how to build the response object
 * for the given request.
 *
 * @param handler_name name of the handler to add to the list
 * @param handler_method method that knows how to build the named response
 * handler
 * @return true if successfully added, false if it already exists
 * @see DODSResponseHandler
 * @see DODSResponseObject
 */
bool
DODSResponseHandlerList::add_handler( string handler_name,
			      p_response_handler handler_method )
{
    DODSResponseHandlerList::Handler_citer i ;
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
 * The method that knows how to build the specified response handler is
 * removed from the list.
 *
 * @param handler_name name of the handler build method to remove from the list
 * @return true if successfully removed, false if it doesn't exist in the list
 * @see DODSResponseHandler
 */
bool
DODSResponseHandlerList::remove_handler( string handler_name )
{
    DODSResponseHandlerList::Handler_iter i ;
    i = _handler_list.find( handler_name ) ;
    if( i != _handler_list.end() )
    {
	_handler_list.erase( i ) ;
	return true ;
    }
    return false ;
}

/** @brief returns the response handler with the given name in the list
 *
 * This method looks up the build method with the given name in the list. If
 * it is found then the build method is invoked with the given handler name
 * and the response handler built with the build method is returned. If the
 * handler build method does not exist in the list then NULL is returned.
 *
 * @param handler_name name of the handler to build and return
 * @return a DODSResponseHandler using the specified build method, or NULL if
 * it doesn't exist in the list.
 * @see DODSResponseHandler
 */
DODSResponseHandler *
DODSResponseHandlerList::find_handler( string handler_name )
{
    DODSResponseHandlerList::Handler_citer i ;
    i = _handler_list.find( handler_name ) ;
    if( i != _handler_list.end() )
    {
	p_response_handler p = (*i).second ;
	if( p )
	{
	    return p( handler_name ) ;
	}
    }
    return 0 ;
}

/** @brief returns the list of all response handlers currently registered with
 * this server.
 *
 * Builds a comma separated list of response handlers registered with this
 * server.
 *
 * @return comma separated list of response handler names
 */
string
DODSResponseHandlerList::get_handler_names()
{
    string ret ;
    bool first_name = true ;
    DODSResponseHandlerList::Handler_citer i = _handler_list.begin() ;
    for( ; i != _handler_list.end(); i++ )
    {
	if( !first_name )
	    ret += ", " ;
	ret += (*i).first ;
	first_name = false ;
    }
    return ret ;
}

DODSResponseHandlerList *
DODSResponseHandlerList::TheList()
{
    if( _instance == 0 )
    {
	_instance = new DODSResponseHandlerList ;
    }
    return _instance ;
}

// $Log: DODSResponseHandlerList.cc,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
