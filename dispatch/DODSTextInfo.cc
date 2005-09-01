// DODSTextInfo.cc

// 2004 Copyright University Corporation for Atmospheric Research

#ifdef __GNUG__
#pragma implementation
#endif

#include "DODSTextInfo.h"

/** @brief constructs a basic text information response object.
 *
 * Uses the default DODS.Info.Buffered key in the dods initialization file to
 * determine whether the information should be buffered or not.
 *
 * @see DODSInfo
 * @see DODSResponseObject
 */
DODSTextInfo::DODSTextInfo()
    : DODSInfo( unknown_type )
{
    initialize( "" ) ;
}

/** @brief constructs a basic text information response object.
 *
 * Uses the default DODS.Info.Buffered key in the dods initialization file to
 * determine whether the information should be buffered or not.
 *
 * @param is_http whether the response is going to a browser
 * @see DODSInfo
 * @see DODSResponseObject
 */
DODSTextInfo::DODSTextInfo( bool is_http )
    : DODSInfo( is_http, unknown_type )
{
    initialize( "" ) ;
}

DODSTextInfo::~DODSTextInfo()
{
}

// $Log: DODSTextInfo.cc,v $
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
