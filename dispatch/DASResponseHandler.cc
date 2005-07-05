// DASResponseHandler.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "DASResponseHandler.h"
#include "DAS.h"
#include "cgi_util.h"
#include "DODSParserException.h"
#include "TheRequestHandlerList.h"

DASResponseHandler::DASResponseHandler( string name )
    : DODSResponseHandler( name )
{
}

DASResponseHandler::~DASResponseHandler( )
{
}

void
DASResponseHandler::parse( DODSTokenizer &tokenizer,
                           DODSDataHandlerInterface &dhi )
{
    throw( DODSParserException( (string)"Improper command " + get_name() ) ) ;
}

void
DASResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    _response = new DAS ;
    TheRequestHandlerList->execute_each( dhi ) ;
}

void
DASResponseHandler::transmit( DODSTransmitter *transmitter,
                              DODSDataHandlerInterface &dhi )
{
    if( _response )
    {
	DAS *das = dynamic_cast<DAS *>(_response) ;
	transmitter->send_das( *das, dhi ) ;
    }
}

DODSResponseHandler *
DASResponseHandler::DASResponseBuilder( string handler_name )
{
    return new DASResponseHandler( handler_name ) ;
}

// $Log: DASResponseHandler.cc,v $
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
