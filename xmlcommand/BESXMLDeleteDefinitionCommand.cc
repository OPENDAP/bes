// BESXMLDeleteDefinitionCommand.cc

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

#include "BESXMLDeleteDefinitionCommand.h"
#include "BESDefinitionStorageList.h"
#include "BESDataNames.h"
#include "BESResponseNames.h"
#include "BESXMLUtils.h"
#include "BESUtil.h"
#include "BESSyntaxUserError.h"
#include "BESDebug.h"

BESXMLDeleteDefinitionCommand::BESXMLDeleteDefinitionCommand( const BESDataHandlerInterface &base_dhi )
    : BESXMLCommand( base_dhi )
{
}

/** @brief parse a show command. No properties or children elements
 *
    <deleteContainer name="containerName" store="storeName" />
 *
 * @param node xml2 element node pointer
 */
void
BESXMLDeleteDefinitionCommand::parse_request( xmlNode *node )
{
    string name ;
    string value ;
    map<string, string> props ;
    BESXMLUtils::GetNodeInfo( node, name, value, props ) ;
    if( name != DELETE_DEFINITION_STR )
    {
	string err = "The specified command " + name
		     + " is not a delete definition command" ;
	throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
    }

    string def_name = props["name"] ;
    if( def_name.empty() )
    {
	string err = name + " command: Must specify the definition to delete" ;
	throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
    }
    _dhi.data[DEF_NAME] = def_name ;

    // optional property
    _dhi.data[STORE_NAME] = props["store"] ; 
    if( _dhi.data[STORE_NAME].empty() )
    {
	_dhi.data[STORE_NAME] = PERSISTENCE_VOLATILE ;
    }

    _dhi.action = DELETE_DEFINITION ;

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
BESXMLDeleteDefinitionCommand::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESXMLDeleteDefinitionCommand::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESXMLCommand::dump( strm ) ;
    BESIndent::UnIndent() ;
}

BESXMLCommand *
BESXMLDeleteDefinitionCommand::CommandBuilder( const BESDataHandlerInterface &base_dhi )
{
    return new BESXMLDeleteDefinitionCommand( base_dhi ) ;
}

