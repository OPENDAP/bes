// OPeNDAPCommand.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org>
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
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>

#ifndef A_OPeNDAPCommand_h
#define A_OPeNDAPCommand_h 1

#include <string>
#include <map>

using std::string ;
using std::map ;

#include "DODSDataHandlerInterface.h"

class DODSResponseHandler ;
class OPeNDAPTokenizer ;

class OPeNDAPCommand
{
private:
    static map< string, OPeNDAPCommand * > cmd_list ;
    typedef map< string, OPeNDAPCommand * >::iterator cmd_iter ;
protected:
    string				_cmd ;
public:
    					OPeNDAPCommand( const string &cmd )
					    : _cmd( cmd ) {}
    virtual				~OPeNDAPCommand() {}

    virtual string			parse_options( OPeNDAPTokenizer &tokens,
					  DODSDataHandlerInterface &dhi ) ;

    virtual DODSResponseHandler *	parse_request( OPeNDAPTokenizer &tokens,
					  DODSDataHandlerInterface &dhi ) = 0 ;

    static OPeNDAPCommand *		TermCommand ;
    static void				add_command( const string &cmd_str,
                                                     OPeNDAPCommand *cmd ) ;
    static OPeNDAPCommand *		rem_command( const string &cmd_str ) ;
    static OPeNDAPCommand *		find_command( const string &cmd_str ) ;
} ;

#endif // A_OPeNDAPCommand_h

