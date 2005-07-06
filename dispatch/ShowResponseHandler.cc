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

/** @brief return the current response object
 *
 * Returns the current response object, null if one has not yet been
 * created. The response handler maintains ownership of the response
 * object, unless the set_response_object() method is called at which
 * point the caller of get_response_object() becomes the owner.
 *
 * Because a show response handler contains a sub response handler that really
 * knows how to build the response object, this get_response_object method
 * turns around and calls the me method on the sub response handler.
 *
 * @see DODSResponseObject
 */
DODSResponseObject *
ShowResponseHandler::get_response_object()
{
    if( _sub_response ) return _sub_response->get_response_object() ;
    return DODSResponseHandler::get_response_object() ;
}

/** @brief knows how to parse a show request
 *
 * This class knows how to parse a show request, building a sub response
 * handler that actually knows how to build the requested response
 * object, such as for show help or show version.
 *
 * A show request looks like:
 *
 * get &lt;info_type&gt;;
 *
 * where info_type is the type of information that the user is requesting,
 * such as help or version
 *
 * This parse method creates the sub response handler that knows how to create
 * the specified information, such as creating HelpResponseHandler.
 *
 * @param tokenizer holds on to the list of tokens to be parsed
 * @param dhi structure that holds request and response information
 * @throws DODSParserException if there is a problem parsing the request
 */
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
ShowResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    _sub_response->execute( dhi ) ;
}

/** @brief transmit the response object built by the execute command
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
ShowResponseHandler::transmit( DODSTransmitter *transmitter,
			      DODSDataHandlerInterface &dhi )
{
    _sub_response->transmit( transmitter, dhi ) ;
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
