// OPENDAP_RESPONSEResponseHandler.h

#ifndef I_OPENDAP_RESPONSEResponseHandler_h
#define I_OPENDAP_RESPONSEResponseHandler_h 1

#include "DODSResponseHandler.h"

class OPENDAP_RESPONSEResponseHandler : public DODSResponseHandler {
public:
				OPENDAP_RESPONSEResponseHandler( string name ) ;
    virtual			~OPENDAP_RESPONSEResponseHandler( void ) ;

    virtual void		execute( DODSDataHandlerInterface &dhi ) ;
    virtual void		transmit( DODSTransmitter *transmitter,
                                          DODSDataHandlerInterface &dhi ) ;

    static DODSResponseHandler *OPENDAP_RESPONSEResponseBuilder( string handler_name ) ;
};

#endif // I_OPENDAP_RESPONSEResponseHandler_h

