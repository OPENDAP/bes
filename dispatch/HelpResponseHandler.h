// HelpResponseHandler.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef I_HelpResponseHandler_h
#define I_HelpResponseHandler_h 1

#include "DODSResponseHandler.h"

/** @brief response handler that returns help information about the server
 * including what types of data are handled by the server and information
 * about those handlers, and what commands are accepted by the server.
 *
 * A request 'show help;' will be handled by this response handler. It
 * returns general help information as well as help information for all of
 * the different types of data handled by this server.
 *
 * @see DODSResponseObject
 * @see DODSContainer
 * @see DODSTransmitter
 * @see DODSTokenizer
 */
class HelpResponseHandler : public DODSResponseHandler {
public:
				HelpResponseHandler( string name ) ;
    virtual			~HelpResponseHandler( void ) ;

    virtual void		execute( DODSDataHandlerInterface &dhi ) ;
    virtual void		transmit( DODSTransmitter *transmitter,
                                          DODSDataHandlerInterface &dhi ) ;

    static DODSResponseHandler *HelpResponseBuilder( string handler_name ) ;
};

#endif // I_HelpResponseHandler_h

// $Log: HelpResponseHandler.h,v $
// Revision 1.3  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
