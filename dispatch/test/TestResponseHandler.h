// TestResponseHandler.h

#ifndef I_TESTRESPONSEHANDLER_H
#define I_TESTRESPONSEHANDLER_H

#include "DODSResponseHandler.h"

class TestResponseHandler : public DODSResponseHandler {
public:
				TestResponseHandler( string name ) ;
    virtual			~TestResponseHandler( void ) ;

    virtual void		execute( DODSDataHandlerInterface &dhi ) ;
    virtual void		transmit( DODSDataHandlerInterface &dhi ) ;
    virtual void		execute_each( DODSDataHandlerInterface &dhi ) ;
    virtual void		execute_all( DODSDataHandlerInterface &dhi ) ;
    virtual void		transmit( DODSTransmitter *transmitter,
                                          DODSDataHandlerInterface &dhi ) ;

    static DODSResponseHandler	*TestResponseBuilder( string handler_name ) ;
};

#endif

