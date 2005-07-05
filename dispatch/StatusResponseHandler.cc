// StatusResponseHandler.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "StatusResponseHandler.h"
#include "DODSTextInfo.h"
#include "DODSParserException.h"
#include "DODSStatus.h"
#include "DODSTokenizer.h"

StatusResponseHandler::StatusResponseHandler( string name )
    : DODSResponseHandler( name )
{
}

StatusResponseHandler::~StatusResponseHandler( )
{
}

void
StatusResponseHandler::parse( DODSTokenizer &tokenizer,
                              DODSDataHandlerInterface &dhi )
{
    dhi.action = _response_name ;
    string my_token = tokenizer.get_next_token() ;
    if( my_token != ";" )
    {
	tokenizer.parse_error( my_token + " not expected, expecting a ';'\n" ) ;
    }
}

void
StatusResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    DODSTextInfo *info = new DODSTextInfo( dhi.transmit_protocol == "HTTP" ) ;
    _response = info ;
    DODSStatus s ;
    info->add_data( "Listener boot time: " + s.get_status() ) ;
}

void
StatusResponseHandler::transmit( DODSTransmitter *transmitter,
                                  DODSDataHandlerInterface &dhi )
{
    if( _response )
    {
	DODSTextInfo *info = dynamic_cast<DODSTextInfo *>(_response) ;
	transmitter->send_text( *info, dhi ) ;
    }
}

DODSResponseHandler *
StatusResponseHandler::StatusResponseBuilder( string handler_name )
{
    return new StatusResponseHandler( handler_name ) ;
}

// $Log: StatusResponseHandler.cc,v $
// Revision 1.2  2005/03/15 19:58:35  pwest
// using DODSTokenizer to get first and next tokens
//
// Revision 1.1  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
