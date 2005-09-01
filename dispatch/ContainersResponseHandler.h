// ContainersResponseHandler.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef I_ContainersResponseHandler_h
#define I_ContainersResponseHandler_h 1

#include "DODSResponseHandler.h"

/** @brief response handler that returns list of continers currently defined
 *
 * A request 'show containers;' will be handled by this response handler. It
 * returns the list of currently defined containers for each container
 * persistence registered with the server and transmits the response as an
 * informational response.
 *
 * @see DODSResponseObject
 * @see DODSContainer
 * @see DODSTransmitter
 * @see DODSTokenizer
 */
class ContainersResponseHandler : public DODSResponseHandler {
public:
				ContainersResponseHandler( string name ) ;
    virtual			~ContainersResponseHandler( void ) ;

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
