// OPENDAP_TYPE_version.h

#ifndef E_OPENDAP_TYPE_version_h
#define E_OPENDAP_TYPE_version_h 1

#include <string>

using std::string ;

#include "config_OPENDAP_TYPE.h"

inline string
OPENDAP_TYPE_version()
{
    return (string)OPENDAP_CLASS_VERSION + ": compiled on " + __DATE__ + ":" + __TIME__ ;
}

#endif // E_OPENDAP_TYPE_version_h

