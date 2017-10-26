// TestCommand.cc

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

#include "TestCommand.h"
#include "BESXMLUtils.h"
#include "BESSyntaxUserError.h"
#include "BESDebug.h"

TestCommand::TestCommand( const BESDataHandlerInterface &base_dhi )
    : BESXMLCommand( base_dhi )
{
}

/** @brief parses the request to build a test signal response
 *
 * A request looks like:
 *
 * test sig;
 *
 * @param tokenizer holds on to the list of tokens to be parsed
 * @param dhi structure that holds request and response information
 * @throws BESSyntaxUserError if there is a problem parsing the command
 * @see BESTokenizer
 * @see _BESDataHandlerInterface
 */
void
TestCommand::parse_request( xmlNode *node )
{
    string name ;
    string value ;
    map<string, string> props ;
    BESXMLUtils::GetNodeInfo( node, name, value, props ) ;
    if( name != "test" )
    {
	string err = "The specified command " + name
		     + " is not a test command" ;
	throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
    }

    string what = props["what"] ;
    d_xmlcmd_dhi.action = what ;
    d_cmd_log_info = (string)"test " + what ;
    BESDEBUG( "besxml", "Converted xml element name to command "
			<< d_xmlcmd_dhi.action << endl ) ;

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
TestCommand::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "TestCommand::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESXMLCommand::dump( strm ) ;
    BESIndent::UnIndent() ;
}

BESXMLCommand *
TestCommand::CommandBuilder( const BESDataHandlerInterface &base_dhi )
{
    return new TestCommand( base_dhi ) ;
}

