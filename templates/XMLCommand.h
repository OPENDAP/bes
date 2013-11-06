// OPENDAP_CLASSOPENDAP_COMMANDCommand.h

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

#ifndef A_OPENDAP_CLASSOPENDAP_COMMANDXMLCommand_h
#define A_OPENDAP_CLASSOPENDAP_COMMANDXMLCommand_h 1

#include "BESXMLCommand.h"
#include "BESDataHandlerInterface.h"

class OPENDAP_CLASSOPENDAP_COMMANDXMLCommand : public BESXMLCommand
{
public:
    				OPENDAP_CLASSOPENDAP_COMMANDXMLCommand( const BESDataHandlerInterface &base_dhi ) ;
    virtual			~OPENDAP_CLASSOPENDAP_COMMANDXMLCommand() {}

    virtual void		parse_request( xmlNode *node ) ;

    // does your command have a response. Some do not. Only one command
    // per request document can have a response.
    virtual bool		has_response() { return false ; }

    virtual void		prep_request() ;

    virtual void		dump( ostream &strm ) const ;

    static BESXMLCommand *	CommandBuilder( const BESDataHandlerInterface &base_dhi ) ;
} ;

#endif // A_OPENDAP_CLASSOPENDAP_COMMANDXMLCommand_h

