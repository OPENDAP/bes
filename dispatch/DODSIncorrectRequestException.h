// DODSIncorrectRequestException.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSIncorrectRequestException_h_
#define DODSIncorrectRequestException_h_ 1

#include "DODSBasicException.h"

class DODSIncorrectRequestException: public DODSBasicException
{
public:
      			DODSIncorrectRequestException() {}
			DODSIncorrectRequestException( const string &s )
			    : DODSBasicException( s ) {}
      virtual		~DODSIncorrectRequestException() {}
};

#endif // DODSIncorrectRequestException_h_

// $Log: DODSIncorrectRequestException.h,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
