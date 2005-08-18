// DeleteResponseHandler.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef I_DeleteResponseHandler_h
#define I_DeleteResponseHandler_h 1

#include "DODSResponseHandler.h"

/** @brief response handler that deletes a containers, a definition or all
 * definitions.
 *
 * Possible requests handled by this response handler are:
 *
 * delete container &lt;container_name&gt;;
 * <BR />
 * delete definition &lt;def_name&gt;;
 * <BR />
 * delete definitions;
 *
 * There is no command to delete all containers.
 *
 * An informational response object is created and returned to the requester
 * to inform them whether the request was successful.
 *
 * @see DODSResponseObject
 * @see DODSContainer
 * @see DODSTransmitter
 * @see DODSTokenizer
 */
class DeleteResponseHandler : public DODSResponseHandler {
public:
				DeleteResponseHandler( string name ) ;
    virtual			~DeleteResponseHandler( void ) ;

    virtual void		execute( DODSDataHandlerInterface &dhi ) ;
    virtual void		transmit( DODSTransmitter *transmitter,
                                          DODSDataHandlerInterface &dhi ) ;

    static DODSResponseHandler *DeleteResponseBuilder( string handler_name ) ;
};

#endif // I_DeleteResponseHandler_h

// $Log: DeleteResponseHandler.h,v $
// Revision 1.1  2005/03/17 19:26:22  pwest
// added delete command to delete a specific container, a specific definition, or all definitions
//
