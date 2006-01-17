// OPeNDAPCmdParser.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org>
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

#ifndef OPeNDAPCmdParser_h_
#define OPeNDAPCmdParser_h_ 1

#include <string>

#include "DODSDataHandlerInterface.h"

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
 * First, the parser builds the list of tokens using the DODSTokernizer
 * object. This list of tokens is then passed to the response handler to
 * parse the remainder of the request.
 *
 * All requests must end with a semicolon.
 *
 * @see OPeNDAPTokenizer
 * @see OPeNDAPParserException
 */
class OPeNDAPCmdParser
{
public:
    				OPeNDAPCmdParser() ;
    				~OPeNDAPCmdParser();

    void			parse( const string &,
                                       DODSDataHandlerInterface & ) ;
} ;

#endif // OPeNDAPCmdParser_h_

// $Log: OPeNDAPCmdParser.h,v $
// Revision 1.4  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
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
