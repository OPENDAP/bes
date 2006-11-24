// OPENDAP_RESPONSEResponseHandler.h

#ifndef I_OPENDAP_RESPONSEResponseHandler_h
#define I_OPENDAP_RESPONSEResponseHandler_h 1

#include "BESResponseHandler.h"

class OPENDAP_RESPONSEResponseHandler : public BESResponseHandler {
public:
				OPENDAP_RESPONSEResponseHandler( string name ) ;
    virtual			~OPENDAP_RESPONSEResponseHandler( void ) ;

    virtual void		execute( BESDataHandlerInterface &dhi ) ;
    virtual void		transmit( BESTransmitter *transmitter,
                                          BESDataHandlerInterface &dhi ) ;

    virtual void		dump( ostream &strm ) const ;

    static BESResponseHandler *OPENDAP_RESPONSEResponseBuilder( string handler_name ) ;
};

#endif // I_OPENDAP_RESPONSEResponseHandler_h

