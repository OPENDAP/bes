// StatusResponseHandler.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef I_StatusResponseHandler_h
#define I_StatusResponseHandler_h 1

#include "DODSResponseHandler.h"

/** @brief response handler that returns the status of server process
 *
 * A request 'show status;' will be handled by this response handler. It
 * returns the status of the server in an informational response object.
 *
 * @see DODSResponseObject
 * @see DODSContainer
 * @see DODSTransmitter
 * @see DODSTokenizer
 */
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
