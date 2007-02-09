// SayResponseHandler.cc

#include "SayResponseHandler.h"
#include "BESInfoList.h"
#include "BESInfo.h"
#include "SayResponseNames.h"

SayResponseHandler::SayResponseHandler( string name )
    : BESResponseHandler( name )
{
}

SayResponseHandler::~SayResponseHandler( )
{
}

void
SayResponseHandler::execute( BESDataHandlerInterface &dhi )
{
    // This is an example. Here you would build the BESResponseObject
    // object and set it to the _response protected data member
    BESInfo *info = BESInfoList::TheList()->build_info() ;
    _response = info ;

    // Here is where your code would fill in the new response object
    info->begin_response( say_RESPONSE ) ;
    string str = dhi.data[ SAY_WHAT ] + " " + dhi.data[ SAY_TO ] ;
    info->add_tag( "text", str ) ;
    info->end_response() ;
}

void
SayResponseHandler::transmit( BESTransmitter *transmitter,
                               BESDataHandlerInterface &dhi )
{
    // Here is where you would transmit your response object using the
    // specified transmitter object. This is the example using the BESInfo
    // response object
    if( _response )
    {
	BESInfo *info = dynamic_cast<BESInfo *>( _response ) ;
	info->transmit( transmitter, dhi ) ;
    }
}

void
SayResponseHandler::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "SayResponseHandler::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESResponseHandler::dump( strm ) ;
    BESIndent::UnIndent() ;
}

BESResponseHandler *
SayResponseHandler::SayResponseBuilder( string handler_name )
{
    return new SayResponseHandler( handler_name ) ;
}

