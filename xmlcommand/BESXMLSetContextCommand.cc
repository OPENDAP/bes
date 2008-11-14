// BESXMLSetContextCommand.cc

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

#include "BESXMLSetContextCommand.h"
#include "BESXMLUtils.h"
#include "BESUtil.h"
#include "BESResponseNames.h"
#include "BESDataNames.h"
#include "BESSyntaxUserError.h"
#include "BESDebug.h"

BESXMLSetContextCommand::BESXMLSetContextCommand( const BESDataHandlerInterface &base_dhi )
    : BESXMLCommand( base_dhi )
{
}

/** @brief parse a show command. No properties or children elements
 *
    <cmd1.1 prop1=\"prop1val\"> \
	<element1>element1val</element1> \
	<element2 prop2.1=\"prop2.1val\">element2val</element2> \
    </cmd1.1> \
 *
 * @param node xml2 element node pointer
 */
void
BESXMLSetContextCommand::parse_request( xmlNode *node )
{
    string value ;
    string name ;
    string action ;
    map<string, string> props ;
    BESXMLUtils::GetNodeInfo( node, action, value, props ) ;
    if( action != SET_CONTEXT_STR )
    {
	string err = "The specified command " + action
		     + " is not a set context command" ;
	throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
    }

    _dhi.action = SET_CONTEXT ;

    name = props["name"] ;
    if( name.empty() )
    {
	string err = action + " command: name property missing" ;
	throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
    }
    if( value.empty() )
    {
	string err = action + " command: context value missing" ;
	throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
    }
    _dhi.data[CONTEXT_NAME] = name ;
    _dhi.data[CONTEXT_VALUE] = value ;

    // now that we've set the action, go get the response handler for the
    // action
    BESXMLCommand::set_response() ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESXMLSetContextCommand::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESXMLSetContextCommand::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESXMLCommand::dump( strm ) ;
    BESIndent::UnIndent() ;
}

BESXMLCommand *
BESXMLSetContextCommand::CommandBuilder( const BESDataHandlerInterface &base_dhi )
{
    return new BESXMLSetContextCommand( base_dhi ) ;
}

