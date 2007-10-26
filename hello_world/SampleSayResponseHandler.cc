// SampleSayResponseHandler.cc

#include "SampleSayResponseHandler.h"
#include "BESInfoList.h"
#include "BESInfo.h"
#include "SampleResponseNames.h"

SampleSayResponseHandler::SampleSayResponseHandler( const string &name )
    : BESResponseHandler( name )
{
}

SampleSayResponseHandler::~SampleSayResponseHandler( )
{
}

void
SampleSayResponseHandler::execute( BESDataHandlerInterface &dhi )
{
    // This is an example. Here you would build the BESResponseObject
    // object and set it to the _response protected data member
    BESInfo *info = BESInfoList::TheList()->build_info() ;
    _response = info ;

    // Here is where your code would fill in the new response object
    info->begin_response( SAY_RESPONSE ) ;
    string str = dhi.data[ SAY_WHAT ] + " " + dhi.data[ SAY_TO ] ;
    info->add_tag( "text", str ) ;
    info->end_response() ;
}

void
SampleSayResponseHandler::transmit( BESTransmitter *transmitter,
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
SampleSayResponseHandler::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "SampleSayResponseHandler::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESResponseHandler::dump( strm ) ;
    BESIndent::UnIndent() ;
}

BESResponseHandler *
SampleSayResponseHandler::SampleSayResponseBuilder( const string &name )
{
    return new SampleSayResponseHandler( name ) ;
}

