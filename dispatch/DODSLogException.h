// DODSLogException.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSLogException_h_
#define DODSLogException_h_ 1

#include "DODSBasicException.h"

/** @brief exception thrown if unable to open or write to the dods log file.
 */
class DODSLogException: public DODSBasicException
{
public:
      			DODSLogException() {}
      virtual		~DODSLogException() {}
};

#endif // DODSLogException_h_

// $Log: DODSLogException.h,v $
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
