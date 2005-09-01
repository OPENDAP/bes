// DODSParserException.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSParserException_h_
#define DODSParserException_h_ 1

#include "DODSBasicException.h"

/** @brief exception thrown if there is a problem parsing the request string
 * passed by the user.
 */
class DODSParserException : public DODSBasicException
{
public:
      			DODSParserException() {}
      			DODSParserException( const string &s )
			    : DODSBasicException( s ) {}
      virtual		~DODSParserException() {}
};

#endif //  DODSParserException_h_

// $Log: DODSParserException.h,v $
// Revision 1.4  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
// Revision 1.3  2004/12/15 17:39:03  pwest
// Added doxygen comments
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
