// VersionResponseHandler.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef I_VersionResponseHandler_h
#define I_VersionResponseHandler_h 1

#include "DODSResponseHandler.h"

class DODSTextInfo ;

/** @brief response handler that returns the version of the OPeNDAP-g server
 * and the version of any data request handlers registered with the server.
 *
 * A request 'show version;' will be handled by this response handler. It
 * returns the version of the OPeNDAP-g server and the version of any data
 * request handlers registered with the server.
 *
 * @see DODSResponseObject
 * @see DODSRequestHandler
 * @see DODSContainer
 * @see DODSTransmitter
 * @see DODSTokenizer
 */
class VersionResponseHandler : public DODSResponseHandler
{
public:
				VersionResponseHandler( string name ) ;
    virtual			~VersionResponseHandler( void ) ;

    virtual void		execute( DODSDataHandlerInterface &dhi ) ;
    virtual void		transmit( DODSTransmitter *transmitter,
                                          DODSDataHandlerInterface &dhi ) ;

    static DODSResponseHandler *VersionResponseBuilder( string handler_name ) ;
};

#endif // I_VersionResponseHandler_h

// $Log: VersionResponseHandler.h,v $
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
