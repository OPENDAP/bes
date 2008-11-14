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
 
// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
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
#include "BESUtil.h"

/** @brief constructs an informational response object as an xml document
 *
 * @see BESInfo
 * @see BESResponseObject
 */
BESXMLInfo::BESXMLInfo( )
    : BESInfo( ),
      _do_indent( true )
{
    //_buffered = false ;
}

BESXMLInfo::~BESXMLInfo()
{
}

/** @brief begin the informational response
 *
 * This will add the response name as well as the &lt;response&gt; tag tot
 * he informational response object
 *
 * @param response_name name of the response this information represents
 */
void
BESXMLInfo::begin_response( const string &response_name )
{
    BESInfo::begin_response( response_name ) ;
    add_data( "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" ) ;
    _response_name = response_name ;
    add_data( (string)"<" + _response_name + ">\n" ) ;
    _indent += "    " ;
    add_data( "<response>\n" ) ;
    _indent += "    " ;
}

/** @brief end the response
 *
 * Add the terminating tags for the response and for the response name. If
 * there are still tags that have not been closed then an exception is
 * thrown.
 *
 */
void
BESXMLInfo::end_response()
{
    BESInfo::end_response() ;
    if( _indent.length() >= 4 )
	_indent = _indent.substr( 0, _indent.length()-4 ) ;
    add_data( "</response>\n" ) ;
    if( _indent.length() >= 4 )
	_indent = _indent.substr( 0, _indent.length()-4 ) ;
    add_data( (string)"</" + _response_name + ">\n" ) ;
}

/** @brief add tagged information to the inforamtional response
 *
 * @param tag_name name of the tag to be added to the response
 * @param tag_data information describing the tag
 * @param attrs map of attributes to add to the tag
 */
void
BESXMLInfo::add_tag( const string &tag_name,
		     const string &tag_data,
		     map<string,string> *attrs )
{
    add_data( (string)"<" + tag_name ) ;
    if( attrs )
    {
	map<string,string>::const_iterator i = attrs->begin() ;
	map<string,string>::const_iterator e = attrs->end() ;
	for( ; i != e; i++ )
	{
	    string name = (*i).first ;
	    string val = (*i).second ;
	    _do_indent = false ;
	    if( val != "" )
		add_data( " " + name + "=" + val ) ;
	    else
		add_data( " " + name ) ;
	}
    }
    _do_indent = false ;
    add_data( ">" + BESUtil::id2xml( tag_data ) + "</" + tag_name + ">\n" ) ;
}

/** @brief begin a tagged part of the information, information to follow
 *
 * @param tag_name name of the tag to begin
 * @param attrs map of attributes to begin the tag with
 */
void
BESXMLInfo::begin_tag( const string &tag_name,
                       map<string,string> *attrs )
{
    BESInfo::begin_tag( tag_name ) ;
    add_data( (string)"<" + tag_name ) ;
    if( attrs )
    {
	map<string,string>::const_iterator i = attrs->begin() ;
	map<string,string>::const_iterator e = attrs->end() ;
	for( ; i != e; i++ )
	{
	    string name = (*i).first ;
	    string val = (*i).second ;
	    _do_indent = false ;
	    if( val != "" )
		add_data( " " + name + "=" + val ) ;
	    else
		add_data( " " + name ) ;
	}
    }
    _do_indent = false ;
    add_data( ">\n" ) ;
    _indent += "    " ;
}

/** @brief end a tagged part of the informational response
 *
 * If the named tag is not the current tag then an error is thrown.
 *
 * @param tag_name name of the tag to end
 */
void
BESXMLInfo::end_tag( const string &tag_name )
{
    BESInfo::end_tag( tag_name ) ;
    if( _indent.length() >= 4 )
	_indent = _indent.substr( 0, _indent.length()-4 ) ;
    add_data( (string)"</" + tag_name + ">\n" ) ;
}

/** @brief add a space to the informational response
 *
 * @param num_spaces the number of spaces to add to the information
 */
void
BESXMLInfo::add_space( unsigned long num_spaces )
{
    string to_add ;
    for( unsigned long i = 0; i < num_spaces; i++ )
    {
	to_add += " " ;
    }
    _do_indent = false ;
    add_data( to_add ) ;
}

/** @brief add a line break to the information
 *
 * @param num_breaks the number of line breaks to add to the information
 */
void
BESXMLInfo::add_break( unsigned long num_breaks )
{
    string to_add ;
    for( unsigned long i = 0; i < num_breaks; i++ )
    {
	to_add += "\n" ;
    }
    _do_indent = false ;
    add_data( to_add ) ;
}

void
BESXMLInfo::add_data( const string &s )
{
    if( _do_indent )
	BESInfo::add_data( _indent + s ) ;
    else
	BESInfo::add_data( s ) ;
    _do_indent = true ;
}

/** @brief add data from a file to the informational object
 *
 * This method simply adds a .XML to the end of the key and passes the
 * request on up to the BESInfo parent class.
 *
 * @param key Key from the initialization file specifying the file to be
 * @param name A description of what is the information being loaded
 */
void
BESXMLInfo::add_data_from_file( const string &key, const string &name )
{
    string newkey = key + ".XML" ;
    BESInfo::add_data_from_file( newkey, name ) ;
}

/** @brief transmit the text information as text
 *
 * use the send_text method on the transmitter to transmit the information
 * back to the client.
 *
 * @param transmitter The type of transmitter to use to transmit the info
 * @param dhi information to help with the transmission
 */
void
BESXMLInfo::transmit( BESTransmitter *transmitter,
		      BESDataHandlerInterface &dhi )
{
    transmitter->send_text( *this, dhi ) ;
}

/** @brief print the information from this informational object to the
 * specified stream
 *
 * @param strm output to this stream
 */
void
BESXMLInfo::print( ostream &strm )
{
    BESInfo::print( strm ) ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * this XML informational object.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESXMLInfo::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESXMLInfo::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "indentation \"" << _indent << "\"" << endl ;
    strm << BESIndent::LMarg << "do indent? " << _do_indent << endl ;
    BESInfo::dump( strm ) ;
    BESIndent::UnIndent() ;
}

BESInfo *
BESXMLInfo::BuildXMLInfo( const string &info_type )
{
    return new BESXMLInfo( ) ;
}

