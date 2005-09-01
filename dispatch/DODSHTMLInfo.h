// DODSHTMLInfo.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSHTMLInfo_h_
#define DODSHTMLInfo_h_ 1

#include "DODSInfo.h"

/** @brief represents an html formatted response object
 *
 * Uses the default add_data method, but overwrites print method in order to
 * set the mime type to html.
 *
 * @see DODSInfo
 * @see DODSResponseObject
 */
class DODSHTMLInfo : public DODSInfo {
public:
  			DODSHTMLInfo() ;
  			DODSHTMLInfo( bool is_http ) ;
    virtual 		~DODSHTMLInfo() ;
};

#endif // DODSHTMLInfo_h_

// $Log: DODSHTMLInfo.h,v $
// Revision 1.4  2005/04/19 17:58:52  pwest
// print of an html information object must include the header
//
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
