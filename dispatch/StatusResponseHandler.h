// StatusResponseHandler.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef I_StatusResponseHandler_h
#define I_StatusResponseHandler_h 1

#include "DODSResponseHandler.h"

class StatusResponseHandler : public DODSResponseHandler
{
public:
				StatusResponseHandler( string name ) ;
    virtual			~StatusResponseHandler( void ) ;

    virtual void		parse( DODSTokenizer &tokenizer,
                                       DODSDataHandlerInterface &dhi ) ;
    virtual void		execute( DODSDataHandlerInterface &dhi ) ;
    virtual void		transmit( DODSTransmitter *transmitter,
                                          DODSDataHandlerInterface &dhi ) ;

    static DODSResponseHandler *StatusResponseBuilder( string handler_name ) ;
};

#endif // I_StatusResponseHandler_h

// $Log: StatusResponseHandler.h,v $
// Revision 1.1  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
