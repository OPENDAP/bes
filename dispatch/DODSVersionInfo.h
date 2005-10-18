// DODSVersionInfo.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSVersionInfo_h_
#define DODSVersionInfo_h_ 1

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
class DODSVersionInfo : public DODSInfo {
private:
    bool		_firstDAPVersion ;
    ostream		*_DAPstrm ;
    bool		_firstBESVersion ;
    ostream		*_BESstrm ;
    bool		_firstHandlerVersion ;
    ostream		*_Handlerstrm ;
public:
  			DODSVersionInfo() ;
  			DODSVersionInfo( bool is_http ) ;
    virtual 		~DODSVersionInfo() ;

    virtual void 	print( FILE *out ) ;

    virtual void	addDAPVersion( const string &v ) ;
    virtual void	addBESVersion( const string &n, const string &v ) ;
    virtual void	addHandlerVersion( const string &n, const string &v ) ;
};

#endif // DODSVersionInfo_h_

// $Log: DODSVersionInfo.h,v $
