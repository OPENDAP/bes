// DDSResponseHandler.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "DDSResponseHandler.h"
#include "DDS.h"
#include "cgi_util.h"
#include "DODSParserException.h"
#include "TheRequestHandlerList.h"

DDSResponseHandler::DDSResponseHandler( string name )
    : DODSResponseHandler( name )
{
}

DDSResponseHandler::~DDSResponseHandler( )
{
}

void
DDSResponseHandler::parse( DODSTokenizer &tokenizer,
                           DODSDataHandlerInterface &dhi )
{
    throw( DODSParserException( (string)"Improper command " + get_name() ) ) ;
}

void
DDSResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    _response = new DDS( "virtual" ) ;
    TheRequestHandlerList->execute_each( dhi ) ;
}

void
DDSResponseHandler::transmit( DODSTransmitter *transmitter,
                              DODSDataHandlerInterface &dhi )
{
    if( _response )
    {
	DDS *dds = dynamic_cast<DDS *>(_response) ;
	transmitter->send_dds( *dds, dhi ) ;
    }
}

DODSResponseHandler *
DDSResponseHandler::DDSResponseBuilder( string handler_name )
{
    return new DDSResponseHandler( handler_name ) ;
}

// $Log: DDSResponseHandler.cc,v $
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
