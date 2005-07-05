// DODSRequestHandler.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "DODSRequestHandler.h"

bool
DODSRequestHandler::add_handler( string handler_name,
			      p_request_handler handler_method )
{
    if( find_handler( handler_name ) == 0 )
    {
	_handler_list[handler_name] = handler_method ;
	return true ;
    }
    return false ;
}

bool
DODSRequestHandler::remove_handler( string handler_name )
{
    DODSRequestHandler::Handler_iter i ;
    i = _handler_list.find( handler_name ) ;
    if( i != _handler_list.end() )
    {
	_handler_list.erase( i ) ;
	return true ;
    }
    return false ;
}

p_request_handler
DODSRequestHandler::find_handler( string handler_name )
{
    DODSRequestHandler::Handler_citer i ;
    i = _handler_list.find( handler_name ) ;
    if( i != _handler_list.end() )
    {
	return (*i).second;
    }
    return 0 ;
}

string
DODSRequestHandler::get_handler_names()
{
    string ret ;
    bool first_name = true ;
    DODSRequestHandler::Handler_citer i = _handler_list.begin() ;
    for( ; i != _handler_list.end(); i++ )
    {
	if( !first_name )
	    ret += ", " ;
	ret += (*i).first ;
	first_name = false ;
    }
    return ret ;
}

// $Log: DODSRequestHandler.cc,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
