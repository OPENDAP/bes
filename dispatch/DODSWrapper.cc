// DODSWrapper.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include <string>

using std::string ;

#include "DODSWrapper.h"
#include "DODSApache.h"
#include "DODSProcessEncodedString.h"

DODSWrapper::DODSWrapper()
    : _encoder( 0 )
{
}

DODSWrapper::~DODSWrapper()
{
    if( _encoder )
    {
	delete _encoder ;
	_encoder = 0 ;
    }
}

/** @brief Execute the given request using DODSApache interface

    @param re DODSDataRequestInterface filled in by the apache module holding
    the request information.
    @return Whether the request was successful or not
    @see _DODSDataRequestInterface
 */
int
DODSWrapper::call_DODS( const DODSDataRequestInterface & re )
{
    DODSApache dods( re ) ;
    int ret = dods.execute_request() ;
    return ret ;
}

/** @brief Parse the command section of the given URL and convert it to
 * readable format

 * @param s URL to parse and convert the different commands to readable
 * format
 */
void
DODSWrapper::process_commands( const char *s )
{
    if( !_encoder )
	_encoder = new DODSProcessEncodedString( s ) ;
}

/** @brief Find the specified command from the URL and convert it to readable
 * format
 *
 * There can be many different requests given to a server, such as setting the
 * user name, creating a container, creating a definition, etc... Find the
 * desired command from the URL.
 *
 * @param s command string to find in the URL
 * @return the value of the specified command string
 */
const char *
DODSWrapper::get_command( const char *s )
{
    char *command = 0 ;
    if( _encoder )
    {
	string str = _encoder->get_key( s ) ;
	char *command = new char[strlen( str.c_str() ) + 1] ;
	strcpy( command, str.c_str() ) ;
    }
    return command ;
}

// $Log: DODSWrapper.cc,v $
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
