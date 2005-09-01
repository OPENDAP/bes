// DataResponseHandler.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef I_DataResponseHandler_H
#define I_DataResponseHandler_H

#include "DODSResponseHandler.h"

/** @brief response handler that builds an OPeNDAP DDS object that includes
 * not only the data definitions, but also the data.
 *
 * A request 'get data for &lt;def_name&gt; [return as &lt;ret_name&gt;];'
 * will be handled by this response handler. Given a definition name it
 * determines what containers are to be used to build the response object.
 * It then transmits the response object using the method send_data.
 *
 * @see DDS
 * @see DODSContainer
 * @see DODSTransmitter
 * @see DODSTokenizer
 */
class DataResponseHandler : public DODSResponseHandler {
public:
				DataResponseHandler( string name ) ;
    virtual			~DataResponseHandler(void) ;

    virtual void		execute( DODSDataHandlerInterface &dhi ) ;
    virtual void		transmit( DODSTransmitter *transmitter,
                                          DODSDataHandlerInterface &dhi ) ;

    static DODSResponseHandler *DataResponseBuilder( string handler_name ) ;
};

#endif

// $Log: DataResponseHandler.h,v $
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
