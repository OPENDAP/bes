// BESXMLInfo.cc

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

#ifdef __GNUG__
#pragma implementation
#endif

#include <sstream>

using std::ostringstream ;

#include "BESXMLInfo.h"

/** @brief constructs an html information response object.
 *
 * Uses the default OPeNDAP.Info.Buffered key in the dods initialization file to
 * determine whether the information should be buffered or not.
 *
 * @see BESInfo
 * @see DODSResponseObject
 */
BESXMLInfo::BESXMLInfo( const string &buffer_key, ObjectType otype )
    : BESInfo( buffer_key, otype )
{
}

/** @brief constructs an html information response object.
 *
 * Uses the default OPeNDAP.Info.Buffered key in the dods initialization file to
 * determine whether the information should be buffered or not.
 *
 * @param is_http whether the response is going to a browser
 * @see BESInfo
 * @see DODSResponseObject
 */
BESXMLInfo::BESXMLInfo( bool is_http,
                          const string &buffer_key,
			  ObjectType otype )
    : BESInfo( is_http, buffer_key, otype )
{
}

BESXMLInfo::~BESXMLInfo()
{
}

/** @brief add exception data to this informational object. If buffering is
 * not set then the information is output directly to the output stream.
 *
 * @param type type of the exception received
 * @param msg the error message
 * @param file file name of where the error was sent
 * @param line line number in the file where the error was sent
 */
void
BESXMLInfo::add_exception( const string &type, const string &msg,
                            const string &file, int line )
{
    add_data( "<BESException>\n" ) ;
    add_data( (string)"    <Type>" + type + "</Type>\n" ) ;
    add_data( (string)"    <Message>" + msg + "</Message>\n" ) ;
    ostringstream s ;
    s << "    <Location>Filename: " << file << " LineNumber: " << line << "</Location>\n" ;
    add_data( s.str() ) ;
    add_data( "</BESException>\n" ) ;
}

// $Log: BESXMLInfo.cc,v $
