// DODSMySQLAuthenticate.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include <time.h>

#include "DODSMySQLAuthenticate.h"
#include "DODSMySQLQuery.h"
#include "DODSMemoryException.h"
#include "TheDODSKeys.h"
#include "DODSAuthenticateException.h"

/** @brief Constructor makes connection to the MySQL database and initializes
 * authentication using MySQL.
 *
 * This default constructor reads the key/value pairs from the dods
 * initiailization file using TheDODSKeys in order to make a connection to the
 * MySQL database and to determine if authentication is turned on or off. The
 * key/value pairs used from the intialization file are:
 *
 * DODS.Authenticate.MySQL.server=&lt;serverName&gt;
 * DODS.Authenticate.MySQL.user=&lt;userName&gt;
 * DODS.Authenticate.MySQL.password=&lt;encryptedPassword&gt;
 * DODS.Authenticate.MySQL.database=&lt;databaseName&gt;
 * DODS.Authenticate.MySQL.mode=&lt;on|off&gt;
 *
 * @throws DODSAuthenticateException if unable to read information from
 * initialization file.
 * @throws DODSMySQLConnectException if unable to connect to the MySQL
 * database.
 * @see DODSAuthenticate
 * @see DODSAuthenticateException
 * @see DODSMySQLConnectException
 * @see DODSKeys
 */
DODSMySQLAuthenticate::DODSMySQLAuthenticate()
{
    bool found = false ;
    string my_key = "DODS.Authenticate.MySQL." ;

    string my_server = TheDODSKeys->get_key( my_key + "server", found ) ;
    if( found == false )
    {
	DODSAuthenticateException pe ;
	pe.set_error_description( "MySQL server not specified for authentication" ) ;
	throw pe ;
    }

    string my_user = TheDODSKeys->get_key( my_key + "user", found  ) ;
    if( found == false )
    {
	DODSAuthenticateException pe ;
	pe.set_error_description( "MySQL user not specified for authentication" ) ;
	throw pe ;
    }

    string my_password = TheDODSKeys->get_key( my_key + "password", found  ) ;
    if( found == false )
    {
	DODSAuthenticateException pe ;
	pe.set_error_description( "MySQL password not specified for authentication" ) ;
	throw pe ;
    }

    string my_database=TheDODSKeys->get_key( my_key + "database", found ) ;
    if( found == false )
    {
	DODSAuthenticateException pe ;
	pe.set_error_description( "MySQL database not specified for authentication" ) ;
	throw pe ;
    }
    
    string authentication_mode=TheDODSKeys->get_key( my_key + "mode", found ) ;
    if( found == false )
    {
	DODSAuthenticateException pe ;
	pe.set_error_description( "Authentication mode (on/off) is not specified" ) ;
	throw pe ;
    }
    else
    {
	if( authentication_mode == "on" )
	    _enforce_authentication = true ;
	else
	{
	    if( authentication_mode == "off" )
		_enforce_authentication = false ;
	    else
	    {
		DODSAuthenticateException pe ;
		pe.set_error_description( "Authentication mode is neither on nor off" ) ;
		throw pe ;
	    }
	}
    }

    try
    {
	_query = new DODSMySQLQuery( my_server, my_user,
				     my_password, my_database ) ;
    }
    catch( bad_alloc::bad_alloc )
    {
	DODSMemoryException ex ;
	ex.set_error_description( "Can not get memory for Persistence object" );
	ex.set_amount_of_memory_required( sizeof( DODSMySQLQuery ) ) ;
	throw ex ;
    }
}

/** @brief closes the connection to the MySQL database if one was created
 */
DODSMySQLAuthenticate::~DODSMySQLAuthenticate()
{
    if( _query ) delete _query ;
    _query =0 ;
}

/** @brief authenticates the user against the MySQL database.
 *
 * This method authenticates the user specifie3d in the
 * DODSDataHandlerInterface using a MySQL database. The table tbl_session in
 * the MySQL database is used to authenticate the user. The session
 * information must be created prior to this method being called.
 *
 * If DODS.Authenticate.MySQL.mode is set to off then the user is
 * automatically authenticated.
 *
 * If authentication is not successful then an exception is thrown. If
 * successful then processing continues.
 *
 * @param dhi contains the user name of the user to be authenticated
 * @throws DODSAuthenticateException if unable to authenticate the user
 * @see _DODSDataHandlerInterface
 * @see DODSAuthenticateExcpetion
 * @see DODSMySQLQuery
 */
void
DODSMySQLAuthenticate::authenticate( DODSDataHandlerInterface &dhi )
{
    if(_enforce_authentication)
    {
	// get the current date and time
	string query = "select USER_NAME from tbl_sessions " ;
	query += "where USER_NAME=\"" + dhi.user_name + "\";" ;
	_query->run_query( query ) ;
	if( !_query->is_empty_set() )
	{
	    if( (_query->get_nfields() != 1) )
	    {
		char err_str[256] ;
		sprintf( err_str, "%s %s\n%s: %d rows and %d fields returned",
			 "Unable to authenticate user",
			 dhi.user_name.c_str(),
			 "Invalid data from MySQL",
			 _query->get_nrows(),
			 _query->get_nfields() ) ;
		DODSAuthenticateException pe ;
		pe.set_error_description( err_str ) ;
		throw pe ;
	    }
	}
	else
	{
	    char err_str[256] ;
	    sprintf( err_str, "%s %s",
		     "Unable to authenticate user",
		     dhi.user_name.c_str() ) ;
	    DODSAuthenticateException pe ;
	    pe.set_error_description( err_str ) ;
	    throw pe ;
	}
    }
}

// $Log: DODSMySQLAuthenticate.cc,v $
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
