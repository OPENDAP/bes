// DODSTestAuthenticate.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef A_DODSTestAuthenticate_h
#define A_DODSTestAuthenticate_h 1

#include "DODSAuthenticate.h"

/** @brief No authentication is performed, user is automatically
 * authenticated.
 *
 * OpenDAP requires authentication, but for testing purpoes or in systems that
 * do not require strict authentication, this class provides a mechanism to
 * automatically pass authentication. No authentication is done.
 *
 * @see DODSAuthenticate
 * @see _DODSDataHandlerInterface
 */
class DODSTestAuthenticate : public DODSAuthenticate
{
public:
    			DODSTestAuthenticate() ;
    virtual		~DODSTestAuthenticate() ;

    virtual void	authenticate( DODSDataHandlerInterface &dhi ) ;
} ;

#endif // A_DODSTestAuthenticate_h

// $Log: DODSTestAuthenticate.h,v $
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
