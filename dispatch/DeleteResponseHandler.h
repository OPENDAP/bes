// DeleteResponseHandler.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef I_DeleteResponseHandler_h
#define I_DeleteResponseHandler_h 1

#include "DODSResponseHandler.h"

class DeleteResponseHandler : public DODSResponseHandler {
private:
    string			_def_name ;
    string			_store_name ;
    string			_container_name ;
    bool			_definitions ;
public:
				DeleteResponseHandler( string name ) ;
    virtual			~DeleteResponseHandler( void ) ;

    virtual void		parse( DODSTokenizer &tokenizer,
                                       DODSDataHandlerInterface &dhi ) ;
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
