// BESInfo.cc

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

#include <sstream>
#include <iostream>
#include <fstream>

using std::ostringstream ;
using std::ifstream ;

#include "BESInfo.h"
#include "TheBESKeys.h"
#include "BESHandlerException.h"

/** @brief constructs a BESInfo object
 *
 * By default, informational responses are buffered, so the output stream is
 * created
 */
BESInfo::BESInfo( )
    : _strm( 0 ),
      _buffered( true )
{
    _strm = new ostringstream ;
}

/** @brief constructs a BESInfo object
 *
 * If the passed key is set to true, True, TRUE, yes, Yes, or YES then the
 * information will be buffered, otherwise it will not be buffered.
 *
 * If the information is not to be buffered then the output stream is set to
 * standard output.
 */
BESInfo::BESInfo( const string &key )
    : _strm( 0 ),
      _buffered( true )
{
    bool found = false ;
    string b = TheBESKeys::TheKeys()->get_key( key, found ) ;
    if( b == "true" || b == "True" || b == "TRUE" ||
	b == "yes" || b == "Yes" || b == "YES" )
    {
	_strm = new ostringstream ;
	_buffered = true ;
    }
    else
    {
	_buffered = false ;
    }
}

BESInfo::~BESInfo()
{
    if( _buffered && _strm ) delete _strm ;
}

void
BESInfo::begin_response( const string &response_name )
{
    _response_started = true ;
    _response_name = response_name ;
}

void
BESInfo::end_response( )
{
    _response_started = false ;
    if( _tags.size() )
    {
	string s = "Not all tags were ended in info response" ;
	throw BESHandlerException( s, __FILE__, __LINE__ ) ;
    }
}

void
BESInfo::begin_tag( const string &tag_name,
		    map<string,string> *attrs )
{
    _tags.push( tag_name ) ;
}

void
BESInfo::end_tag( const string &tag_name )
{
    if( _tags.size() == 0 || _tags.top() != tag_name )
    {
	string s = (string)"tag " + tag_name
	           + " alreaded ended or not started" ;
	throw BESHandlerException( s, __FILE__, __LINE__ ) ;
    }
    else
    {
	_tags.pop() ;
    }
}

/** @brief add data to this informational object. If buffering is not set then
 * the information is output directly to the output stream.
 *
 * @param s information to be added to this informational response object
 */
void
BESInfo::add_data( const string &s )
{
    if( !_buffered )
    {
	fprintf( stdout, "%s", s.c_str() ) ;
    }
    else
    {
	(*_strm) << s ;
    }
}

/** @brief add data from a file to the informational object.
 *
 * Adds data from a file to the informational object using the file
 * specified by the passed key string. The key is found from the bes
 * configuration file.
 *
 * If the key does not exist in the initialization file then this
 * information is added to the informational object, no excetion is thrown.
 *
 * If the file does not exist then this information is added to the
 * informational object, no exception is thrown.
 *
 * @param key Key from the initialization file specifying the file to be
 * @param name A description of what is the information being loaded
 */
void
BESInfo::add_data_from_file( const string &key, const string &name )
{
    bool found = false ;
    string file = TheBESKeys::TheKeys()->get_key( key, found ) ;
    if( found == false )
    {
	add_data( name + " file key " + key + " not found, information not available\n" ) ;
    }
    else
    {
	ifstream ifs( file.c_str() ) ;
	if( !ifs )
	{
	    add_data( name + " file " + file + "not found, information not available\n" ) ;
	}
	else
	{
	    char line[4096] ;
	    while( !ifs.eof() )
	    {
		ifs.getline( line, 4096 ) ;
		if( !ifs.eof() )
		{
		    add_data( line ) ;
		    add_data( "\n" ) ;
		}
	    }
	    ifs.close() ;
	}
    }
}

/** @brief add exception information to this informational object
 *
 * Exception information is added differently to different informational
 * objects, such as html, xml, plain text. But, using the other methods of
 * this class we can take care of exceptions here.
 *
 * @param type The type of exception being thrown
 * @param e The exception to add to the informational response object
 */
void
BESInfo::add_exception( const string &type, BESException &e )
{
    begin_tag( "BESException" ) ;
    add_tag( "Type", type ) ;
    add_tag( "Message", e.get_message() ) ;
    begin_tag( "Location" ) ;
    add_tag( "File", e.get_file() ) ;
    ostringstream sline ;
    sline << e.get_line() ;
    add_tag( "Line", sline.str() ) ;
    end_tag( "Location" ) ;
    end_tag( "BESException" ) ;
}

/** @brief print the information from this informational object to the
 * specified FILE descriptor
 *
 * If the information was not buffered then this method does nothing,
 * otherwise the information is output to the specified FILE descriptor.
 *
 * @param out output to this file descriptor if information buffered.
 */
void
BESInfo::print(FILE *out)
{
    if( _buffered )
    {
	fprintf( out, "%s", ((ostringstream *)_strm)->str().c_str() ) ;
    }
}

