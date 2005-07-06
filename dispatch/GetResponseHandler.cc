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

/** @brief return the current response object
 *
 * Returns the current response object, null if one has not yet been
 * created. The response handler maintains ownership of the response
 * object, unless the set_response_object() method is called at which
 * point the caller of get_response_object() becomes the owner.
 *
 * Because a get response handler contains a sub response handler that really
 * knows how to build the response object, this get_response_object method
 * turns around and calls the same method on the sub response handler.
 *
 * @see DODSResponseObject
 */
DODSResponseObject *
GetResponseHandler::get_response_object()
{
    if( _sub_response ) return _sub_response->get_response_object() ;
    return DODSResponseHandler::get_response_object() ;
}

/** @brief knows how to parse a get request
 *
 * This class knows how to parse a get request, building a sub response
 * handler that actually knows how to build the requested response
 * object, such as das, dds, data, ddx, etc...
 *
 * A get request looks like:
 *
 * get &lt;response_type&gt; for &lt;def_name&gt; [return as &lt;ret_name&gt;;
 *
 * where response_type is the type of response being requested, for example
 * das, dds, dods.
 * where def_name is the name of the definition that has already been created,
 * like a view into the data
 * where ret_name is the method of transmitting the response. This is
 * optional.
 *
 * This parse method creates the sub response handler, retrieves the
 * definition information and finds the return object if one is specified.
 *
 * @param tokenizer holds on to the list of tokens to be parsed
 * @param dhi structure that holds request and response information
 * @throws DODSParserException if there is a problem parsing the request
 */
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

/** @brief knows how to build a requested response object
 *
 * Redirects the execute method to the sub response handler, which knows how
 * to build the response object.
 *
 * @param dhi structure that holds request and response information
 * @throws DODSResponseException if there is a problem building the
 * response object
 * @see _DODSDataHandlerInterface
 * @see DODSResponseObject
 */
void
GetResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    if( _sub_response ) _sub_response->execute( dhi ) ;
}

/** @brief transmit the respobse object built by the execute command
 * using the specified transmitter object
 *
 * Redirects the transmit method to the sub response handler, which knows
 * which method of the transmitter to use
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see DODSResponseObject
 * @see DODSTransmitter
 * @see _DODSDataHandlerInterface
 */
void
GetResponseHandler::transmit( DODSTransmitter *transmitter,
			      DODSDataHandlerInterface &dhi )
{
    if( _sub_response ) _sub_response->transmit( transmitter, dhi ) ;
}

/** @brief replaces the current response object with the specified one
 *
 * This method is used to replace the response object with a new one, for
 * example if during aggregation a new response object is built from the
 * current response object.
 *
 * The current response object is NOT deleted. The response handler
 * assumes ownership of the new response object.
 *
 * Redirects the transmit method to the sub response handler, which knows how
 * to build the response object.
 *
 * @param new_response new response object used to replace the current one
 * @see DODSResponseObject
 */
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
