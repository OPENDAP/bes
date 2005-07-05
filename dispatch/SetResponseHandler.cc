// SetResponseHandler.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "SetResponseHandler.h"
#include "DODSTextInfo.h"
#include "DODSParserException.h"
#include "DODSTokenizer.h"
#include "ThePersistenceList.h"
#include "DODSContainerPersistence.h"
#include "DODSContainerPersistenceException.h"

SetResponseHandler::SetResponseHandler( string name )
    : DODSResponseHandler( name )
{
}

SetResponseHandler::~SetResponseHandler( )
{
}

void
SetResponseHandler::parse( DODSTokenizer &tokenizer,
                           DODSDataHandlerInterface &dhi )
{
    dhi.action = _response_name ;
    string my_token = tokenizer.get_next_token() ;
    if( my_token == "container" )
    {
	_persistence = PERSISTENCE_VOLATILE ;
	my_token = tokenizer.get_next_token() ;
	if( my_token != "values" )
	{
	    if( my_token == "in" )
	    {
		_persistence = tokenizer.get_next_token() ;
	    }
	    else
	    {
		tokenizer.parse_error( my_token + " not expected\n" ) ;
	    }
	    my_token = tokenizer.get_next_token() ;
	}

	if( my_token == "values" )
	{
	    _symbolic_name = tokenizer.get_next_token() ;
	    my_token = tokenizer.get_next_token() ;
	    if( my_token == "," )
	    {
		_real_name = tokenizer.get_next_token() ; 
		my_token = tokenizer.get_next_token() ;
		if( my_token == "," )
		{
		    _type = tokenizer.get_next_token() ;
		    my_token = tokenizer.get_next_token() ;
		    if( my_token != ";" )
		    {
			tokenizer.parse_error( my_token + " not expected\n" ) ;
		    }
		}
		else
		{
		    tokenizer.parse_error( my_token + " not expected\n" ) ;
		}
	    }
	    else
	    {
		tokenizer.parse_error( my_token + " not expected\n" ) ;
	    }
	}
	else
	{
	    tokenizer.parse_error( my_token + " not expected\n" ) ;
	}
    }
    else
    {
	tokenizer.parse_error( my_token + " not expected\n" ) ;
    }
}

void
SetResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    DODSTextInfo *info = new DODSTextInfo( dhi.transmit_protocol == "HTTP" ) ;
    _response = info ;

    DODSContainerPersistence *cp = ThePersistenceList->find_persistence( _persistence ) ;
    if( cp )
    {
	try
	{
	    string def_type = "added" ;
	    bool deleted = cp->rem_container( _symbolic_name ) ;
	    if( deleted == true )
	    {
		def_type = "replaced" ;
	    }
	    cp->add_container( _symbolic_name, _real_name, _type ) ;
	    string ret = (string)"Successfully " + def_type + " container "
			 + _symbolic_name + " to persistent store "
			 + _persistence + "\n" ;
	    info->add_data( ret ) ;
	}
	catch( DODSContainerPersistenceException &e )
	{
	    string ret = (string)"Unable to add container "
			 + _symbolic_name + " to persistent store "
			 + _persistence + "\n" ;
	    info->add_data( ret ) ;
	    info->add_data( e.get_error_description() ) ;
	}
    }
    else
    {
	string ret = (string)"Unable to add container "
		     + _symbolic_name + " to persistent store "
		     + _persistence + "\n" ;
	info->add_data( ret ) ;
	info->add_data( (string)"Persistence store " + _persistence + " does not exist" ) ;
    }
}

void
SetResponseHandler::transmit( DODSTransmitter *transmitter,
                                  DODSDataHandlerInterface &dhi )
{
    if( _response )
    {
	DODSTextInfo *info = dynamic_cast<DODSTextInfo *>(_response) ;
	transmitter->send_text( *info, dhi ) ;
    }
}

DODSResponseHandler *
SetResponseHandler::SetResponseBuilder( string handler_name )
{
    return new SetResponseHandler( handler_name ) ;
}

// $Log: SetResponseHandler.cc,v $
// Revision 1.5  2005/03/17 19:23:58  pwest
// deleting the container in rem_container instead of returning the removed container, returning true if successfully removed and false otherwise
//
// Revision 1.4  2005/03/15 19:58:35  pwest
// using DODSTokenizer to get first and next tokens
//
// Revision 1.3  2005/02/10 18:48:23  pwest
// missing escaped quote in response text
//
// Revision 1.2  2005/02/02 00:03:13  pwest
// ability to replace containers and definitions
//
// Revision 1.1  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
