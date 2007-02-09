// SayResponseHandler.h

#ifndef I_SayResponseHandler_h
#define I_SayResponseHandler_h 1

#include "BESResponseHandler.h"

class SayResponseHandler : public BESResponseHandler {
public:
				SayResponseHandler( string name ) ;
    virtual			~SayResponseHandler( void ) ;

    virtual void		execute( BESDataHandlerInterface &dhi ) ;
    virtual void		transmit( BESTransmitter *transmitter,
                                          BESDataHandlerInterface &dhi ) ;

    virtual void		dump( ostream &strm ) const ;

    static BESResponseHandler *SayResponseBuilder( string handler_name ) ;
};

#endif // I_SayResponseHandler_h

