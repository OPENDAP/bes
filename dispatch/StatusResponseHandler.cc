// StatusResponseHandler.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "StatusResponseHandler.h"
#include "DODSTextInfo.h"
#include "DODSParserException.h"
#include "DODSStatus.h"
#include "DODSTokenizer.h"

StatusResponseHandler::StatusResponseHandler( string name )
    : DODSResponseHandler( name )
{
}

StatusResponseHandler::~StatusResponseHandler( )
{
}

/** @brief executes the command 'show status;' by returning the status of
 * the server process using DODSStatus
 *
 * This response handler knows how to retrieve the status for the server
 * from DODSStatus and stores it in a DODSTextInfo informational response
 * object.
 *
 * @param dhi structure that holds request and response information
 * @throws DODSResponseException if there is a problem building the
 * response object
 * @see _DODSDataHandlerInterface
 * @see DODSTextInfo
 * @see DODSStatus
 */
void
StatusResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    DODSTextInfo *info = new DODSTextInfo( dhi.transmit_protocol == "HTTP" ) ;
    _response = info ;
    DODSStatus s ;
    info->add_data( "Listener boot time: " + s.get_status() ) ;
}

/** @brief transmit the response object built by the execute command
 * using the specified transmitter object
 *
 * If a response object was built then transmit it as text.
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see DODSResponseObject
 * @see DODSTransmitter
 * @see _DODSDataHandlerInterface
 */
void
StatusResponseHandler::transmit( DODSTransmitter *transmitter,
                                  DODSDataHandlerInterface &dhi )
{
    if( _response )
    {
	DODSTextInfo *info = dynamic_cast<DODSTextInfo *>(_response) ;
	transmitter->send_text( *info, dhi ) ;
    }
}

DODSResponseHandler *
StatusResponseHandler::StatusResponseBuilder( string handler_name )
{
    return new StatusResponseHandler( handler_name ) ;
}

// $Log: StatusResponseHandler.cc,v $
// Revision 1.2  2005/03/15 19:58:35  pwest
// using DODSTokenizer to get first and next tokens
//
// Revision 1.1  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
