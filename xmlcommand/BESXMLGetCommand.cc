// BESXMLGetCommand.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include "BESXMLGetCommand.h"
#include "BESDefinitionStorageList.h"
#include "BESDefinitionStorage.h"
#include "BESDefine.h"
#include "BESDataNames.h"
#include "BESResponseNames.h"
#include "BESDapNames.h"
#include "BESXMLUtils.h"
#include "BESUtil.h"
#include "BESSyntaxUserError.h"
#include "BESDebug.h"

BESXMLGetCommand::BESXMLGetCommand( const BESDataHandlerInterface &base_dhi )
    : BESXMLCommand( base_dhi ), _sub_cmd( 0 )
{
}

/** @brief parse a get command.
 *
    &lt;get  type="dds" definition="d" returnAs="name" /&gt;
 *
 * @param node xml2 element node pointer
 */
void
BESXMLGetCommand::parse_request( xmlNode *node )
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

    // grab the type first and check to see if there is a registered command
    // to handle get.<type> requests
    string type = props["type"] ;
    if( type.empty() )
    {
	string err = name + " command: Must specify data product type" ;
	throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
    }
    string new_cmd = (string)GET_RESPONSE + "." + type ;
    p_xmlcmd_builder bldr = BESXMLCommand::find_command( new_cmd ) ;
    if( bldr )
    {
	// the base dhi was copied to this instance's _dhi variable.
	_sub_cmd = bldr( _dhi ) ;
	if( !_sub_cmd )
	{
	    string err = (string)"Failed to build command object for "
			 + new_cmd ;
	    throw BESInternalError( err, __FILE__, __LINE__ ) ;
	}

	// parse the request given the current node
	_sub_cmd->parse_request( node ) ;

	// return from this sub command
	return ;
    }

    parse_basic_get( node, name, type, value, props ) ;
    _str_cmd += ";" ;

    // now that we've set the action, go get the response handler for the
    // action
    BESXMLCommand::set_response() ;
}

void
BESXMLGetCommand::parse_basic_get( xmlNode *node,
				   const string &name,
				   const string &type,
				   const string &value,
				   map<string,string> &props )
{
    _str_cmd = (string)"get " + type ;

    _definition = props["definition"] ;
    if( _definition.empty() )
    {
	string err = name + " command: Must specify definition" ;
	throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
    }
    _str_cmd += " for " + _definition ;

    _space = props["space"] ;
    if( !_space.empty() ) _str_cmd += " in " + _space ;

    string returnAs = props["returnAs"] ;
    if( returnAs.empty() )
    {
	returnAs = DAP2_FORMAT ;
    }
    _dhi.data[RETURN_CMD] = returnAs ;

    _str_cmd += " return as " + returnAs ;

    _dhi.action = "get." ;
    _dhi.action += BESUtil::lowercase( type ) ;
    BESDEBUG( "besxml", "Converted xml element name to command "
			<< _dhi.action << endl ) ;
}

/** @brief returns the BESDataHandlerInterface of either a sub command, if
 * one exists, or this command's
 *
 * @return BESDataHandlerInterface of sub command if it exists or this
 *         instances
 */
BESDataHandlerInterface &
BESXMLGetCommand::get_dhi()
{
    if( _sub_cmd ) return _sub_cmd->get_dhi() ;

    return _dhi ;
}

/** @brief Prepare any information needed to execute the request of
 * this get command
 *
 * This function is used to prepare the information needed to execute
 * the get request. It finds the definition specified in the element
 * and prepares all of the containers within that definition.
 */
void
BESXMLGetCommand::prep_request()
{
    // if there is a sub command then execute the prep request on it
    if( _sub_cmd )
    {
	_sub_cmd->prep_request() ;
	return ;
    }

    BESDefine *d = 0 ;

    if( !_space.empty() )
    {
	BESDefinitionStorage *ds =
	    BESDefinitionStorageList::TheList()->find_persistence( _space ) ;
	if( ds )
	{
	    d = ds->look_for( _definition ) ;
	}
    }
    else
    {
	d = BESDefinitionStorageList::TheList()->look_for( _definition ) ;
    }

    if( !d )
    {
	string s = (string)"Unable to find definition " + _definition ;
	throw BESSyntaxUserError( s, __FILE__, __LINE__ ) ;
    }

    BESDefine::containers_citer i = d->first_container() ;
    BESDefine::containers_citer ie = d->end_container() ;
    while( i != ie )
    {
	_dhi.containers.push_back( *i ) ;
	i++ ;
    }
    _dhi.data[AGG_CMD] = d->get_agg_cmd() ;
    _dhi.data[AGG_HANDLER] = d->get_agg_handler() ;

}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESXMLGetCommand::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESXMLGetCommand::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESXMLCommand::dump( strm ) ;
    BESIndent::UnIndent() ;
}

BESXMLCommand *
BESXMLGetCommand::CommandBuilder( const BESDataHandlerInterface &base_dhi )
{
    return new BESXMLGetCommand( base_dhi ) ;
}

