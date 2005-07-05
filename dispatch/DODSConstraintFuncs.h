// DODSConstraintFuncs.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef E_DODSConstraintFuncs_h
#define E_DODSConstraintFuncs_h 1

#include <string>

using std::string ;

#include "DODSDataHandlerInterface.h"

namespace DODSConstraintFuncs
{
    string pre_to_post_constraint( const string &name,
				   const string &pre_constraint ) ;
    void post_append( DODSDataHandlerInterface &dhi ) ;
}

#endif // E_DODSConstraintFuncs_h

// $Log: DODSConstraintFuncs.h,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
