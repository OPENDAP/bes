// OPeNDAPDatabaseException.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef OPeNDAPDatabaseException_h_
#define OPeNDAPDatabaseException_h_ 1

#include "DODSBasicException.h"

class OPeNDAPDatabaseException: public DODSBasicException
{
public:
      			OPeNDAPDatabaseException()
			    : DODSBasicException() {}
			OPeNDAPDatabaseException( const string &s )
			    : DODSBasicException( s ) {}
    virtual		~OPeNDAPDatabaseException() {}
};  

#endif // OPeNDAPDatabaseException

// $Log: OPeNDAPDatabaseException.h,v $
