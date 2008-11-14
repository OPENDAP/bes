// BESXMLCommand.cc

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

#include "BESXMLCommand.h"
#include "BESResponseHandlerList.h"
#include "BESSyntaxUserError.h"

map< string, p_xmlcmd_builder> BESXMLCommand::cmd_list ;

BESXMLCommand::BESXMLCommand( const BESDataHandlerInterface &base_dhi )
{
    _dhi.make_copy( base_dhi ) ;
}

void
BESXMLCommand::set_response()
{
    _dhi.response_handler =
	BESResponseHandlerList::TheList()->find_handler( _dhi.action ) ;
    if( !_dhi.response_handler )
    {
	string err( "Command " ) ;
	err += _dhi.action + " does not have a registered response handler" ;
	throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
    }
}

void
BESXMLCommand::add_command( const string &cmd_str, p_xmlcmd_builder cmd )
{
    BESXMLCommand::cmd_list[cmd_str] = cmd ;
}

bool
BESXMLCommand::del_command( const string &cmd_str )
{
    bool ret = false ;

    BESXMLCommand::cmd_iter iter = BESXMLCommand::cmd_list.find( cmd_str ) ;
    if( iter != BESXMLCommand::cmd_list.end() )
    {
	BESXMLCommand::cmd_list.erase( iter ) ;
    }
    return ret ;
}

p_xmlcmd_builder
BESXMLCommand::find_command( const string &cmd_str )
{
    return BESXMLCommand::cmd_list[cmd_str] ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESXMLCommand::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESXMLCommand::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESIndent::UnIndent() ;
}

