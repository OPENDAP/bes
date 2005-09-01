// DDXResponseHandler.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "DDXResponseHandler.h"
#include "DAS.h"
#include "DDS.h"
#include "cgi_util.h"
#include "DODSResponseNames.h"
#include "DODSParserException.h"
#include "TheRequestHandlerList.h"

DDXResponseHandler::DDXResponseHandler( string name )
    : DODSResponseHandler( name )
{
}

DDXResponseHandler::~DDXResponseHandler( )
{
}

/** @brief executes the command 'get ddx for &lt;def_name&gt;;' by executing
 * the request for each container in the specified defnition def_name.
 *
 * For each container in the specified defnition def_name go to the request
 * handler for that container and have it first add to the OPeNDAP DDS response
 * object. Oncew the DDS object has been filled in, repeat the process but
 * this time for the OPeNDAP DAS response object. Then add the attributes from
 * the DAS object to the DDS object.
 *
 * @param dhi structure that holds request and response information
 * @throws DODSResponseException if there is a problem building the
 * response object
 * @see _DODSDataHandlerInterface
 * @see DDS
 * @see DAS
 * @see TheRequestHandlerList
 */
void
DDXResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    // Fill the DDS
    DDS *dds = new DDS( "virtual" ) ;
    _response = dds ;
    _response_name = DDS_RESPONSE ;
    TheRequestHandlerList->execute_each( dhi ) ;

    // Fill the DAS
    DAS *das = new DAS ;
    _response = das ;
    _response_name = DAS_RESPONSE ;
    TheRequestHandlerList->execute_each( dhi ) ;

    // Transfer the DAS to the DDS
    dds->transfer_attributes( das ) ;

    _response = dds ;
}

/** @brief transmit the response object built by the execute command
 * using the specified transmitter object
 *
 * If a response object was built then transmit it using the send_ddx method.
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see DDS
 * @see DAS
 * @see DODSTransmitter
 * @see _DODSDataHandlerInterface
 */
void
DDXResponseHandler::transmit( DODSTransmitter *transmitter,
                              DODSDataHandlerInterface &dhi )
{
    if( _response )
    {
	DDS *dds = dynamic_cast<DDS *>(_response) ;
	transmitter->send_ddx( *dds, dhi ) ;
    }
}

DODSResponseHandler *
DDXResponseHandler::DDXResponseBuilder( string handler_name )
{
    return new DDXResponseHandler( handler_name ) ;
}

// $Log: DDXResponseHandler.cc,v $
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
