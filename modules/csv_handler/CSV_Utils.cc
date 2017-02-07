// CSV_Utils.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
// Author: Stephan Zednik <zednik@ucar.edu> and Patrick West <pwest@ucar.edu>
// and Jose Garcia <jgarcia@ucar.edu>
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
//	zednik      Stephan Zednik <zednik@ucar.edu>
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <list>

#include "CSV_Utils.h"

#include <BESUtil.h>

/** @brief Splits a string into separate strings based on the delimiter
 *
 * @param str The string to break apart into different tokens
 * @param delimiters Contains the delimiters to use to break apart the string
 * @param tokens The resulting vector of tokens
 */
void
CSV_Utils::split( const string& str, char delimiter,
                 vector<string> &tokens )
{
    /*
    string::size_type lastPos = str.find_first_not_of( delimiters, 0 ) ;
    string::size_type pos = str.find_first_not_of( delimiters, lastPos ) ;

    while( string::npos != pos || string::npos != lastPos )
    {
	if(lastPos != pos)
	{
	    string token = str.substr( lastPos, pos - lastPos ) ;
	    tokens.push_back( token ) ;
	}
	lastPos = str.find_first_not_of( delimiters, pos ) ;
	pos = str.find_first_of( delimiters, lastPos ) ;
    }
    */
    if( !str.empty() )
    {
	list<string> tmplist ;
	BESUtil::explode( delimiter, str, tmplist ) ;
	list<string>::iterator i = tmplist.begin() ;
	list<string>::iterator e = tmplist.end() ;
	for( ; i != e; i++ )
	{
	    tokens.push_back( (*i) ) ;
	}
    }
}

/** @brief Strips leading and trailing double quotes from string
 *
 * There must be a leading and trailing quote for them to be removed. If
 * there is just one or the other, then the double quote is left.
 *
 * @param str string to remove leading and trailing double quotes from
 */
void
CSV_Utils::slim( string& str )
{
    if( *(--str.end()) == '\"' and *str.begin() == '\"' )
	str = str.substr( 1, str.length() - 2 ) ;
}

