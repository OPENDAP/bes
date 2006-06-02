// OPeNDAPCommand.cc

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
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include "OPeNDAPCommand.h"
#include "OPeNDAPTermCommand.h"
#include "OPeNDAPTokenizer.h"
#include "OPeNDAPDataNames.h"

string
OPeNDAPCommand::parse_options( OPeNDAPTokenizer &tokens,
			       DODSDataHandlerInterface &dhi )
{
    string my_token = tokens.get_next_token() ;
    return my_token ;
}

OPeNDAPCommand *OPeNDAPCommand::TermCommand = new OPeNDAPTermCommand( "term" ) ;
map< string, OPeNDAPCommand * > OPeNDAPCommand::cmd_list ;

void
OPeNDAPCommand::add_command( const string &cmd_str, OPeNDAPCommand *cmd )
{
    OPeNDAPCommand::cmd_list[cmd_str] = cmd ;
}

OPeNDAPCommand *
OPeNDAPCommand::rem_command( const string &cmd_str )
{
    OPeNDAPCommand *cmd = NULL ;

    OPeNDAPCommand::cmd_iter iter = OPeNDAPCommand::cmd_list.find( cmd_str ) ;
    if( iter != OPeNDAPCommand::cmd_list.end() )
    {
	cmd = (*iter).second ;
	if( cmd == OPeNDAPCommand::TermCommand )
	    cmd = NULL ;
	OPeNDAPCommand::cmd_list.erase( iter ) ;
    }
    return cmd ;
}

OPeNDAPCommand *
OPeNDAPCommand::find_command( const string &cmd_str )
{
    return OPeNDAPCommand::cmd_list[cmd_str] ;
}

