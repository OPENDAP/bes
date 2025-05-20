// SSLClient.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include "config.h"

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/socket.h> // for socket connect
#include <netinet/in.h> // for inet_addr
#include <arpa/inet.h>  // for htons
#include <netdb.h>      // for gethostbyname
#include <ctype.h>      // for isdigit

#include <cstring>
#include <iostream>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

using std::endl ;

#include "SSLClient.h"
#include "BESInternalError.h"
#include "BESDebug.h"

SSLClient::SSLClient( const string &hostStr, int portVal,
                      const string &cert_file, const string &cert_auth_file,
		      const string &key_file )
    : SSLConnection(),
      _host( hostStr ),
      _port( portVal ),
      _cfile( cert_file ),
      _cafile( cert_auth_file),
      _kfile( key_file )
{
}

SSLClient::~SSLClient()
{
}

void
SSLClient::initConnection()
{
    BESDEBUG( "ppt", "Loading SSL error strings ... " << endl ) ;
    SSL_load_error_strings() ;
    BESDEBUG( "ppt", "OK" << endl ) ;

    BESDEBUG( "ppt", "Initializing SSL library ... " << endl ) ;
    SSL_library_init() ;
    BESDEBUG( "ppt", "OK" << endl ) ;

    SSL_METHOD *method = NULL ;
    SSL_CTX *context = NULL ;

    BESDEBUG( "ppt", "Creating method and context ... " << endl ) ;
    method = SSLv3_client_method() ;
    if( method )
    {
	context = SSL_CTX_new( method ) ;
    }
    if( !context )
    {
	string msg = "Failed to create SSL context\n" ;
	msg += ERR_error_string( ERR_get_error(), NULL ) ;
	throw BESInternalError( msg, __FILE__, __LINE__ ) ;
    }
    else
    {
	BESDEBUG( "ppt", "OK" << endl ) ;
    }

    bool ok_2_continue = false ;
    string err_msg ;
    BESDEBUG( "ppt", "Setting certificate and key ... " << endl ) ;
    if( SSL_CTX_use_certificate_file( context, _cfile.c_str(), SSL_FILETYPE_PEM ) <= 0 )
    {
	err_msg = "FAILED to use certificate file " + _cfile + "\n" ;
	err_msg += ERR_error_string( ERR_get_error(), NULL ) ;
    }
    else if( SSL_CTX_use_PrivateKey_file( context, _kfile.c_str(), SSL_FILETYPE_PEM ) <= 0 )
    {
	err_msg = "FAILED to use private key file " + _kfile + "\n" ;
	err_msg += ERR_error_string( ERR_get_error(), NULL ) ;
    }
    else if( !SSL_CTX_check_private_key( context ) )
    {
	err_msg = "FAILED to authenticate private key\n" ;
	err_msg += ERR_error_string( ERR_get_error(), NULL ) ;
    }
    else
    {
	ok_2_continue = true ;
    }

    if( ok_2_continue )
    {
	BESDEBUG( "ppt", "OK" << endl ) ;
	BESDEBUG( "ppt", "Certificate setup ... " << endl ) ;
	SSL_CTX_set_verify( context, SSL_VERIFY_PEER, SSLClient::verify_server ) ;
	SSL_CTX_set_client_CA_list( context, SSL_load_client_CA_file( _cafile.c_str() ));
	if( ( !SSL_CTX_load_verify_locations( context, _cafile.c_str(), NULL )) ||
	    ( !SSL_CTX_set_default_verify_paths( context ) ) )
	{
	    err_msg = "Certificate setup failed\n" ;
	    err_msg += ERR_error_string( ERR_get_error(), NULL ) ;
	    ok_2_continue = false ;
	}
    }

    int sock_fd = -1 ;
    if( ok_2_continue )
    {
	BESDEBUG( "ppt", "OK" << endl ) ;

	BESDEBUG( "ppt", "Establishing TCP connection to "
			 << _host << ":" << _port << " ... " << endl ) ;
	sock_fd = connect_to_server() ;
	if( sock_fd < 0 )
	{
	    err_msg = "Failed to establish TCP connection" ;
	    ok_2_continue = false ;
	}
    }

    if( ok_2_continue )
    {
	BESDEBUG( "ppt", "OK" << endl ) ;

	BESDEBUG( "ppt", "Establishing secure connection ... " << endl ) ;
	int ssl_ret = 0 ;
	_connection = SSL_new( context ) ;
	if( !_connection )
	{
	    err_msg =  "FAILED to create new connection\n" ;
	    err_msg += ERR_error_string( ERR_get_error(), NULL ) ;
	    ok_2_continue = false ;
	}
	else if( ( ssl_ret = SSL_set_fd( _connection, sock_fd ) )  < 0 )
	{
	    err_msg = "FAILED to set the socket descriptor\n" ;
	    err_msg += ERR_error_string( ERR_get_error(), NULL ) ;
	    ok_2_continue = false ;
	}
	else if( ( ssl_ret = SSL_connect( _connection ) ) < 0 )
	{
	    err_msg = "FAILED to create SSL connection\n" ;
	    err_msg += ERR_error_string( SSL_get_error( _connection, ssl_ret ), NULL ) ;
	    ok_2_continue = false ;
	}
	else if( verify_connection() < 0 )
	{
	    err_msg = "FAILED to verify SSL connection\n" ;
	    err_msg += ERR_error_string( ERR_get_error(), NULL ) ;
	    ok_2_continue = false ;
	}
    }

    if( ok_2_continue )
    {
	BESDEBUG( "ppt", "OK" << endl ) ;
    }
    else
    {
	BESDEBUG( "ppt", "FAILED" << endl ) ;
	if( _context ) SSL_CTX_free( _context ) ; _context = NULL ;
	throw BESInternalError( err_msg, __FILE__, __LINE__ ) ;
    }

    _connected = true ;
}

int
SSLClient::connect_to_server( )
{
    int fd = -1 ;
    struct sockaddr_in addr ;

    fd = socket( PF_INET, SOCK_STREAM, 0 ) ;
    if( fd < 0 ) return -1 ;

    memset( &addr, 0, sizeof( addr ) ) ;
    addr.sin_family = AF_INET ;
    addr.sin_port = htons( _port ) ;
    if( isdigit( (int)*_host.c_str() ) )
    {
	addr.sin_addr.s_addr = inet_addr( _host.c_str() ) ;
    }
    else
    {
	struct hostent *hostEntry ;
	if( ( hostEntry = gethostbyname( _host.c_str() ) ) != 0 )
	{
	    if ( hostEntry->h_length > sizeof(addr.sin_addr) ) {
	    	close( fd );
	        throw BESInternalError("Memory exception.", __FILE__, __LINE__);
	    }
	    memcpy( &addr.sin_addr, hostEntry->h_addr, hostEntry->h_length ) ;
	}
	else
	{
	    close( fd ) ;
	    return -1 ;
	}
    }
    if( connect( fd, (struct sockaddr *)&addr, sizeof( addr ) ) < 0 )
    {
	close( fd ) ;
	return -1 ;
    }

    return fd ;
}

int
SSLClient::verify_connection( )
{
    X509 *server_cert = NULL ;
    char *str = NULL ;

    /*
    server_cert = SSL_get_peer_certificate( _connection ) ;
    if( server_cert == NULL )
    {
	cout << "server doesn't have a certificate" << endl ;
    }
    */

    return 1 ;
}

int
SSLClient::verify_server( int ok, X509_STORE_CTX *ctx )
{
    if( ok )
    {
	BESDEBUG( "ppt", "VERIFIED " << endl ) ;
    }
    else
    {
	char mybuf[256] ;
	X509 *err_cert ;
	int err ;

	err_cert = X509_STORE_CTX_get_current_cert( ctx ) ;
	err = X509_STORE_CTX_get_error( ctx ) ;
	X509_NAME_oneline( X509_get_subject_name( err_cert ), mybuf, 256 ) ;
	BESDEBUG( "ppt", "FAILED for " << mybuf << endl ) ;
	BESDEBUG( "ppt", "  " << X509_verify_cert_error_string( err )
			 << endl ) ;
	switch( ctx->error )
	{
	    case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
	    {
		X509_NAME_oneline( X509_get_issuer_name( err_cert ), mybuf, 256 ) ;
		BESDEBUG( "ppt", "  issuer = " << mybuf << endl ) ;
		break ;
	    }

	    case X509_V_ERR_CERT_NOT_YET_VALID:
	    case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
	    {
		BESDEBUG( "ppt", "  not yet valid!" << endl ) ;
		break ;
	    }

	    case X509_V_ERR_CERT_HAS_EXPIRED:
	    case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
	    {
		BESDEBUG( "ppt", "  expired!" << endl ) ;
		break ;
	    }
	}
    }

    return 1 ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
SSLClient::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "SSLClient::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "host: " << _host << endl ;
    strm << BESIndent::LMarg << "port: " << _port << endl ;
    strm << BESIndent::LMarg << "cert file: " << _cfile << endl ;
    strm << BESIndent::LMarg << "cert authority file: " << _cafile << endl ;
    strm << BESIndent::LMarg << "key file: " << _kfile << endl ;
    SSLConnection::dump( strm ) ;
    BESIndent::UnIndent() ;
}

