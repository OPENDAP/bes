// DODSRequesthandlerList.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "DODSRequestHandlerList.h"
#include "DODSRequestHandler.h"
#include "DODSHandlerException.h"

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

DODSRequestHandlerList::Handler_citer
DODSRequestHandlerList::get_first_handler()
{
    return _handler_list.begin() ;
}

DODSRequestHandlerList::Handler_citer
DODSRequestHandlerList::get_last_handler()
{
    return _handler_list.end() ;
}

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
                   if( dhi.real_name_list != "" )
                       dhi.real_name_list += ", " ;
                   dhi.real_name_list += dhi.container->get_real_name() ;
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
