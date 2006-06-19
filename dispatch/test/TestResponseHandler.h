// TestResponseHandler.h

#ifndef I_TESTRESPONSEHANDLER_H
#define I_TESTRESPONSEHANDLER_H

#include "BESResponseHandler.h"

class TestResponseHandler : public BESResponseHandler {
public:
				TestResponseHandler( string name ) ;
    virtual			~TestResponseHandler( void ) ;

    virtual void		execute( BESDataHandlerInterface &dhi ) ;
    virtual void		transmit( BESDataHandlerInterface &dhi ) ;
    virtual void		execute_each( BESDataHandlerInterface &dhi ) ;
    virtual void		execute_all( BESDataHandlerInterface &dhi ) ;
    virtual void		transmit( BESTransmitter *transmitter,
                                          BESDataHandlerInterface &dhi ) ;

    static BESResponseHandler	*TestResponseBuilder( string handler_name ) ;
};

#endif

