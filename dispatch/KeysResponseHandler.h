// KeysResponseHandler.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef I_KeysResponseHandler_h
#define I_KeysResponseHandler_h 1

#include "DODSResponseHandler.h"

class DODSTextInfo ;

class KeysResponseHandler : public DODSResponseHandler
{
public:
				KeysResponseHandler( string name ) ;
    virtual			~KeysResponseHandler( void ) ;

    virtual void		parse( DODSTokenizer &tokenizer,
                                       DODSDataHandlerInterface &dhi ) ;
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
