// TestException.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef TestException_h_
#define TestException_h_ 1

#include "DODSBasicException.h"
#include "DODSDataHandlerInterface.h"

#define CEDAR_AUTHENTICATE_EXCEPTION 13

/** @brief exception thrown if authentication fails
 */
class TestException: public DODSBasicException
{
public:
      			TestException() :
			    DODSBasicException() {}
      			TestException( const string &s ) :
			    DODSBasicException( s ) {}
      virtual		~TestException() {}

      static int	handleException( DODSException &e,
					 DODSDataHandlerInterface &dhi ) ;
};

#endif // TestException_h_ 

