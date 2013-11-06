// OPENDAP_CLASSOPENDAP_COMMANDXMLCommand.cc

// Copyright (c) 2013 OPeNDAP, Inc. Author: James Gallagher
// <jgallagher@opendap.org>, Patrick West <pwest@opendap.org>
// Nathan Potter <npotter@opendap.org>
//                                                                            
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2.1 of
// the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02110-1301 U\ SA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI.
// 02874-0112.
#include "OPENDAP_CLASSOPENDAP_COMMANDXMLCommand.h"
#include "BESXMLUtils.h"
#include "BESSyntaxUserError.h"
#include "BESDebug.h"

OPENDAP_CLASSOPENDAP_COMMANDXMLCommand::OPENDAP_CLASSOPENDAP_COMMANDXMLCommand( const BESDataHandlerInterface &base_dhi )
    : BESXMLCommand( base_dhi )
{
}

/** @brief parse a OPENDAP_COMMAND command.
 *
 *
 * @param node xml2 element node pointer
 */
void
OPENDAP_CLASSOPENDAP_COMMANDXMLCommand::parse_request( xmlNode *node )
{
    // your code here - parsing the node for your purposes

    // now that we've set the action, go get the response handler for the
    // action
    BESXMLCommand::set_response() ;
}

/** @brief the request has been parsed, take the information and prepare
 * to execute the request
 */
void
OPENDAP_CLASSOPENDAP_COMMANDXMLCommand::prep_request()
{
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
OPENDAP_CLASSOPENDAP_COMMANDXMLCommand::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "OPENDAP_CLASSOPENDAP_COMMANDXMLCommand::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESXMLCommand::dump( strm ) ;
    BESIndent::UnIndent() ;
}

BESXMLCommand *
OPENDAP_CLASSOPENDAP_COMMANDXMLCommand::CommandBuilder( const BESDataHandlerInterface &base_dhi )
{
    return new OPENDAP_CLASSOPENDAP_COMMANDXMLCommand( base_dhi ) ;
}

