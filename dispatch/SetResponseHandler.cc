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

/** @brief parses the request to create a new container or replace an already
 * existing container given a symbolic name, a real name, and a data type.
 *
 * The syntax for a request handled by this response handler is:
 *
 * set container values * &lt;sym_name&gt;,&lt;real_name&gt;,&lt;data_type&gt;;
 *
 * The request must end with a semicolon and must contain the symbolic name,
 * the real name (in most cases a file name), and the type of data represented
 * by this container (e.g. cedar, netcdf, cdf, hdf, etc...).
 *
 * @param tokenizer holds on to the list of tokens to be parsed
 * @param dhi structure that holds request and response information
 * @throws DODSParserException if there is a problem parsing the request
 * @see DODSTokenizer
 * @see _DODSDataHandlerInterface
 */
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

/** @brief executes the command to create a new container or replaces an
 * already existing container based on the provided symbolic name.
 *
 * The symbolic name is first looked up in the volatile persistence object. If
 * the symbolic name already exists (the container already exists) then the
 * already existing container is removed and the new one added using the given
 * real name (usually a file name) and the type of data represented by this
 * container (e.g. cedar, cdf, netcdf, hdf, etc...)
 *
 * An informational response object DODSTextInfo is created to hold whether or
 * not the container was successfully added/replaced. Possible responses are:
 *
 * Successfully added container &lt;sym_name&gt; to persistent store volatile
 * Successfully replaced container &lt;sym_name&gt; to persistent store volatile
 *
 * Unable to add container &lt;sym_name&gt; to persistent store volatile
 *
 * @param dhi structure that holds request and response information
 * @throws DODSResponseException if there is a problem building the
 * response object
 * @see _DODSDataHandlerInterface
 * @see DODSTextInfo
 * @see ThePersistenceList
 * @see DODSContainerPersistence
 * @see DODSContainer
 */
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
