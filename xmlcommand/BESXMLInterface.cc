// BESXMLInterface.cc

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

#include <iostream>
#include <sstream>

using std::endl ;
using std::cout ;
using std::stringstream ;

#include "BESXMLInterface.h"
#include "BESXMLCommand.h"
#include "BESXMLUtils.h"
#include "BESDataNames.h"
#include "BESDebug.h"
#include "BESLog.h"
#include "BESSyntaxUserError.h"
#include "BESReturnManager.h"

BESXMLInterface::BESXMLInterface( const string &xml_doc, ostream *strm )
    : BESBasicInterface( strm )
{
    _dhi = &_base_dhi ;
    _dhi->data[DATA_REQUEST] = "xml document" ;
    _dhi->data["XMLDoc"] = xml_doc ;
}

BESXMLInterface::~BESXMLInterface()
{
    clean() ;
}

int
BESXMLInterface::execute_request( const string &from )
{
    return BESBasicInterface::execute_request( from ) ;
}

/** @brief Initialize the BES
 */
void
BESXMLInterface::initialize()
{
    BESBasicInterface::initialize() ;
}

/** @brief Validate the incoming request information
 */
void
BESXMLInterface::validate_data_request()
{
    BESBasicInterface::validate_data_request() ;
}

/** @brief Build the data request plan using the BESCmdParser.
 */
void
BESXMLInterface::build_data_request_plan()
{
    BESDEBUG( "besxml", "building request plan for xml document: "
			<< endl << _dhi->data["XMLDoc"] << endl )
    if( BESLog::TheLog()->is_verbose() )
    {
	*(BESLog::TheLog()) << _dhi->data[SERVER_PID]
		    << " from " << _dhi->data[REQUEST_FROM]
		    << " [" << _dhi->data[DATA_REQUEST] << "] building"
		    << endl ;
    }

    LIBXML_TEST_VERSION

    xmlDoc *doc = NULL ;
    xmlNode *root_element = NULL ;
    xmlNode *current_node = NULL ;

    // set the default error function to my own
    vector<string> parseerrors ;
    xmlSetGenericErrorFunc( (void *)&parseerrors, BESXMLUtils::XMLErrorFunc ) ;

    doc = xmlParseDoc( (unsigned char *)_dhi->data["XMLDoc"].c_str() ) ;
    if( doc == NULL )
    {
	string err = "Problem parsing the request xml document:\n" ;
	bool isfirst = true ;
	vector<string>::const_iterator i = parseerrors.begin() ;
	vector<string>::const_iterator e = parseerrors.end() ;
	for( ; i != e; i++ )
	{
	    if( !isfirst && (*i).compare( 0, 6, "Entity" ) == 0 )
	    {
		err += "\n" ;
	    }
	    err += (*i) ;
	    isfirst = false ;
	}
	throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
    }

    // get the root element and make sure it exists and is called request
    root_element = xmlDocGetRootElement( doc ) ;
    if( !root_element )
    {
	string err = "There is no root element in the xml document" ;
	throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
    }
    if( (string)(char *)root_element->name != "request" )
    {
	string err = (string)"The root element should be a request element, "
		     + "name is " + (char *)root_element->name ;
	throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
    }

    // there should be a request id property with one value.
    map< string, string> props ;
    BESXMLUtils::GetProps( root_element, props ) ;
    string &reqId = props[REQUEST_ID] ;
    if( reqId.empty() )
    {
	string err = (string)"request id value empty" ;
	throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
    }
    _dhi->data[REQUEST_ID] = reqId ;
    BESDEBUG( "besxml", "request id = "<< _dhi->data[REQUEST_ID] << endl )

    // iterate through the children of the request element. Each child is an
    // individual command.
    bool has_response = false ;
    current_node = root_element->children ;

    while( current_node )
    {
        if( current_node->type == XML_ELEMENT_NODE )
	{
	    // given the name of this node we should be able to find a
	    // BESXMLCommand object
	    string node_name = (char *)current_node->name ;
	    p_xmlcmd_builder bldr = BESXMLCommand::find_command( node_name ) ;
	    if( bldr )
	    {
		BESXMLCommand *current_cmd = bldr( _base_dhi ) ;
		if( !current_cmd )
		{
		    string err = (string)"Failed to build command object for "
				 + node_name ;
		    throw BESInternalError( err, __FILE__, __LINE__ ) ;
		}

		// only one of the commands can build a response. If more
		// than one builds a response, throw an error
		bool cmd_has_response = current_cmd->has_response() ;
		if( has_response && cmd_has_response )
		{
		    string err = "Multiple responses not allowed" ;
		    throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
		}
		has_response = cmd_has_response ;

		// parse the request given the current node
		current_cmd->parse_request( current_node ) ;

		// push this new command to the back of the list
		_cmd_list.push_back( current_cmd ) ;

		BESDataHandlerInterface &current_dhi = current_cmd->get_dhi() ;
		string returnAs = current_dhi.data[RETURN_CMD] ;
		if( returnAs != "" )
		{
		    BESDEBUG( "xml", "Finding transmitter: " << returnAs
				     << " ...  " << endl )
		    BESTransmitter *transmitter =
			BESReturnManager::TheManager()->find_transmitter( returnAs ) ;
		    if( !transmitter )
		    {
			string s = (string)"Unable to find transmitter "
				   + returnAs ;
			throw BESSyntaxUserError( s, __FILE__, __LINE__ ) ;
		    }
		    BESDEBUG( "xml", "OK" << endl )
		}
	    }
	    else
	    {
		string err = (string)"Unable to find command for "
			     + node_name ;
		throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
	    }
	}
	current_node = current_node->next ;
    }

    xmlFreeDoc( doc ) ;

    xmlCleanupParser() ;

    BESDEBUG( "besxml", "Done building request plan" << endl )

    BESBasicInterface::build_data_request_plan() ;
}

/** @brief Execute the data request plan
 */
void
BESXMLInterface::execute_data_request_plan()
{
    vector<BESXMLCommand *>::iterator i = _cmd_list.begin() ;
    vector<BESXMLCommand *>::iterator e = _cmd_list.end() ;
    for( ; i != e; i++ )
    {
	(*i)->prep_request() ;
	_dhi = &(*i)->get_dhi() ;
	BESBasicInterface::execute_data_request_plan() ;
    }
}

/** @brief Invoke the aggregation server, if there is one
 */
void
BESXMLInterface::invoke_aggregation()
{
    BESBasicInterface::invoke_aggregation() ;
}

/** @brief Transmit the response object
 */
void
BESXMLInterface::transmit_data()
{
    string returnAs = _dhi->data[RETURN_CMD] ;
    if( returnAs != "" )
    {
	BESDEBUG( "xml", "Setting transmitter: " << returnAs
			 << " ...  " << endl )
	_transmitter =
	    BESReturnManager::TheManager()->find_transmitter( returnAs ) ;
	if( !_transmitter )
	{
	    string s = (string)"Unable to find transmitter "
		       + returnAs ;
	    throw BESSyntaxUserError( s, __FILE__, __LINE__ ) ;
	}
	BESDEBUG( "xml", "OK" << endl )
    }

    BESBasicInterface::transmit_data() ;
} 

/** @brief Log the status of the request to the BESLog file

    @see BESLog
 */
void
BESXMLInterface::log_status()
{
    BESBasicInterface::log_status() ;
}

/** @brief Clean up after the request is completed
 */
void
BESXMLInterface::clean()
{
    BESBasicInterface::clean() ;
    vector<BESXMLCommand *>::iterator i = _cmd_list.begin() ;
    vector<BESXMLCommand *>::iterator e = _cmd_list.end() ;
    for( ; i != e; i++ )
    {
	BESXMLCommand *cmd = *i ;
	delete cmd ;
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESXMLInterface::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESXMLInterface::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESBasicInterface::dump( strm ) ;
    BESIndent::UnIndent() ;


}

