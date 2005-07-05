// TestRequestHandler.h

#ifndef I_TESTREQUESTHANDLER_H
#define I_TESTREQUESTHANDLER_H

#include "DODSRequestHandler.h"

class TestRequestHandler : public DODSRequestHandler {
public:
			TestRequestHandler( string name ) ;
    virtual		~TestRequestHandler( void ) ;

    static bool		test_build_resp1( DODSDataHandlerInterface &r ) ;
    static bool		test_build_resp2( DODSDataHandlerInterface &r ) ;
    static bool		test_build_resp3( DODSDataHandlerInterface &r ) ;
    static bool		test_build_resp4( DODSDataHandlerInterface &r ) ;

    int			test() ;
    int			_resp_num ;
};

#endif

