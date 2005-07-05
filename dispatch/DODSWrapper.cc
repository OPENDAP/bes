// DODSWrapper.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include <string>

using std::string ;

#include "DODSWrapper.h"
#include "DODSApache.h"
#include "DODSProcessEncodedString.h"

DODSWrapper::DODSWrapper()
{
    _data_request = 0 ;
}

DODSWrapper::~DODSWrapper()
{
    if( _data_request )
    {
	delete [] _data_request ;
	_data_request = 0 ;
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

/** @brief Find the request from the URL and convert it to readable format

    @param s URL to convert into an OpenDAP request
    @return Resulting OpenDAP request string
 */
const char *
DODSWrapper::process_request(const char*s)
{
    DODSProcessEncodedString h( s ) ;
    string str = h.get_key( "request" ) ;
    _data_request = new char[strlen( str.c_str() ) + 1] ;
    strcpy( _data_request, str.c_str() ) ;
    return _data_request ;
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
