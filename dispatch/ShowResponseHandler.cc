// ShowResponseHandler.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "ShowResponseHandler.h"
#include "DODSParser.h"
#include "TheResponseHandlerList.h"
#include "DODSResponseObject.h"
#include "DODSParserException.h"
#include "DODSTokenizer.h"

ShowResponseHandler::ShowResponseHandler( string name )
    : DODSResponseHandler( name ),
      _sub_response( 0 )
{
}

ShowResponseHandler::~ShowResponseHandler( )
{
    if( _sub_response )
    {
	delete _sub_response ;
    }
    _sub_response = 0 ;
}

DODSResponseObject *
ShowResponseHandler::get_response_object()
{
    if( _sub_response ) return _sub_response->get_response_object() ;
    return DODSResponseHandler::get_response_object() ;
}

void
ShowResponseHandler::parse( DODSTokenizer &tokenizer,
                           DODSDataHandlerInterface &dhi )
{
    string my_token = tokenizer.get_next_token() ;

    dhi.action = my_token ;
    _sub_response = TheResponseHandlerList->find_handler( my_token ) ;
    if( !_sub_response )
    {
	throw DODSParserException( (string)"Improper command show " + my_token );
    }

    _sub_response->parse( tokenizer, dhi ) ;
}

void
ShowResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    _sub_response->execute( dhi ) ;
}

void
ShowResponseHandler::transmit( DODSTransmitter *transmitter,
			      DODSDataHandlerInterface &dhi )
{
    _sub_response->transmit( transmitter, dhi ) ;
}

void
ShowResponseHandler::set_response_object( DODSResponseObject *new_response )
{
    if( _sub_response ) _sub_response->set_response_object( new_response ) ;
}

DODSResponseHandler *
ShowResponseHandler::ShowResponseBuilder( string handler_name )
{
    return new ShowResponseHandler( handler_name ) ;
}

// $Log: ShowResponseHandler.cc,v $
// Revision 1.3  2005/03/26 00:33:33  pwest
// fixed aggregation server invoke problems. Other commands use the aggregation command but no aggregation is needed, so set aggregation to empty string when done
//
// Revision 1.2  2005/03/15 19:55:36  pwest
// show containers and show definitions
//
// Revision 1.1  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
