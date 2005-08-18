// KeysResponseHandler.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "KeysResponseHandler.h"
#include "TheDODSKeys.h"
#include "DODSTextInfo.h"
#include "DODSParserException.h"
#include "DODSTokenizer.h"

KeysResponseHandler::KeysResponseHandler( string name )
    : DODSResponseHandler( name )
{
}

KeysResponseHandler::~KeysResponseHandler( )
{
}

/** @brief executes the command 'show keys;' by returning the list of
 * all keys defined in the OPeNDAP initialization file.
 *
 * This response handler knows how to retrieve the list of keys retrieved from
 * the OPeNDAP initialization file and stored in TheDODSKeys. A DODSTextInfo
 * informational response object is built to hold all of the information.
 *
 * The information is returned, one key per line, like:
 *
 * key: "&lt;key_name&gt;", value: "&lt;key_value&gt"
 *
 * @param dhi structure that holds request and response information
 * @throws DODSResponseException if there is a problem building the
 * response object
 * @see _DODSDataHandlerInterface
 * @see DODSTextInfo
 * @see TheDODSKeys
 */
void
KeysResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    DODSTextInfo *info = new DODSTextInfo( dhi.transmit_protocol == "HTTP" ) ;
    _response = info ;

    info->add_data( (string)"List of currently defined keys "
                    + " from the initialization file "
                    + TheDODSKeys->keys_file_name() + "\n" ) ;

    DODSKeys::Keys_citer ki = TheDODSKeys->keys_begin() ;
    DODSKeys::Keys_citer ke = TheDODSKeys->keys_end() ;
    for( ; ki != ke; ki++ )
    {
	string line = (string)"key: \"" + (*ki).first
	              + "\", value:\"" + (*ki).second
		      + "\"\n" ;
	info->add_data( line ) ;
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
KeysResponseHandler::transmit( DODSTransmitter *transmitter,
                                  DODSDataHandlerInterface &dhi )
{
    if( _response )
    {
	DODSTextInfo *info = dynamic_cast<DODSTextInfo *>(_response) ;
	transmitter->send_text( *info, dhi );
    }
}

DODSResponseHandler *
KeysResponseHandler::KeysResponseBuilder( string handler_name )
{
    return new KeysResponseHandler( handler_name ) ;
}

// $Log: KeysResponseHandler.cc,v $
// Revision 1.1  2005/04/19 17:53:08  pwest
// show keys handler
//
