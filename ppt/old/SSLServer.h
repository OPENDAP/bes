// SSLServer.h

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

#ifndef SSLServer_h
#define SSLServer_hS 1

#include <openssl/ssl.h>

#include <string>

using std::string ;

#include "SSLConnection.h"

class Socket ;

class SSLServer : public SSLConnection
{
private:
    int				_port ;
    string			_cfile ;
    string			_cafile ;
    string			_kfile ;

    int				verify_connection( ) ;
    int				open_port( ) ;

    static int			verify_client( int ok, X509_STORE_CTX *ctx ) ;
public:
    				SSLServer( int portVal,
					   const string &cert_file,
					   const string &cert_auth_file,
					   const string &key_file );
    				~SSLServer() ;
    virtual void		initConnection() ;

    virtual void		dump( ostream &strm ) const ;
} ;

#endif // SSLServer_h

