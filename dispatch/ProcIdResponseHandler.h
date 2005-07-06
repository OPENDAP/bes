// ProcIdResponseHandler.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef I_ProcIdResponseHandler_h
#define I_ProcIdResponseHandler_h 1

#include "DODSResponseHandler.h"

/** @brief response handler that returns the process id for the server process
 *
 * A request 'show process;' will be handled by this response handler. It
 * returns the process id of the server.
 *
 * @see DODSResponseObject
 * @see DODSContainer
 * @see DODSTransmitter
 * @see DODSTokenizer
 */
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
