// OPENDAP_RESPONSEResponseHandler.cc

#include "OPENDAP_RESPONSEResponseHandler.h"
#include "BESInfo.h"

OPENDAP_RESPONSEResponseHandler::OPENDAP_RESPONSEResponseHandler( string name )
    : BESResponseHandler( name )
{
}

OPENDAP_RESPONSEResponseHandler::~OPENDAP_RESPONSEResponseHandler( )
{
}

void
OPENDAP_RESPONSEResponseHandler::execute( BESDataHandlerInterface &dhi )
{
    // This is an example. Here you would build the BESResponseObject
    // object and set it to the _response protected data member
    BESInfo *info = new BESInfo( dhi.transmit_protocol == "HTTP" ) ;
    _response = info ;

    // And here is where your code would code to fill in that response
    // object
}

void
OPENDAP_RESPONSEResponseHandler::transmit( BESTransmitter *transmitter,
                               BESDataHandlerInterface &dhi )
{
    // Here is where you would transmit your response object using the
    // specified transmitter object. This is the example using the BESInfo
    // response object
    if( _response )
    {
	BESInfo *info = dynamic_cast<BESInfo *>(_response) ;
	if( dhi.transmit_protocol == "HTTP" )
	    transmitter->send_html( *info, dhi ) ;
	else
	    transmitter->send_text( *info, dhi ) ;
    }
}

BESResponseHandler *
OPENDAP_RESPONSEResponseHandler::OPENDAP_RESPONSEResponseBuilder( string handler_name )
{
    return new OPENDAP_RESPONSEResponseHandler( handler_name ) ;
}

