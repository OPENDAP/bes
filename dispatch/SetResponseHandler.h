// SetResponseHandler.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef I_SetResponseHandler_h
#define I_SetResponseHandler_h 1

#include "DODSResponseHandler.h"

class SetResponseHandler : public DODSResponseHandler
{
private:
    string			_symbolic_name ;
    string			_real_name ;
    string			_type ;
    string			_persistence ;
public:
				SetResponseHandler( string name ) ;
    virtual			~SetResponseHandler( void ) ;

    virtual void		parse( DODSTokenizer &tokenizer,
                                       DODSDataHandlerInterface &dhi ) ;
    virtual void		execute( DODSDataHandlerInterface &dhi ) ;
    virtual void		transmit( DODSTransmitter *transmitter,
                                          DODSDataHandlerInterface &dhi ) ;

    static DODSResponseHandler *SetResponseBuilder( string handler_name ) ;
};

#endif // I_SetResponseHandler_h

// $Log: SetResponseHandler.h,v $
// Revision 1.1  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
