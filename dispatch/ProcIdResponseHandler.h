// ProcIdResponseHandler.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef I_ProcIdResponseHandler_h
#define I_ProcIdResponseHandler_h 1

#include "DODSResponseHandler.h"

class ProcIdResponseHandler : public DODSResponseHandler
{
private:
    char *			fastpidconverter( long val,
                                                  char *buf,
						  int base ) ;
public:
				ProcIdResponseHandler( string name ) ;
    virtual			~ProcIdResponseHandler( void ) ;

    virtual void		parse( DODSTokenizer &tokenizer,
                                       DODSDataHandlerInterface &dhi ) ;
    virtual void		execute( DODSDataHandlerInterface &dhi ) ;
    virtual void		transmit( DODSTransmitter *transmitter,
                                          DODSDataHandlerInterface &dhi ) ;

    static DODSResponseHandler *ProcIdResponseBuilder( string handler_name ) ;
};

#endif // I_ProcIdResponseHandler_h

// $Log: ProcIdResponseHandler.h,v $
// Revision 1.1  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
