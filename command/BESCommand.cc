// BESCommand.cc

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

#include "BESCommand.h"
#include "BESTermCommand.h"
#include "BESTokenizer.h"
#include "BESDataNames.h"

string
BESCommand::parse_options( BESTokenizer &tokens,
			   BESDataHandlerInterface &dhi )
{
    return tokens.get_next_token() ;
}

BESCommand *BESCommand::TermCommand = new BESTermCommand( "term" ) ;
map< string, BESCommand * > BESCommand::cmd_list ;

void
BESCommand::add_command( const string &cmd_str, BESCommand *cmd )
{
    BESCommand::cmd_list[cmd_str] = cmd ;
}

bool
BESCommand::del_command( const string &cmd_str )
{
    bool ret = false ;
    BESCommand *cmd = NULL ;

    BESCommand::cmd_iter iter = BESCommand::cmd_list.find( cmd_str ) ;
    if( iter != BESCommand::cmd_list.end() )
    {
	cmd = (*iter).second ;
	if( cmd != NULL )
	    ret = true ;
	if( cmd == BESCommand::TermCommand )
	    cmd = NULL ;
	BESCommand::cmd_list.erase( iter ) ;
    }
    if( cmd ) delete cmd ;
    return ret ;
}

BESCommand *
BESCommand::find_command( const string &cmd_str )
{
    return BESCommand::cmd_list[cmd_str] ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESCommand::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESCommand::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "command: " << _cmd << endl ;
    BESIndent::UnIndent() ;
}

