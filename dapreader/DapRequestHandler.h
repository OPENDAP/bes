// DapRequestHandler.h

#ifndef I_DapRequestHandler_H
#define I_DapRequestHandler_H

#include "BESRequestHandler.h"

class DapRequestHandler : public BESRequestHandler {
public:
			DapRequestHandler( const string &name ) ;
    virtual		~DapRequestHandler( void ) ;

    virtual void	dump( ostream &strm ) const ;

    static bool		dap_build_das( BESDataHandlerInterface &dhi ) ;
    static bool		dap_build_dds( BESDataHandlerInterface &dhi ) ;
    static bool		dap_build_data( BESDataHandlerInterface &dhi ) ;
    static bool		dap_build_vers( BESDataHandlerInterface &dhi ) ;
    static bool		dap_build_help( BESDataHandlerInterface &dhi ) ;
};

#endif // DapRequestHandler.h

