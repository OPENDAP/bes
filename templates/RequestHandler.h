// OPENDAP_CLASSRequestHandler.h

#ifndef I_OPENDAP_CLASSRequestHandler_H
#define I_OPENDAP_CLASSRequestHandler_H

#include "DODSRequestHandler.h"

class OPENDAP_CLASSRequestHandler : public DODSRequestHandler {
public:
			OPENDAP_CLASSRequestHandler( string name ) ;
    virtual		~OPENDAP_CLASSRequestHandler( void ) ;

    static bool		OPENDAP_TYPE_build_vers( DODSDataHandlerInterface &dhi ) ;
    static bool		OPENDAP_TYPE_build_help( DODSDataHandlerInterface &dhi ) ;
};

#endif // OPENDAP_CLASSRequestHandler.h

