// opendap_commands.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org> and Jose Garcia <jgarcia@ucar.org>
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

#include <iostream>

using std::endl ;

#include "opendap_commands.h"

#include "DODSResponseNames.h"

#include "DODSLog.h"

#include "OPeNDAPGetCommand.h"
#include "OPeNDAPSetCommand.h"
#include "OPeNDAPDeleteCommand.h"
#include "OPeNDAPShowCommand.h"

#include "OPeNDAPParserException.h"
#include "DODS.h"

int
opendap_commands::initialize( int, char** )
{
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "Initializing default commands:" << endl;

    OPeNDAPCommand *cmd = NULL ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << GET_RESPONSE << " command" << endl;
    cmd = new OPeNDAPGetCommand( GET_RESPONSE ) ;
    OPeNDAPCommand::add_command( GET_RESPONSE, cmd ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << SHOW_RESPONSE << " command" << endl;
    cmd = new OPeNDAPShowCommand( SHOW_RESPONSE ) ;
    OPeNDAPCommand::add_command( SHOW_RESPONSE, cmd ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << HELP_RESPONSE << " command" << endl;
    OPeNDAPCommand::add_command( HELP_RESPONSE, OPeNDAPCommand::TermCommand ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << PROCESS_RESPONSE << " command" << endl;
    OPeNDAPCommand::add_command( PROCESS_RESPONSE, OPeNDAPCommand::TermCommand ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << KEYS_RESPONSE << " command" << endl;
    OPeNDAPCommand::add_command( KEYS_RESPONSE, OPeNDAPCommand::TermCommand ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << VERS_RESPONSE << " command" << endl;
    OPeNDAPCommand::add_command( VERS_RESPONSE, OPeNDAPCommand::TermCommand ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << STATUS_RESPONSE << " command" << endl;
    OPeNDAPCommand::add_command( STATUS_RESPONSE, OPeNDAPCommand::TermCommand ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << SET_RESPONSE << " command" << endl;
    cmd = new OPeNDAPSetCommand( SET_RESPONSE ) ;
    OPeNDAPCommand::add_command( SET_RESPONSE, cmd ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << DELETE_RESPONSE << " command" << endl;
    cmd = new OPeNDAPDeleteCommand( DELETE_RESPONSE ) ;
    OPeNDAPCommand::add_command( DELETE_RESPONSE, cmd ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding parser exception callback" << endl ;
    DODS::add_ehm_callback( OPeNDAPParserException::handleException ) ;

    return 0;
}

int
opendap_commands::terminate( void )
{
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "Removing DODS Commands:" << endl;

    OPeNDAPCommand *cmd = OPeNDAPCommand::rem_command( GET_RESPONSE ) ;
    if( cmd ) delete cmd ;

    cmd = OPeNDAPCommand::rem_command( SHOW_RESPONSE ) ;
    if( cmd ) delete cmd ;

    cmd = OPeNDAPCommand::rem_command( SET_RESPONSE ) ;
    if( cmd ) delete cmd ;

    cmd = OPeNDAPCommand::rem_command( DELETE_RESPONSE ) ;
    if( cmd ) delete cmd ;

    return true;
}

