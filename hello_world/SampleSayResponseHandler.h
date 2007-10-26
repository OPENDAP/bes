// SampleSayResponseHandler.h

#ifndef I_SampleSayResponseHandler_h
#define I_SampleSayResponseHandler_h 1

#include "BESResponseHandler.h"

class SampleSayResponseHandler : public BESResponseHandler {
public:
				SampleSayResponseHandler( const string &name ) ;
    virtual			~SampleSayResponseHandler( void ) ;

    virtual void		execute( BESDataHandlerInterface &dhi ) ;
    virtual void		transmit( BESTransmitter *transmitter,
                                          BESDataHandlerInterface &dhi ) ;

    virtual void		dump( ostream &strm ) const ;

    static BESResponseHandler *SampleSayResponseBuilder( const string &name ) ;
};

#endif // I_SampleSayResponseHandler_h

