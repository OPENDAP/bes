// DDSResponseHandler.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "DDSResponseHandler.h"
#include "DDS.h"
#include "cgi_util.h"
#include "DODSParserException.h"
#include "TheRequestHandlerList.h"

DDSResponseHandler::DDSResponseHandler( string name )
    : DODSResponseHandler( name )
{
}

DDSResponseHandler::~DDSResponseHandler( )
{
}

/** @brief parses the request 'get dds for &lt;def_name&gt;;' where def_name
 * is the name of a definition previously created.
 *
 * This request has already been parsed by the GetResponseHandler, so there is
 * nothing more to parse. If there is, then throw an exception.
 *
 * @param tokenizer holds on to the list of tokens to be parsed
 * @param dhi structure that holds request and response information
 * @throws DODSParserException if this method is called, as the request string
 * should have already been parsed.
 * @see DODSTokenizer
 * @see _DODSDataHandlerInterface
 */
void
DDSResponseHandler::parse( DODSTokenizer &tokenizer,
                           DODSDataHandlerInterface &dhi )
{
    throw( DODSParserException( (string)"Improper command " + get_name() ) ) ;
}

/** @brief executes the command 'get dds for &lt;def_name&gt;;' by executing
 * the request for each container in the specified defnition def_name.
 *
 * For each container in the specified defnition def_name go to the request
 * handler for that container and have it add to the OPeNDAP DDS response
 * object. The DDS response object is built within this method and passed
 * to the request handler list.
 *
 * @param dhi structure that holds request and response information
 * @throws DODSResponseException if there is a problem building the
 * response object
 * @see _DODSDataHandlerInterface
 * @see DDS
 * @see TheRequestHandlerList
 */
void
DDSResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    _response = new DDS( "virtual" ) ;
    TheRequestHandlerList->execute_each( dhi ) ;
}

/** @brief transmit the response object built by the execute command
 * using the specified transmitter object
 *
 * If a response object was built then transmit it using the send_dds method.
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see DDS
 * @see DODSTransmitter
 * @see _DODSDataHandlerInterface
 */
void
DDSResponseHandler::transmit( DODSTransmitter *transmitter,
                              DODSDataHandlerInterface &dhi )
{
    if( _response )
    {
	DDS *dds = dynamic_cast<DDS *>(_response) ;
	transmitter->send_dds( *dds, dhi ) ;
    }
}

DODSResponseHandler *
DDSResponseHandler::DDSResponseBuilder( string handler_name )
{
    return new DDSResponseHandler( handler_name ) ;
}

// $Log: DDSResponseHandler.cc,v $
// Revision 1.3  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
