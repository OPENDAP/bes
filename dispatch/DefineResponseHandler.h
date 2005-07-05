// DefineResponseHandler.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef I_DefineResponseHandler_h
#define I_DefineResponseHandler_h 1

#include "DODSResponseHandler.h"

class DefineResponseHandler : public DODSResponseHandler
{
private:
    string			_def_name ;
public:
				DefineResponseHandler( string name ) ;
    virtual			~DefineResponseHandler( void ) ;

    virtual void		parse( DODSTokenizer &tokenizer,
                                       DODSDataHandlerInterface &dhi ) ;
    virtual void		execute( DODSDataHandlerInterface &dhi ) ;
    virtual void		transmit( DODSTransmitter *transmitter,
                                          DODSDataHandlerInterface &dhi ) ;

    static DODSResponseHandler *DefineResponseBuilder( string handler_name ) ;
};

#endif // I_DefineResponseHandler_h

// $Log: DefineResponseHandler.h,v $
// Revision 1.1  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
