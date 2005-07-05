// DODSMySQLQueryException.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSMySQLQueryException_h_
#define DODSMySQLQueryException_h_ 1

#include "DODSBasicException.h"

class DODSMySQLQueryException: public DODSBasicException
{
public:
      			DODSMySQLQueryException() {}
      virtual		~DODSMySQLQueryException() {}
};  

#endif // DODSMySQLQueryException

// $Log: DODSMySQLQueryException.h,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
