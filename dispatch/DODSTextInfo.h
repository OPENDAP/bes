// DODSTextInfo.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSTextInfo_h_
#define DODSTextInfo_h_ 1

#include "DODSInfo.h"

/** brief represents simple text information in a response object, such as
 * version and help inforamtion.
 *
 * Uses the default add_data and print methods, where the print method, if the
 * response is going to a browser, sets the mime type to text.
 *
 * @see DODSInfo
 * @see DODSResponseObject
 */
class DODSTextInfo : public DODSInfo {
public:
  			DODSTextInfo() ;
  			DODSTextInfo( bool is_http ) ;
    virtual 		~DODSTextInfo() ;
};

#endif // DODSTextInfo_h_

// $Log: DODSTextInfo.h,v $
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
