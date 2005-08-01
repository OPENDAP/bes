// OPeNDAPAggFactory.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "OPeNDAPAggFactory.h"

/** @brief add an aggregation handler to the list
 *
 * This method actually adds to the list a method that knows how to build an
 * aggregation handler.
 *
 * @param handler_name name of the handler to add to the list
 * @param handler_method method that knows how to build the named agg handler
 * @return true if successfully added, false if it already exists
 * @see DODSAggregationServer
 */
bool
OPeNDAPAggFactory::add_handler( string handler_name,
			        p_agg_handler handler_method )
{
    OPeNDAPAggFactory::Handler_citer i ;
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
 * @see DODSAggregationServer
 */
bool
OPeNDAPAggFactory::remove_handler( string handler_name )
{
    OPeNDAPAggFactory::Handler_iter i ;
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
 * @return a DODSAggregationServer using the specified build method, or NULL if
 * it doesn't exist in the list.
 * @see DODSAggregationServer
 */
DODSAggregationServer *
OPeNDAPAggFactory::find_handler( string handler_name )
{
    OPeNDAPAggFactory::Handler_citer i ;
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
OPeNDAPAggFactory::get_handler_names()
{
    string ret ;
    bool first_name = true ;
    OPeNDAPAggFactory::Handler_citer i = _handler_list.begin() ;
    for( ; i != _handler_list.end(); i++ )
    {
	if( !first_name )
	    ret += ", " ;
	ret += (*i).first ;
	first_name = false ;
    }
    return ret ;
}

// $Log: OPeNDAPAggFactory.cc,v $
