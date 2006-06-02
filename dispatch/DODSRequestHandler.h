// DODSRequestHandler.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#ifndef I_DODSRequestHandler_h
#define I_DODSRequestHandler_h 1

#include <map>
#include <string>

using std::map ;
using std::string ;

#include "DODSDataHandlerInterface.h"

typedef bool (*p_request_handler)(DODSDataHandlerInterface &);

/** @brief Represents a specific data type request handler
 *
 * A request handler is an object that represents a specific data type. The
 * specific data type knows how to fill in a response object, such as a DAS,
 * DDS, help, version, etc... response object.
 *
 * The response handlers know how to build the specified response object, and
 * the request handler knows how to fill them in.
 *
 * Each container in the DODSDataHandlerInterface has an associated data type
 * for that data continer, such as Cedar, NetCDF, CDF, HDF, etc... Usually, in
 * a given request, only one data type is requested. In other words, at least
 * currently, it is rare to see a request asking for information from more
 * than one type of data.
 *
 * Each data request handler is registered with the server. When a request
 * comes in, the request handler is looked up for each of those data types and
 * is passed the information it needs to fill in the specified response
 * object.
 *
 * Each request handler can handle different types of response objects.
 * Methods are registered within the request handler for the responses that
 * the request handler can fill in. This method is looked up and is passed the
 * information to fill in the response object.
 */
class DODSRequestHandler {
private:
    map< string, p_request_handler > _handler_list ;
    string			_name ;
public:
				DODSRequestHandler( string name )
				    : _name( name ) {}
    virtual			~DODSRequestHandler(void) {}

    typedef map< string, p_request_handler >::const_iterator Handler_citer ;
    typedef map< string, p_request_handler >::iterator Handler_iter ;

    virtual string		get_name( ) { return _name ; }

    virtual bool		add_handler( string handler_name,
					    p_request_handler handler_method ) ;
    virtual bool		remove_handler( string handler_name ) ;
    virtual p_request_handler	find_handler( string handler_name ) ;

    virtual string		get_handler_names() ;
};

#endif // I_DODSRequestHandler_h

// $Log: DODSRequestHandler.h,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
