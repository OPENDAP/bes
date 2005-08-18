// DefinitionsResponseHandler.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef I_DefinitionsResponseHandler_h
#define I_DefinitionsResponseHandler_h 1

#include "DODSResponseHandler.h"

/** @brief response handler that returns list of definitions currently defined
 *
 * A request 'show definitions;' will be handled by this response handler. It
 * returns the list of currently defined definitions and transmits the
 * response as an informational response.
 *
 * @see DODSResponseObject
 * @see DODSContainer
 * @see DODSTransmitter
 * @see DODSTokenizer
 */
class DefinitionsResponseHandler : public DODSResponseHandler {
public:
				DefinitionsResponseHandler( string name ) ;
    virtual			~DefinitionsResponseHandler( void ) ;

    virtual void		execute( DODSDataHandlerInterface &dhi ) ;
    virtual void		transmit( DODSTransmitter *transmitter,
                                          DODSDataHandlerInterface &dhi ) ;

    static DODSResponseHandler *DefinitionsResponseBuilder( string handler_name ) ;
};

#endif // I_DefinitionsResponseHandler_h

// $Log: DefinitionsResponseHandler.h,v $
// Revision 1.1  2005/03/15 20:06:20  pwest
// show definitions and show containers response handler
//
