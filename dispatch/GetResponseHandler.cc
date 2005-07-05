// GetResponseHandler.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "GetResponseHandler.h"
#include "DODSParser.h"
#include "TheResponseHandlerList.h"
#include "TheRequestHandlerList.h"
#include "DODSResponseObject.h"
#include "DODSRequestHandler.h"
#include "DODSHandlerException.h"
#include "DODSParserException.h"
#include "ThePersistenceList.h"
#include "DODSTokenizer.h"
#include "TheDefineList.h"
#include "DODSDefine.h"

GetResponseHandler::GetResponseHandler( string name )
    : DODSResponseHandler( name ),
      _sub_response( 0 )
{
}

GetResponseHandler::~GetResponseHandler( )
{
    if( _sub_response ) delete _sub_response ;
    _sub_response = 0 ;
}

DODSResponseObject *
GetResponseHandler::get_response_object()
{
    if( _sub_response ) return _sub_response->get_response_object() ;
    return DODSResponseHandler::get_response_object() ;
}

void
GetResponseHandler::parse( DODSTokenizer &tokenizer,
                           DODSDataHandlerInterface &dhi )
{
    string def_name ;

    string my_token = tokenizer.get_next_token() ;
    dhi.action = my_token ;
    _sub_response = TheResponseHandlerList->find_handler( my_token ) ;
    if( !_sub_response )
    {
	throw DODSParserException( (string)"Improper command get " + my_token );
    }

    my_token = tokenizer.get_next_token() ;
    if( my_token != "for" )
    {
	tokenizer.parse_error( my_token + " not expected\n" ) ;
    }
    else
    {
	def_name = tokenizer.get_next_token() ;

	my_token = tokenizer.get_next_token() ;
	if( my_token == "return" )
	{
	    my_token = tokenizer.get_next_token() ;
	    if( my_token != "as" )
	    {
		tokenizer.parse_error( my_token + " not expected, expecting \"as\"" ) ;
	    }
	    else
	    {
		my_token = tokenizer.get_next_token() ;
		dhi.return_command = tokenizer.remove_quotes( my_token ) ;

		my_token = tokenizer.get_next_token() ;
		if( my_token != ";" )
		{
		    tokenizer.parse_error( my_token + " not expected, expecting ';'" ) ;
		}
	    }
	}
	else
	{
	    if( my_token != ";" )
	    {
		tokenizer.parse_error( my_token + " not expected, expecting \"return\" or ';'" ) ;
	    }
	}
    }

    DODSDefine *d = TheDefineList->find_def( def_name ) ;
    if( !d )
    {
	throw DODSParserException( (string)"Unable to find definition " + def_name ) ;
    }

    DODSDefine::containers_iterator i = d->first_container() ;
    DODSDefine::containers_iterator ie = d->end_container() ;
    while( i != ie )
    {
	dhi.containers.push_back( *i ) ;
	i++ ;
    }
    dhi.aggregation_command = d->aggregation_command ;
}

void
GetResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    if( _sub_response ) _sub_response->execute( dhi ) ;
}

void
GetResponseHandler::transmit( DODSTransmitter *transmitter,
			      DODSDataHandlerInterface &dhi )
{
    if( _sub_response ) _sub_response->transmit( transmitter, dhi ) ;
}

void
GetResponseHandler::set_response_object( DODSResponseObject *new_response )
{
    if( _sub_response ) _sub_response->set_response_object( new_response ) ;
}

DODSResponseHandler *
GetResponseHandler::GetResponseBuilder( string handler_name )
{
    return new GetResponseHandler( handler_name ) ;
}

// $Log: GetResponseHandler.cc,v $
// Revision 1.4  2005/03/26 00:33:33  pwest
// fixed aggregation server invoke problems. Other commands use the aggregation command but no aggregation is needed, so set aggregation to empty string when done
//
// Revision 1.3  2005/03/15 19:58:35  pwest
// using DODSTokenizer to get first and next tokens
//
// Revision 1.2  2005/02/10 18:51:23  pwest
// was not getting next token adter as in return as segment
//
// Revision 1.1  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
