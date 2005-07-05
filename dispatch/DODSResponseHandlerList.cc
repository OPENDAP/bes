// DODSResponseHandlerList.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "DODSResponseHandlerList.h"

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

// $Log: DODSResponseHandlerList.cc,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
