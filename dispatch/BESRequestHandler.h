// BESRequestHandler.h

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

#ifndef I_BESRequestHandler_h
#define I_BESRequestHandler_h 1

#include <map>
#include <string>

using std::map ;
using std::string ;

#include "BESObj.h"
#include "BESDataHandlerInterface.h"

typedef bool (*p_request_handler)(BESDataHandlerInterface &);
#define BES_REQUEST_HANDLER_CATCH_ALL "catch_all"

/** @brief Represents a specific data type request handler
 *
 * A request handler is an object that represents a specific data type. The
 * specific data type knows how to fill in a response object, such as a DAS,
 * DDS, help, version, etc... response object.
 *
 * The response handlers know how to build the specified response object, and
 * the request handler knows how to fill them in.
 *
 * Each container in the BESDataHandlerInterface has an associated data type
 * for that data container, such as Cedar, NetCDF, CDF, HDF, etc... Usually, in
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
class BESRequestHandler : public BESObj
{
private:
    map< string, p_request_handler > _handler_list ;
    string			_name ;
public:
				BESRequestHandler( const string &name )
				    : _name( name ) {}
    virtual			~BESRequestHandler(void) {}

    typedef map< string, p_request_handler >::const_iterator Handler_citer ;
    typedef map< string, p_request_handler >::iterator Handler_iter ;

    virtual const string &	get_name( ) const { return _name ; }

    virtual bool		add_handler( const string &handler_name,
					    p_request_handler handler_method ) ;
    virtual bool		remove_handler( const string &handler_name ) ;
    virtual p_request_handler	find_handler( const string &handler_name ) ;

    virtual string		get_handler_names() ;

    virtual void		dump( ostream &strm ) const ;
};

#endif // I_BESRequestHandler_h

