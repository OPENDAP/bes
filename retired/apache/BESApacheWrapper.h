// BESApacheWrapper.h

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

#ifndef BESApacheWrapper_h_
#define BESApacheWrapper_h_ 1

/** @brief Wrapper class to OpenDAP for Apache Modules.

    @see _BESDataRequestInterface
*/

#include "BESDataRequestInterface.h"

class BESApacheRequests ;

class BESApacheWrapper
{
    char *		_data_request ;
    char *		_user_name ;
    char *		_token ;
    BESApacheRequests	*_requests ;
public:
    			BESApacheWrapper() ;
    			~BESApacheWrapper() ;

    int			call_BES( const BESDataRequestInterface &re ) ;
    void		process_request( const char *s ) ;
    const char *	get_first_request() ;
    const char *	get_next_request() ;
    const char *	process_user( const char *s ) ;
    const char *	process_token( const char *s ) ;
};

#endif // BESApacheWrapper_h_

// $Log: BESApacheWrapper.h,v $
