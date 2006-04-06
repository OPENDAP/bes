// TestException.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "TestException.h"
#include "DODSStatusReturn.h"

int
TestException::handleException( DODSException &e,
				DODSDataHandlerInterface &dhi )
{
    TestException *te = dynamic_cast<TestException*>(&e);
    if( te )
    {
	fprintf( stdout, "Reporting Test Exception.\n" ) ;
	fprintf( stdout, "%s\n", e.get_error_description().c_str() ) ;
	return 12 ;
    } 
    return DODS_EXECUTED_OK ;
}

