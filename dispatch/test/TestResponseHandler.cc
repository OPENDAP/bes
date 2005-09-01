// TestResponseHandler.cc

#include <iostream>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "TestResponseHandler.h"

TestResponseHandler::TestResponseHandler( string name )
    : DODSResponseHandler( name )
{
}

TestResponseHandler::~TestResponseHandler()
{
}

void
TestResponseHandler::parse( DODSTokenizer &tokenizer,
			    DODSDataHandlerInterface &dhi )
{
}

void
TestResponseHandler::execute( DODSDataHandlerInterface & )
{
}

void
TestResponseHandler::transmit( DODSDataHandlerInterface & )
{
}

void
TestResponseHandler::execute_each( DODSDataHandlerInterface & )
{
}

void
TestResponseHandler::execute_all( DODSDataHandlerInterface & )
{
}

void
TestResponseHandler::transmit( DODSTransmitter *transmitter,
			       DODSDataHandlerInterface &dhi )
{
}

DODSResponseHandler *
TestResponseHandler::TestResponseBuilder( string handler_name )
{
    return new TestResponseHandler( handler_name ) ;
}

