// DASResponseHandler.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "DASResponseHandler.h"
#include "DAS.h"
#include "cgi_util.h"
#include "DODSParserException.h"
#include "DODSRequestHandlerList.h"

DASResponseHandler::DASResponseHandler( string name )
    : DODSResponseHandler( name )
{
}

DASResponseHandler::~DASResponseHandler( )
{
}

/** @brief executes the command 'get das for &lt;def_name&gt;;' by executing
 * the request for each container in the specified defnition def_name.
 *
 * For each container in the specified defnition def_name go to the request
 * handler for that container and have it add to the OPeNDAP DAS response
 * object. The DAS response object is built within this method and passed
 * to the request handler list.
 *
 * @param dhi structure that holds request and response information
 * @throws DODSResponseException if there is a problem building the
 * response object
 * @see _DODSDataHandlerInterface
 * @see DAS
 * @see DODSRequestHandlerList
 */
void
DASResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    _response = new DAS ;
    DODSRequestHandlerList::TheList()->execute_each( dhi ) ;
}

/** @brief transmit the response object built by the execute command
 * using the specified transmitter object
 *
 * If a response object was built then transmit it using the send_das method.
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see DAS
 * @see DODSTransmitter
 * @see _DODSDataHandlerInterface
 */
void
DASResponseHandler::transmit( DODSTransmitter *transmitter,
                              DODSDataHandlerInterface &dhi )
{
    if( _response )
    {
	DAS *das = dynamic_cast<DAS *>(_response) ;
	transmitter->send_das( *das, dhi ) ;
    }
}

DODSResponseHandler *
DASResponseHandler::DASResponseBuilder( string handler_name )
{
    return new DASResponseHandler( handler_name ) ;
}

// $Log: DASResponseHandler.cc,v $
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
