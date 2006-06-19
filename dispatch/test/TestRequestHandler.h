// TestRequestHandler.h

#ifndef I_TESTREQUESTHANDLER_H
#define I_TESTREQUESTHANDLER_H

#include "BESRequestHandler.h"

class TestRequestHandler : public BESRequestHandler {
public:
			TestRequestHandler( string name ) ;
    virtual		~TestRequestHandler( void ) ;

    static bool		test_build_resp1( BESDataHandlerInterface &r ) ;
    static bool		test_build_resp2( BESDataHandlerInterface &r ) ;
    static bool		test_build_resp3( BESDataHandlerInterface &r ) ;
    static bool		test_build_resp4( BESDataHandlerInterface &r ) ;

    int			test() ;
    int			_resp_num ;
};

#endif

