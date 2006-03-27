// OPeNDAPCommandModule.cc

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

#include <iostream>

using std::endl ;
using std::cout ;

#include "OPeNDAPCommandModule.h"

#include "DODSResponseNames.h"

#include "DODSLog.h"

#include "OPeNDAPGetCommand.h"
#include "OPeNDAPSetCommand.h"
#include "OPeNDAPShowCommand.h"
#include "OPeNDAPDefineCommand.h"
#include "OPeNDAPDeleteCommand.h"
#include "OPeNDAPCatalogCommand.h"

#include "OPeNDAPParserException.h"
#include "DODS.h"


void
OPeNDAPCommandModule::initialize()
{
    cout << "OPeNDAPCommandModule initializing" << endl ;
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "Initializing OPeNDAP Default Commands" << endl;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << GET_RESPONSE << " command" << endl;
    OPeNDAPCommand *cmd = new OPeNDAPGetCommand( GET_RESPONSE ) ;
    OPeNDAPCommand::add_command( GET_RESPONSE, cmd ) ;

    string cmd_name = string( GET_RESPONSE ) + "." + DAS_RESPONSE ;
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << cmd_name << " command" << endl;
    OPeNDAPCommand::add_command( cmd_name, OPeNDAPCommand::TermCommand ) ;

    cmd_name = string( GET_RESPONSE ) + "." + DDS_RESPONSE ;
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << cmd_name << " command" << endl;
    OPeNDAPCommand::add_command( cmd_name, OPeNDAPCommand::TermCommand ) ;

    cmd_name = string( GET_RESPONSE ) + "." + DDX_RESPONSE ;
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << cmd_name << " command" << endl;
    OPeNDAPCommand::add_command( cmd_name, OPeNDAPCommand::TermCommand ) ;

    cmd_name = string( GET_RESPONSE ) + "." + DATA_RESPONSE ;
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << cmd_name << " command" << endl;
    OPeNDAPCommand::add_command( cmd_name, OPeNDAPCommand::TermCommand ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << SHOW_RESPONSE << " command" << endl;
    cmd = new OPeNDAPShowCommand( SHOW_RESPONSE ) ;
    OPeNDAPCommand::add_command( SHOW_RESPONSE, cmd ) ;

    cmd_name = string( SHOW_RESPONSE ) + "." + HELP_RESPONSE ;
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << cmd_name << " command" << endl;
    OPeNDAPCommand::add_command( cmd_name, OPeNDAPCommand::TermCommand ) ;

    cmd_name = string( SHOW_RESPONSE ) + "." + PROCESS_RESPONSE ;
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << cmd_name << " command" << endl;
    OPeNDAPCommand::add_command( cmd_name, OPeNDAPCommand::TermCommand ) ;

    cmd_name = string( SHOW_RESPONSE ) + "." + CONTAINERS_RESPONSE ;
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << cmd_name << " command" << endl;
    OPeNDAPCommand::add_command( cmd_name, OPeNDAPCommand::TermCommand ) ;

    cmd_name = string( SHOW_RESPONSE ) + "." + DEFINITIONS_RESPONSE ;
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << cmd_name << " command" << endl;
    OPeNDAPCommand::add_command( cmd_name, OPeNDAPCommand::TermCommand ) ;

    cmd_name = string( SHOW_RESPONSE ) + "." + KEYS_RESPONSE ;
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << cmd_name << " command" << endl;
    OPeNDAPCommand::add_command( cmd_name, OPeNDAPCommand::TermCommand ) ;

    cmd_name = string( SHOW_RESPONSE ) + "." + VERS_RESPONSE ;
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << cmd_name << " command" << endl;
    OPeNDAPCommand::add_command( cmd_name, OPeNDAPCommand::TermCommand ) ;

    cmd_name = string( SHOW_RESPONSE ) + "." + STATUS_RESPONSE ;
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << cmd_name << " command" << endl;
    OPeNDAPCommand::add_command( cmd_name, OPeNDAPCommand::TermCommand ) ;

    cmd_name = string( SHOW_RESPONSE ) + "." + CATALOG_RESPONSE ;
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << cmd_name << " command" << endl;
    cmd = new OPeNDAPCatalogCommand( cmd_name ) ;
    OPeNDAPCommand::add_command( cmd_name, cmd ) ;

    cmd_name = string( SHOW_RESPONSE ) + "." + CATALOG_RESPONSE ;
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << cmd_name << " command" << endl;
    cmd = new OPeNDAPCatalogCommand( cmd_name ) ;
    OPeNDAPCommand::add_command( cmd_name, cmd ) ;

    cmd_name = string( SHOW_RESPONSE ) + "." + SHOW_INFO_RESPONSE ;
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << cmd_name << " command" << endl;
    cmd = new OPeNDAPCatalogCommand( cmd_name ) ;
    OPeNDAPCommand::add_command( cmd_name, cmd ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << DEFINE_RESPONSE << " command" << endl;
    cmd = new OPeNDAPDefineCommand( DEFINE_RESPONSE ) ;
    OPeNDAPCommand::add_command( DEFINE_RESPONSE, cmd ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << SET_RESPONSE << " command" << endl;
    cmd = new OPeNDAPSetCommand( SET_RESPONSE ) ;
    OPeNDAPCommand::add_command( SET_RESPONSE, cmd ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << DELETE_RESPONSE << " command" << endl;
    cmd = new OPeNDAPDeleteCommand( DELETE_RESPONSE ) ;
    OPeNDAPCommand::add_command( DELETE_RESPONSE, cmd ) ;

    cmd_name = string( DELETE_RESPONSE ) + "." + DELETE_CONTAINER ;
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << cmd_name << " command" << endl;
    OPeNDAPCommand::add_command( cmd_name, OPeNDAPCommand::TermCommand ) ;

    cmd_name = string( DELETE_RESPONSE ) + "." + DELETE_DEFINITION ;
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << cmd_name << " command" << endl;
    OPeNDAPCommand::add_command( cmd_name, OPeNDAPCommand::TermCommand ) ;

    cmd_name = string( DELETE_RESPONSE ) + "." + DELETE_DEFINITIONS ;
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << cmd_name << " command" << endl;
    OPeNDAPCommand::add_command( cmd_name, OPeNDAPCommand::TermCommand ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding parser exception callback" << endl ;
    DODS::add_ehm_callback( OPeNDAPParserException::handleException ) ;
}

void
OPeNDAPCommandModule::terminate()
{
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "Removing OPeNDAP Default Commands" << endl;

    OPeNDAPCommand *cmd = OPeNDAPCommand::rem_command( GET_RESPONSE ) ;
    if( cmd ) delete cmd ;

    cmd = OPeNDAPCommand::rem_command( SHOW_RESPONSE ) ;
    if( cmd ) delete cmd ;

    cmd = OPeNDAPCommand::rem_command( DEFINE_RESPONSE ) ;
    if( cmd ) delete cmd ;

    cmd = OPeNDAPCommand::rem_command( SET_RESPONSE ) ;
    if( cmd ) delete cmd ;

    cmd = OPeNDAPCommand::rem_command( DELETE_RESPONSE ) ;
    if( cmd ) delete cmd ;
}

extern "C"
{
    OPeNDAPAbstractModule *maker()
    {
	return new OPeNDAPCommandModule ;
    }
}

