// DODSAuthenticateException.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSAuthenticateException_h_
#define DODSAuthenticateException_h_ 1

#include "DODSBasicException.h"

/** @brief exception thrown if authentication fails
 */
class DODSAuthenticateException: public DODSBasicException
{
public:
      			DODSAuthenticateException() {}
      virtual		~DODSAuthenticateException() {}
};

#endif // DODSAuthenticateException_h_ 

// $Log: DODSAuthenticateException.h,v $
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
