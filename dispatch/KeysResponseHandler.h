// KeysResponseHandler.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef I_KeysResponseHandler_h
#define I_KeysResponseHandler_h 1

#include "DODSResponseHandler.h"

class DODSTextInfo ;

/** @brief response handler that returns the list of keys defined in the
 * OPeNDAP initialization file.
 *
 * A request 'show keys;' will be handled by this response handler. It
 * returns the list of all keys currently defined in the OPeNDAP
 * initialization file and transmits the response as an informational response.
 *
 * @see DODSResponseObject
 * @see DODSContainer
 * @see DODSTransmitter
 * @see DODSTokenizer
 */
class KeysResponseHandler : public DODSResponseHandler
{
public:
				KeysResponseHandler( string name ) ;
    virtual			~KeysResponseHandler( void ) ;

    virtual void		execute( DODSDataHandlerInterface &dhi ) ;
    virtual void		transmit( DODSTransmitter *transmitter,
                                          DODSDataHandlerInterface &dhi ) ;

    static DODSResponseHandler *KeysResponseBuilder( string handler_name ) ;
};

#endif // I_KeysResponseHandler_h

// $Log: KeysResponseHandler.h,v $
// Revision 1.1  2005/04/19 17:53:08  pwest
// show keys handler
//
