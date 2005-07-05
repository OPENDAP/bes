// DefinitionsResponseHandler.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "DefinitionsResponseHandler.h"
#include "DODSTokenizer.h"
#include "DODSTextInfo.h"
#include "TheDefineList.h"

DefinitionsResponseHandler::DefinitionsResponseHandler( string name )
    : DODSResponseHandler( name )
{
}

DefinitionsResponseHandler::~DefinitionsResponseHandler( )
{
}

void
DefinitionsResponseHandler::parse( DODSTokenizer &tokenizer,
                           DODSDataHandlerInterface &dhi )
{
    string my_token = tokenizer.get_next_token() ;
    if( my_token != ";" )
    {
	tokenizer.parse_error( my_token + " not expected" ) ;
    }
}

void
DefinitionsResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    DODSTextInfo *info = new DODSTextInfo( dhi.transmit_protocol == "HTTP" ) ;
    _response = info ;
    TheDefineList->show_definitions( *info ) ;
}

void
DefinitionsResponseHandler::transmit( DODSTransmitter *transmitter,
                               DODSDataHandlerInterface &dhi )
{
}

DODSResponseHandler *
DefinitionsResponseHandler::DefinitionsResponseBuilder( string handler_name )
{
    return new DefinitionsResponseHandler( handler_name ) ;
}

// $Log: DefinitionsResponseHandler.cc,v $
// Revision 1.1  2005/03/15 20:06:20  pwest
// show definitions and show containers response handler
//
