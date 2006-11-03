// opendap_commands.cc

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

#include <iostream>

using std::endl ;

#include "opendap_commands.h"

#include "BESResponseNames.h"

#include "BESLog.h"

#include "BESGetCommand.h"
#include "BESSetCommand.h"
#include "BESDeleteCommand.h"
#include "BESShowCommand.h"

#include "BESSetContainerCommand.h"
#include "BESDelContainerCommand.h"
#include "BESDelContainersCommand.h"

#include "BESDefineCommand.h"
#include "BESDelDefCommand.h"
#include "BESDelDefsCommand.h"

#include "BESParserException.h"
#include "BESExceptionManager.h"

int
opendap_commands::initialize( int, char** )
{
    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "Initializing default commands:" << endl;

    BESCommand *cmd = NULL ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << GET_RESPONSE << " command" << endl;
    cmd = new BESGetCommand( GET_RESPONSE ) ;
    BESCommand::add_command( GET_RESPONSE, cmd ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << SHOW_RESPONSE << " command" << endl;
    cmd = new BESShowCommand( SHOW_RESPONSE ) ;
    BESCommand::add_command( SHOW_RESPONSE, cmd ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << HELP_RESPONSE << " command" << endl;
    BESCommand::add_command( HELP_RESPONSE, BESCommand::TermCommand ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << PROCESS_RESPONSE << " command" << endl;
    BESCommand::add_command( PROCESS_RESPONSE, BESCommand::TermCommand ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << KEYS_RESPONSE << " command" << endl;
    BESCommand::add_command( KEYS_RESPONSE, BESCommand::TermCommand ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << VERS_RESPONSE << " command" << endl;
    BESCommand::add_command( VERS_RESPONSE, BESCommand::TermCommand ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << STATUS_RESPONSE << " command" << endl;
    BESCommand::add_command( STATUS_RESPONSE, BESCommand::TermCommand ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << SET_RESPONSE << " command" << endl;
    cmd = new BESSetCommand( SET_RESPONSE ) ;
    BESCommand::add_command( SET_RESPONSE, cmd ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << DELETE_RESPONSE << " command" << endl;
    cmd = new BESDeleteCommand( DELETE_RESPONSE ) ;
    BESCommand::add_command( DELETE_RESPONSE, cmd ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << SETCONTAINER << " command" << endl;
    cmd = new BESSetContainerCommand( SETCONTAINER ) ;
    BESCommand::add_command( SETCONTAINER, cmd ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << SHOWCONTAINERS_RESPONSE << " command" << endl;
    BESCommand::add_command( SHOWCONTAINERS_RESPONSE, BESCommand::TermCommand ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << DELETE_CONTAINER << " command" << endl;
    cmd = new BESDelContainerCommand( DELETE_CONTAINER ) ;
    BESCommand::add_command( DELETE_CONTAINER, cmd ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << DELETE_CONTAINERS << " command" << endl;
    cmd = new BESDelContainersCommand( DELETE_CONTAINERS ) ;
    BESCommand::add_command( DELETE_CONTAINERS, cmd ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << DEFINE_RESPONSE << " command" << endl;
    cmd = new BESDefineCommand( DEFINE_RESPONSE ) ;
    BESCommand::add_command( DEFINE_RESPONSE, cmd ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << SHOWDEFS_RESPONSE << " command" << endl;
    BESCommand::add_command( SHOWDEFS_RESPONSE, BESCommand::TermCommand ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << DELETE_DEFINITION << " command" << endl;
    cmd = new BESDelDefCommand( DELETE_DEFINITION ) ;
    BESCommand::add_command( DELETE_DEFINITION, cmd ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << DELETE_DEFINITIONS << " command" << endl;
    cmd = new BESDelDefsCommand( DELETE_DEFINITIONS ) ;
    BESCommand::add_command( DELETE_DEFINITIONS, cmd ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding parser exception callback" << endl ;
    BESExceptionManager::TheEHM()->add_ehm_callback( BESParserException::handleException ) ;

    return 0;
}

int
opendap_commands::terminate( void )
{
    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "Removing default commands:" << endl;

    BESCommand::del_command( GET_RESPONSE ) ;
    BESCommand::del_command( SHOW_RESPONSE ) ;
    BESCommand::del_command( SET_RESPONSE ) ;
    BESCommand::del_command( DELETE_RESPONSE ) ;

    BESCommand::del_command( SETCONTAINER ) ;
    BESCommand::del_command( SHOWCONTAINERS_RESPONSE ) ;
    BESCommand::del_command( DELETE_CONTAINER ) ;
    BESCommand::del_command( DELETE_CONTAINERS ) ;

    BESCommand::del_command( DEFINE_RESPONSE ) ;
    BESCommand::del_command( SHOWDEFS_RESPONSE ) ;
    BESCommand::del_command( DELETE_DEFINITION ) ;
    BESCommand::del_command( DELETE_DEFINITIONS ) ;

    return true;
}

