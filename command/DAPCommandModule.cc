// DAPCommandModule.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the DAP Data Access Protocol.

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
using std::cout ;

#include "DAPCommandModule.h"

#include "BESResponseNames.h"
#include "BESLog.h"

#include "BESCommand.h"
#include "BESSetContainerCommand.h"
#include "BESDefineCommand.h"
#include "BESDelContainerCommand.h"
#include "BESDelContainersCommand.h"
#include "BESDelDefCommand.h"
#include "BESDelDefsCommand.h"
#include "BESCatalogCommand.h"

#include "BESParserException.h"
#include "BESExceptionManager.h"


void
DAPCommandModule::initialize()
{
    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "Initializing DAP Commands" << endl;

    BESCommand *cmd = NULL ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << DAS_RESPONSE << " command" << endl;
    BESCommand::add_command( DAS_RESPONSE, BESCommand::TermCommand ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << DDS_RESPONSE << " command" << endl;
    BESCommand::add_command( DDS_RESPONSE, BESCommand::TermCommand ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << DDX_RESPONSE << " command" << endl;
    BESCommand::add_command( DDX_RESPONSE, BESCommand::TermCommand ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << DATA_RESPONSE << " command" << endl;
    BESCommand::add_command( DATA_RESPONSE, BESCommand::TermCommand ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << SHOWCONTAINERS_RESPONSE << " command" << endl;
    BESCommand::add_command( SHOWCONTAINERS_RESPONSE, BESCommand::TermCommand ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << SHOWDEFS_RESPONSE << " command" << endl;
    BESCommand::add_command( SHOWDEFS_RESPONSE, BESCommand::TermCommand ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << CATALOG_RESPONSE << " command" << endl;
    cmd = new BESCatalogCommand( CATALOG_RESPONSE ) ;
    BESCommand::add_command( CATALOG_RESPONSE, cmd ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << SHOW_INFO_RESPONSE << " command" << endl;
    cmd = new BESCatalogCommand( SHOW_INFO_RESPONSE ) ;
    BESCommand::add_command( SHOW_INFO_RESPONSE, cmd ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << DEFINE_RESPONSE << " command" << endl;
    cmd = new BESDefineCommand( DEFINE_RESPONSE ) ;
    BESCommand::add_command( DEFINE_RESPONSE, cmd ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << SETCONTAINER << " command" << endl;
    cmd = new BESSetContainerCommand( SETCONTAINER ) ;
    BESCommand::add_command( SETCONTAINER, cmd ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << DELETE_CONTAINER << " command" << endl;
    cmd = new BESDelContainerCommand( DELETE_CONTAINER ) ;
    BESCommand::add_command( DELETE_CONTAINER, cmd ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << DELETE_CONTAINERS << " command" << endl;
    cmd = new BESDelContainersCommand( DELETE_CONTAINERS ) ;
    BESCommand::add_command( DELETE_CONTAINERS, cmd ) ;

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
}

void
DAPCommandModule::terminate()
{
    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "Removing DAP Commands" << endl;

    BESCommand *cmd = NULL ;

    (*BESLog::TheLog()) << "Removing DEFINE_RESPONSE" << endl;
    cmd = BESCommand::rem_command( DEFINE_RESPONSE ) ;
    if( cmd ) delete cmd ;

    (*BESLog::TheLog()) << "Removing SHOWCONTAINERS_RESPONSE" << endl;
    cmd = BESCommand::rem_command( SHOWCONTAINERS_RESPONSE ) ;
    if( cmd ) delete cmd ;

    (*BESLog::TheLog()) << "Removing DELETE_CONTAINER" << endl;
    cmd = BESCommand::rem_command( DELETE_CONTAINER ) ;
    if( cmd ) delete cmd ;

    (*BESLog::TheLog()) << "Removing DELETE_CONTAINERS" << endl;
    cmd = BESCommand::rem_command( DELETE_CONTAINERS ) ;
    if( cmd ) delete cmd ;

    (*BESLog::TheLog()) << "Removing DELETE_DEFINITION" << endl;
    cmd = BESCommand::rem_command( DELETE_DEFINITION ) ;
    if( cmd ) delete cmd ;

    (*BESLog::TheLog()) << "Removing DELETE_DEFINITIONS" << endl;
    cmd = BESCommand::rem_command( DELETE_DEFINITIONS ) ;
    if( cmd ) delete cmd ;
}

extern "C"
{
    BESAbstractModule *maker()
    {
	return new DAPCommandModule ;
    }
}

