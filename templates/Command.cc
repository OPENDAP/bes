// OPENDAP_CLASSOPENDAP_COMMANDCommand.cc

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
#include "OPENDAP_CLASSOPENDAP_COMMANDCommand.h"
#include "BESTokenizer.h"
#include "BESResponseHandlerList.h"
#include "BESSyntaxUserError.h"
#include "OPENDAP_CLASSResponseNames.h"

BESResponseHandler *
OPENDAP_CLASSOPENDAP_COMMANDCommand::parse_request( BESTokenizer &tokenizer,
                                           BESDataHandlerInterface &dhi )
{
    string my_token ;

    /* No sub command, so proceed with the default command
     */
    dhi.action = OPENDAP_MACRO_RESPONSE ;
    BESResponseHandler *retResponse =
	BESResponseHandlerList::TheList()->find_handler( OPENDAP_MACRO_RESPONSE ) ;
    if( !retResponse )
    {
	string s = (string)"No response handler for command " + OPENDAP_MACRO_RESPONSE ;
	throw BESSyntaxUserError( s, __FILE__, __LINE__ ) ;
    }

    my_token = tokenizer.get_next_token() ;
    if( my_token == ";" )
    {
	tokenizer.parse_error( my_token + " not expected\n" ) ;
    }

    // Here is where your code would parse the tokens

    // Last token should be the terminating semicolon (;)
    my_token = tokenizer.get_next_token() ;
    if( my_token != ";" )
    {
	tokenizer.parse_error( my_token + " not expected\n" ) ;
    }

    return retResponse ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
OPENDAP_CLASSOPENDAP_COMMANDCommand::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "OPENDAP_CLASSOPENDAP_COMMANDCommand::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESCommand::dump( strm ) ;
    BESIndent::UnIndent() ;
}

