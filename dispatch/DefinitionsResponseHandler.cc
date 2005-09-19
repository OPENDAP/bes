// DefinitionsResponseHandler.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "DefinitionsResponseHandler.h"
#include "DODSTokenizer.h"
#include "DODSTextInfo.h"
#include "DODSDefineList.h"

DefinitionsResponseHandler::DefinitionsResponseHandler( string name )
    : DODSResponseHandler( name )
{
}

DefinitionsResponseHandler::~DefinitionsResponseHandler( )
{
}

/** @brief executes the command 'show definitions;' by returning the list of
 * currently defined definitions
 *
 * This response handler knows how to retrieve the list of definitions
 * currently defined in the server. It simply asks the definition list
 * to show all definitions given the DODSTextInfo object created here.
 *
 * @param dhi structure that holds request and response information
 * @throws DODSResponseException if there is a problem building the
 * response object
 * @see _DODSDataHandlerInterface
 * @see DODSTextInfo
 * @see DODSDefineList
 */
void
DefinitionsResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    DODSTextInfo *info = new DODSTextInfo( dhi.transmit_protocol == "HTTP" ) ;
    _response = info ;
    DODSDefineList::TheList()->show_definitions( *info ) ;
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
DefinitionsResponseHandler::transmit( DODSTransmitter *transmitter,
                               DODSDataHandlerInterface &dhi )
{
    if( _response )
    {
	DODSTextInfo *info = dynamic_cast<DODSTextInfo *>(_response) ;
	transmitter->send_text( *info, dhi );
    }
}

DODSResponseHandler *
DefinitionsResponseHandler::DefinitionsResponseBuilder( string handler_name )
{
    return new DefinitionsResponseHandler( handler_name ) ;
}

// $Log: DefinitionsResponseHandler.cc,v $
// Revision 1.1  2005/03/15 20:06:20  pwest
// show definitions and show containers response handler
//
