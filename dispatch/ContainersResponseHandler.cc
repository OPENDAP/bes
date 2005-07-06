// ContainersResponseHandler.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "ContainersResponseHandler.h"
#include "DODSTokenizer.h"
#include "DODSTextInfo.h"
#include "ThePersistenceList.h"

ContainersResponseHandler::ContainersResponseHandler( string name )
    : DODSResponseHandler( name )
{
}

ContainersResponseHandler::~ContainersResponseHandler( )
{
}

/** @brief parses the request 'show containers;'
 *
 * The syntax for a request handled by this response handler is 'show
 * containers;'. The keywords 'show' and 'containers' have already been
 * parsed, which is how we got to the parse method. This method makes sure
 * that the command is terminated by a semicolon and that there is no more
 * text after the keyword 'containers'.
 *
 * @param tokenizer holds on to the list of tokens to be parsed
 * @param dhi structure that holds request and response information
 * @throws DODSParserException if there is a problem parsing the request
 * @see DODSTokenizer
 * @see _DODSDataHandlerInterface
 */
void
ContainersResponseHandler::parse( DODSTokenizer &tokenizer,
                           DODSDataHandlerInterface &dhi )
{
    string my_token = tokenizer.get_next_token() ;
    if( my_token != ";" )
    {
	tokenizer.parse_error( my_token + " not expected" ) ;
    }
}

/** @brief executes the command 'show containers;' by returning the list of
 * currently defined containers
 *
 * This response handler knows how to retrieve the list of containers
 * currently defined in the server. It simply asks the container persistence
 * list, that all containers have registered with, to show all containers
 * given the DODSTextInfo object created here.
 *
 * @param dhi structure that holds request and response information
 * @throws DODSResponseException if there is a problem building the
 * response object
 * @see _DODSDataHandlerInterface
 * @see DODSTextInfo
 */
void
ContainersResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    DODSTextInfo *info = new DODSTextInfo( dhi.transmit_protocol == "HTTP" ) ;
    _response = info ;
    ThePersistenceList->show_containers( *info ) ;
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
ContainersResponseHandler::transmit( DODSTransmitter *transmitter,
                               DODSDataHandlerInterface &dhi )
{
    if( _response )
    {
	DODSTextInfo *info = dynamic_cast<DODSTextInfo *>(_response) ;
	transmitter->send_text( *info, dhi );
    }
}

DODSResponseHandler *
ContainersResponseHandler::ContainersResponseBuilder( string handler_name )
{
    return new ContainersResponseHandler( handler_name ) ;
}

// $Log: ContainersResponseHandler.cc,v $
// Revision 1.1  2005/03/15 20:06:20  pwest
// show definitions and show containers response handler
//
