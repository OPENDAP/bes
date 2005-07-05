// DeleteResponseHandler.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "DeleteResponseHandler.h"
#include "DODSTokenizer.h"
#include "DODSTextInfo.h"
#include "TheDefineList.h"
#include "DODSDefine.h"
#include "ThePersistenceList.h"
#include "DODSContainerPersistence.h"
#include "DODSContainer.h"

DeleteResponseHandler::DeleteResponseHandler( string name )
    : DODSResponseHandler( name ),
      _def_name( "" ),
      _store_name( "" ),
      _container_name( "" ),
      _definitions( false )
{
}

DeleteResponseHandler::~DeleteResponseHandler( )
{
}

void
DeleteResponseHandler::parse( DODSTokenizer &tokenizer,
                           DODSDataHandlerInterface &dhi )
{
    string my_token = tokenizer.get_next_token() ;
    if( my_token == "container" )
    {
	_container_name = tokenizer.get_next_token() ;
	if( _container_name == ";" )
	{
	    tokenizer.parse_error( my_token + " not expected, expecting container name" ) ;
	}
	_store_name = PERSISTENCE_VOLATILE ;
	my_token = tokenizer.get_next_token() ;
	if( my_token == "from" )
	{
	    _store_name = tokenizer.get_next_token() ;
	    if( _store_name == ";" )
	    {
		tokenizer.parse_error( my_token + " not expected, expecting persistent name" ) ;
	    }
	    my_token = tokenizer.get_next_token() ;
	}
    }
    else if( my_token == "definition" )
    {
	_def_name = tokenizer.get_next_token() ;
	if( _def_name == ";" )
	{
	    tokenizer.parse_error( my_token + " not expected, expecting definition name" ) ;
	}
	my_token = tokenizer.get_next_token() ;
    }
    else if( my_token == "definitions" )
    {
	_definitions = true ;
	my_token = tokenizer.get_next_token() ;
    }
    if( my_token != ";" )
    {
	tokenizer.parse_error( my_token + " not expected, expecting semicolon (;)" ) ;
    }
}

void
DeleteResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    DODSTextInfo *info = new DODSTextInfo( dhi.transmit_protocol == "HTTP" ) ;
    _response = info ;
    if( _definitions )
    {
	TheDefineList->remove_defs() ;
	info->add_data( "Successfully deleted all definitions\n" ) ;
    }
    else if( _def_name != "" )
    {
	bool deleted = TheDefineList->remove_def( _def_name ) ;
	if( deleted == true )
	{
	    string line = (string)"Successfully deleted definition \""
	                  + _def_name
			  + "\"\n" ;
	    info->add_data( line ) ;
	}
	else
	{
	    string line = (string)"Definition \""
	                  + _def_name
			  + "\" does not exist.  Unable to delete.\n" ;
	    info->add_data( line ) ;
	}
    }
    else if( _store_name != "" && _container_name != "" )
    {
	DODSContainerPersistence *cp =
		ThePersistenceList->find_persistence( _store_name ) ;
	if( cp )
	{
	    bool deleted =  cp->rem_container( _container_name ) ;
	    if( deleted == true )
	    {
		string line = (string)"Successfully deleted container \""
		              + _container_name
			      + "\" from persistent store \""
			      + _store_name
			      + "\"\n" ;
		info->add_data( line ) ;
	    }
	    else
	    {
		string line = (string)"Unable to delete container. "
		              + "The container \""
		              + _container_name
			      + "\" does not exist in the persistent store \""
			      + _store_name
			      + "\"\n" ;
		info->add_data( line ) ;
	    }
	}
	else
	{
	    string line = (string)"The persistence store \""
	                  + _store_name
			  + "\" does not exist. "
			  + "Unable to delete container \""
			  + _container_name
			  + "\".\n" ;
	    info->add_data( line ) ;
	}
    }
}

void
DeleteResponseHandler::transmit( DODSTransmitter *transmitter,
                               DODSDataHandlerInterface &dhi )
{
}

DODSResponseHandler *
DeleteResponseHandler::DeleteResponseBuilder( string handler_name )
{
    return new DeleteResponseHandler( handler_name ) ;
}

// $Log: DeleteResponseHandler.cc,v $
// Revision 1.1  2005/03/17 19:26:22  pwest
// added delete command to delete a specific container, a specific definition, or all definitions
//
