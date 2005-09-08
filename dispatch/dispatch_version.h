// dispatch_version.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef E_dispatch_version_h
#define E_dispatch_version_h 1

#include <string>

using std::string ;

inline string
dispatch_version()
{
    return (string)PACKAGE_STRING + ": compiled on " + __DATE__ + ":" + __TIME__ ;
}

#endif // E_dispatch_version_h

// $Log: dispatch_version.h,v $
// Revision 1.5  2005/04/26 20:57:58  pwest
// added ': ' to version information before compile time
//
// Revision 1.4  2005/04/26 20:50:12  pwest
// updated versioning information
//
// Revision 1.3  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
