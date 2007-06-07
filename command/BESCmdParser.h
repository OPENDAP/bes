// BESCmdParser.h

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
 
// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#ifndef BESCmdParser_h_
#define BESCmdParser_h_ 1

#include <string>

#include "BESDataHandlerInterface.h"

using std::string ;

/** @brief parses an incoming request and creates the information necessary to
 * carry out the request.
 *
 * Parses the incoming request, retrieving first the type of response object
 * that is being requested and passing off the parsing of the request to that
 * response handler. For example, if a "get" request is being sent, the parser
 * parses the string "get", locates the response handler that handes a "get"
 * request, and hands off the parsing of the ramaining request string to
 * that response handler.
 *
 * First, the parser builds the list of tokens using the BESTokernizer
 * object. This list of tokens is then passed to the response handler to
 * parse the remainder of the request.
 *
 * All requests must end with a semicolon.
 *
 * @see BESTokenizer
 * @see BESParserException
 */
class BESCmdParser
{
public:
    				BESCmdParser() ;
    				~BESCmdParser();

    static void			parse( const string &,
                                       BESDataHandlerInterface & ) ;
} ;

#endif // BESCmdParser_h_

