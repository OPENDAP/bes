// DODSAuthenticate.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef A_DODSAuthenticate_h
#define A_DODSAuthenticate_h 1

#include "DODSDataHandlerInterface.h"

#define AUTH_USERS_TIMEOUT 15

/** @brief Authenticate the user of the OpenDAP server
 *
 * DODSAuthenticate facilitates the authentication of the OpenDAP user.
 * Authentication occurs by ensuring that the user specified in the
 * DODSDataHandlerInterface currently has a session open. The session is
 * created during user login, which is handled by the client interface.
 *
 * The method of authentication used is determined on the link line. There is
 * a global object defined called TheDODSAuthenticator. This global object is
 * defined in files, currently, test_authenticator.cc (which defines it as
 * DODSTestAuthenticate) and mysql_authenticator.cc (which defines it as
 * DODSMySQLAuthenticate.) Depending on which object module gets linked into
 * the application determines the method of authentication. We also have
 * defined a key in the ini file called "DODS.AUTHENTICATE" which can be set
 * to either MySQL or none. To use this method link in the object module
 * ini_authenticator.o.
 *
 * DODS requires that a method for authenticating be specified, even if just
 * using the DODSTestAuthenticate method, which automatially authenticates the
 * user.
 *
 * @see _DODSDataHandlerInterface
 * @see DODSMySQLAuthenticate
 * @see DODSTestAuthenticate
 */
class DODSAuthenticate
{
public:
    virtual		~DODSAuthenticate() {} ;
    /** @brief Abstract method used to authenticate the user specified in the
     * DODSDataHandlerInterface.
     *
     * @param dhi Data provided throughout the request execution that holds
     * the user information.
     * @throws DODSAuthenticateException if unable to authenticate the user,
     * otherwise authentication successful.
     * @see _DODSDataHandlerInterface
     */
    virtual void	authenticate( DODSDataHandlerInterface &dhi ) = 0 ;
} ;

#endif // A_DODSAuthenticate_h

// $Log: DODSAuthenticate.h,v $
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
