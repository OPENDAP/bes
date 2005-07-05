// ContainersResponseHandler.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "ContainersResponseHandler.h"
#include "DODSTokenizer.h"
#include "DODSTextInfo.h"
#include "ThePersistenceList.h"

ContainersResponseHandler::ContainersResponseHandler( string name )
    : DODSResponseHandler( name )
{
}

ContainersResponseHandler::~ContainersResponseHandler( )
{
}

void
ContainersResponseHandler::parse( DODSTokenizer &tokenizer,
                           DODSDataHandlerInterface &dhi )
{
    string my_token = tokenizer.get_next_token() ;
    if( my_token != ";" )
    {
	tokenizer.parse_error( my_token + " not expected" ) ;
    }
}

void
ContainersResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    DODSTextInfo *info = new DODSTextInfo( dhi.transmit_protocol == "HTTP" ) ;
    _response = info ;
    ThePersistenceList->show_containers( *info ) ;
}

void
ContainersResponseHandler::transmit( DODSTransmitter *transmitter,
                               DODSDataHandlerInterface &dhi )
{
}

DODSResponseHandler *
ContainersResponseHandler::ContainersResponseBuilder( string handler_name )
{
    return new ContainersResponseHandler( handler_name ) ;
}

// $Log: ContainersResponseHandler.cc,v $
// Revision 1.1  2005/03/15 20:06:20  pwest
// show definitions and show containers response handler
//
