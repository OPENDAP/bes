// TestResponseHandler.cc

#include <iostream>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "TestResponseHandler.h"

TestResponseHandler::TestResponseHandler( const string &name )
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
TestResponseHandler::TestResponseBuilder( const string &name )
{
    return new TestResponseHandler( name ) ;
}

