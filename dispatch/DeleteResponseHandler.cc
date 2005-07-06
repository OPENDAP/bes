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

/** @brief parses the request to delete a container, a definition or all
 * definitions.
 *
 * Possible requests handled by this response handler are:
 *
 * delete container &lt;container_name&gt;;
 * <BR />
 * delete definition &lt;def_name&gt;;
 * <BR />
 * delete definitions;
 *
 * And remember to terminate all commands with a semicolon (;)
 *
 * @param tokenizer holds on to the list of tokens to be parsed
 * @param dhi structure that holds request and response information
 * @throws DODSParserException if there is a problem parsing the request
 * @see DODSTokenizer
 * @see _DODSDataHandlerInterface
 */
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

/** @brief executes the command to delete a container, a definition, or all
 * definitions.
 *
 * Removes definitions from TheDefineList and containers from volatile
 * container persistence object, which is found in ThePersistenceList.
 *
 * The response object built is a DODSTextInfo object. Status of the delition
 * will be added to the informational object, one of:
 *
 * Successfully deleted all definitions
 * <BR />
 * Successfully deleted definition "&lt;_def_name&gt;"
 * <BR />
 * Definition "&lt;def_name&gt;" does not exist.  Unable to delete.
 * <BR />
 * Successfully deleted container "&lt;container_name&gt;" from persistent store "volatile"
 * <BR />
 * Unable to delete container. The container "&lt;container_name&gt;" does not exist in the persistent store "volatile"
 * <BR />
 * The persistence store "volatile" does not exist. Unable to delete container "&lt;container_name&gt;"
 *
 * @param dhi structure that holds request and response information
 * @throws DODSResponseException if there is a problem building the
 * response object
 * @see _DODSDataHandlerInterface
 * @see DODSTextInfo
 * @see TheDefineList
 * @see DODSDefine
 * @see ThePersistenceList
 * @see DODSContainerPersistence
 */
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
			  + "\"\n" ;
	    info->add_data( line ) ;
	}
    }
}

/** @brief transmit the response object built by the execute command
 * using the specified transmitter object
 *
 * If a response object was built then transmit it as text.
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see DODSTextInfo
 * @see DODSTransmitter
 * @see _DODSDataHandlerInterface
 */
void
DeleteResponseHandler::transmit( DODSTransmitter *transmitter,
                               DODSDataHandlerInterface &dhi )
{
    if( _response )
    {
	DODSTextInfo *info = dynamic_cast<DODSTextInfo *>(_response) ;
	transmitter->send_text( *info, dhi );
    }
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
