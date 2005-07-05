// ContainersResponseHandler.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef I_ContainersResponseHandler_h
#define I_ContainersResponseHandler_h 1

#include "DODSResponseHandler.h"

class ContainersResponseHandler : public DODSResponseHandler {
public:
				ContainersResponseHandler( string name ) ;
    virtual			~ContainersResponseHandler( void ) ;

    virtual void		parse( DODSTokenizer &tokenizer,
                                       DODSDataHandlerInterface &dhi ) ;
    virtual void		execute( DODSDataHandlerInterface &dhi ) ;
    virtual void		transmit( DODSTransmitter *transmitter,
                                          DODSDataHandlerInterface &dhi ) ;

    static DODSResponseHandler *ContainersResponseBuilder( string handler_name ) ;
};

#endif // I_ContainersResponseHandler_h

// $Log: ContainersResponseHandler.h,v $
// Revision 1.1  2005/03/15 20:06:20  pwest
// show definitions and show containers response handler
//
