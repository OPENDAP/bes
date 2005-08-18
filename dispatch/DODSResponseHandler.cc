// DODSResponseHandler.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "DODSResponseHandler.h"
#include "TheRequestHandlerList.h"
#include "DODSResponseObject.h"
#include "DODSRequestHandler.h"
#include "DODSHandlerException.h"

DODSResponseHandler::DODSResponseHandler( string name )
    : _response_name( name ),
      _response( 0 )
{
}

DODSResponseHandler::~DODSResponseHandler( )
{
    if( _response )
    {
	delete _response ;
    }
    _response = 0 ;
}

DODSResponseObject *
DODSResponseHandler::get_response_object()
{
    return _response ;
}

void
DODSResponseHandler::set_response_object( DODSResponseObject *new_response )
{
    _response = new_response ;
}

// $Log: DODSResponseHandler.cc,v $
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
