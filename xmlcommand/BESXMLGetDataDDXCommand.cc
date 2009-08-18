// BESXMLGetDataDDXCommand.cc

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

#include "BESXMLGetDataDDXCommand.h"
#include "BESDefinitionStorageList.h"
#include "BESDefinitionStorage.h"
#include "BESDefine.h"
#include "BESDapNames.h"
#include "BESResponseNames.h"
#include "BESXMLUtils.h"
#include "BESUtil.h"
#include "BESSyntaxUserError.h"
#include "BESDebug.h"

BESXMLGetDataDDXCommand::BESXMLGetDataDDXCommand( const BESDataHandlerInterface &base_dhi )
    : BESXMLGetCommand( base_dhi )
{
}

/** @brief parse a show command. No properties or children elements
 *
    <get  type="dds" definition="d" returnAs="name" />
 *
 * @param node xml2 element node pointer
 */
void
BESXMLGetDataDDXCommand::parse_request( xmlNode *node )
{
    string name ;
    string value ;
    map<string, string> props ;
    BESXMLUtils::GetNodeInfo( node, name, value, props ) ;

    if( name != GET_RESPONSE )
    {
	string err = "The specified command " + name
		     + " is not a get command" ;
	throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
    }

    string type = props["type"] ;
    if( type.empty() || type != DATADDX_SERVICE )
    {
	string err = name + " command: data product must be "
			  + DATADDX_SERVICE ;
	throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
    }

    parse_basic_get( node, name, type, value, props ) ;

    // Get the elements for contentStartId and mimeBoundary
    map<string,string> cprops ;
    string cname ;
    string cval ;
    int elems = 0 ;
    xmlNode *cnode = BESXMLUtils::GetFirstChild( node, cname, cval, cprops ) ;
    while( cnode && (elems < 2) )
    {
	if( cname == "contentStartId" )
	{
	    if( !_contentStartId.empty() )
	    {
		string err = name
			     + " command: contentStartId has multiple values" ;
		throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
	    }
	    _contentStartId = cval ;
	    _str_cmd += " contentStartId " + _contentStartId ;
	    elems++ ;
	}
	if( cname == "mimeBoundary" )
	{
	    if( !_mimeBoundary.empty() )
	    {
		string err = name
			     + " command: mimeBoundary has multiple values" ;
		throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
	    }
	    _mimeBoundary = cval ;
	    _str_cmd += " mimeBoundary " + _mimeBoundary ;
	    elems++ ;
	}
	cprops.clear() ;
	cnode = BESXMLUtils::GetNextChild( cnode, cname, cval, cprops ) ;
    }
    if( _contentStartId.empty() )
    {
	string err = name + " command: contentStartId not specified" ;
	throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
    }
    if( _mimeBoundary.empty() )
    {
	string err = name + " command: mimeBoundary not specified" ;
	throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
    }

    // now that we've set the action, go get the response handler for the
    // action
    BESXMLCommand::set_response() ;
}

/** @brief prepare the get dataddx command
 *
 * set the contentStartId and mimeBoundary values in the data handler
 * interface
 */
void
BESXMLGetDataDDXCommand::prep_request()
{
    BESXMLGetCommand::prep_request() ;
    _dhi.data[DATADDX_STARTID] = _contentStartId ;
    _dhi.data[DATADDX_BOUNDARY] = _mimeBoundary ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESXMLGetDataDDXCommand::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESXMLGetDataDDXCommand::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESXMLCommand::dump( strm ) ;
    BESIndent::UnIndent() ;
}

BESXMLCommand *
BESXMLGetDataDDXCommand::CommandBuilder( const BESDataHandlerInterface &base_dhi )
{
    return new BESXMLGetDataDDXCommand( base_dhi ) ;
}

