// TestResponseHandler.cc

#include <iostream>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "TestResponseHandler.h"

TestResponseHandler::TestResponseHandler( string name )
    : BESResponseHandler( name )
{
}

TestResponseHandler::~TestResponseHandler()
{
}

void
TestResponseHandler::execute( BESDataHandlerInterface & )
{
}

void
TestResponseHandler::transmit( BESDataHandlerInterface & )
{
}

void
TestResponseHandler::execute_each( BESDataHandlerInterface & )
{
}

void
TestResponseHandler::execute_all( BESDataHandlerInterface & )
{
}

void
TestResponseHandler::transmit( BESTransmitter *transmitter,
			       BESDataHandlerInterface &dhi )
{
}

BESResponseHandler *
TestResponseHandler::TestResponseBuilder( string handler_name )
{
    return new TestResponseHandler( handler_name ) ;
}

