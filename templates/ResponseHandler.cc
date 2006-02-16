// OPENDAP_RESPONSEResponseHandler.cc

#include "OPENDAP_RESPONSEResponseHandler.h"
#include "DODSTextInfo.h"

OPENDAP_RESPONSEResponseHandler::OPENDAP_RESPONSEResponseHandler( string name )
    : DODSResponseHandler( name )
{
}

OPENDAP_RESPONSEResponseHandler::~OPENDAP_RESPONSEResponseHandler( )
{
}

void
OPENDAP_RESPONSEResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    // This is an example. Here you would build the DODSResponseObject
    // object and set it to the _response protected data member
    DODSInfo *info = new DODSTextInfo( dhi.transmit_protocol == "HTTP" ) ;
    _response = info ;

    // And here is where your code would code to fill in that response
    // object
}

void
OPENDAP_RESPONSEResponseHandler::transmit( DODSTransmitter *transmitter,
                               DODSDataHandlerInterface &dhi )
{
    // Here is where you would transmit your response object using the
    // specified transmitter object. This is the example using the DODSInfo
    // response object
    if( _response )
    {
	DODSInfo *info = dynamic_cast<DODSInfo *>(_response) ;
	if( dhi.transmit_protocol == "HTTP" )
	    transmitter->send_html( *info, dhi ) ;
	else
	    transmitter->send_text( *info, dhi ) ;
    }
}

DODSResponseHandler *
OPENDAP_RESPONSEResponseHandler::OPENDAP_RESPONSEResponseBuilder( string handler_name )
{
    return new OPENDAP_RESPONSEResponseHandler( handler_name ) ;
}

