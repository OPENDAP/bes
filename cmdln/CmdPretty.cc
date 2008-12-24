// CmdPretty.cc

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

#include <libxml/encoding.h>

#include <iostream>
#include <map>

using std::cout ;
using std::endl ;
using std::map ;

#include "CmdPretty.h"
#include "CmdXMLUtils.h"

string CmdPretty::_indent ;

void
CmdPretty::make_pretty( const string &response, ostream &strm )
{
    if( response[0] == '<' && response[1] == '?' )
    {
	LIBXML_TEST_VERSION

	xmlDoc *doc = NULL ;
	xmlNode *root_element = NULL ;

	doc = xmlParseDoc( (unsigned char *)response.c_str() ) ;
	if( doc == NULL )
	{
	    cout << "bescmdln tried to pretty print xml document: "
	         << "failed to parse document" << endl ;
	    strm << response ;
	    xmlCleanupParser() ;
	    return ;
	}

	// get the root element
	root_element = xmlDocGetRootElement( doc ) ;
	if( !root_element )
	{
	    cout << "bescmdln tried to pretty print xml document: "
	         << "failed to get document root" << endl ;
	    strm << response ;
	    if( doc ) xmlFreeDoc( doc ) ;
	    xmlCleanupParser() ;
	    return ;
	}

	CmdPretty::pretty_element( root_element, strm ) ;

	if( doc ) xmlFreeDoc( doc ) ;
	xmlCleanupParser() ;
    }
    else
    {
	strm << response ;
    }
}

void
CmdPretty::pretty_element( xmlNode *node, ostream &strm )
{
    if( !node )
    {
	return ;
    }

    string node_name ;
    string node_val ;
    map< string, string> node_props ;
    CmdXMLUtils::GetNodeInfo( node, node_name, node_val, node_props ) ;
    strm << CmdPretty::_indent << "<" << node_name ;
    map<string,string>::const_iterator i = node_props.begin() ;
    map<string,string>::const_iterator e = node_props.end() ;
    for( ; i != e; i++ )
    {
	strm << " " << (*i).first ;
	if( !(*i).second.empty() )
	{
	    strm << "=\"" << (*i).second << "\"" ;
	}
    }

    string cname ;
    string cval ;
    map<string, string> cprops ;

    // ok ... bad variable name. This means that the node either has a value
    // or has children. So it has stuff.
    bool has_stuff = false ;
    xmlNode *cnode = CmdXMLUtils::GetFirstChild( node, cname, cval, cprops ) ;
    
    if( !cnode && !node_val.empty() && node_val.length() < 80-_indent.length() )
    {
	strm << ">" << node_val << "</" << node_name << ">" << endl ;
    }
    else
    {
	if( cnode || !node_val.empty() )
	{
	    has_stuff = true ;
	    strm << ">" << endl ;
	    CmdPretty::indent( 4 ) ;
	}
	if( !node_val.empty() )
	{
	    CmdPretty::pretty_value( node_val, strm ) ;
	}
	while( cnode )
	{
	    CmdPretty::pretty_element( cnode, strm ) ;
	    cnode = CmdXMLUtils::GetNextChild( cnode, cname, cval, cprops ) ;
	}
	if( has_stuff )
	{
	    CmdPretty::unindent( 4 ) ;
	    strm << _indent << "</" << node_name << ">" << endl ;
	}
	else
	{
	    strm << " />" << endl ;
	}
    }
}

void
CmdPretty::pretty_value( string &value, ostream &strm )
{
    strm << _indent << value << endl ;
}

void
CmdPretty::indent( unsigned int spaces )
{
    unsigned int index = 0 ;
    for( index = 0; index < spaces; index++ )
    {
	CmdPretty::_indent += " " ;
    }
}

void
CmdPretty::unindent( unsigned int spaces )
{
    if( spaces >= _indent.length() )
    {
	_indent = "" ;
    }
    else
    {
	_indent = _indent.substr( 0, _indent.length() - spaces ) ;
    }
}

