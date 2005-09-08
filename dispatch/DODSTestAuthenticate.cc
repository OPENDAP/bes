// DODSTestAuthenticate.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "DODSTestAuthenticate.h"

DODSTestAuthenticate::DODSTestAuthenticate()
{
}

DODSTestAuthenticate::~DODSTestAuthenticate()
{
}

/** @brief No authentication is performed.
 *
 * @param dhi user information
 * @see DODSDataHandlerInterface
 */
void
DODSTestAuthenticate::authenticate( DODSDataHandlerInterface & )
{
}

// $Log: DODSTestAuthenticate.cc,v $
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
