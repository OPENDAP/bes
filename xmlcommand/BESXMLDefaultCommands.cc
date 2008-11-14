// BESXMLDefaultCommands.cc

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

#include <iostream>

using std::endl ;

#include "BESXMLDefaultCommands.h"

#include "BESResponseNames.h"

#include "BESDebug.h"

#include "BESXMLShowCommand.h"
#include "BESXMLSetContextCommand.h"
#include "BESXMLSetContainerCommand.h"
#include "BESXMLDefineCommand.h"
#include "BESXMLGetCommand.h"
#include "BESXMLDeleteContainerCommand.h"
#include "BESXMLDeleteContainersCommand.h"
#include "BESXMLDeleteDefinitionCommand.h"
#include "BESXMLDeleteDefinitionsCommand.h"

int
BESXMLDefaultCommands::initialize( int, char** )
{
    BESDEBUG( "besxml", "Initializing default commands:" << endl )

    BESXMLCommand *cmd = NULL ;

    BESDEBUG( "besxml", "    adding " << SHOW_CONTEXT_STR
			<< " command" << endl )
    BESXMLCommand::add_command( SHOW_CONTEXT_STR,
				BESXMLShowCommand::CommandBuilder ) ;

    BESDEBUG( "besxml", "    adding " << SHOWDEFS_RESPONSE_STR
			<< " command" << endl )
    BESXMLCommand::add_command( SHOWDEFS_RESPONSE_STR,
				BESXMLShowCommand::CommandBuilder ) ;

    BESDEBUG( "besxml", "    adding " << SHOWCONTAINERS_RESPONSE_STR
			<< " command" << endl)
    BESXMLCommand::add_command( SHOWCONTAINERS_RESPONSE_STR,
				BESXMLShowCommand::CommandBuilder ) ;

    BESDEBUG( "besxml", "    adding " << HELP_RESPONSE_STR
			<< " command" << endl)
    BESXMLCommand::add_command( HELP_RESPONSE_STR,
				BESXMLShowCommand::CommandBuilder ) ;

#ifdef BES_DEVELOPER
    BESDEBUG( "besxml", "    adding " << PROCESS_RESPONSE_STR
			<< " command" << endl)
    BESXMLCommand::add_command( PROCESS_RESPONSE_STR,
				BESXMLShowCommand::CommandBuilder ) ;

    BESDEBUG( "besxml", "    adding " << CONFIG_RESPONSE_STR
			<< " command" << endl)
    BESXMLCommand::add_command( CONFIG_RESPONSE_STR,
				BESXMLShowCommand::CommandBuilder ) ;
#endif

    BESDEBUG( "besxml", "    adding " << VERS_RESPONSE_STR
			<< " command" << endl)
    BESXMLCommand::add_command( VERS_RESPONSE_STR,
				BESXMLShowCommand::CommandBuilder ) ;

    BESDEBUG( "besxml", "    adding " << STATUS_RESPONSE_STR
			<< " command" << endl)
    BESXMLCommand::add_command( STATUS_RESPONSE_STR,
				BESXMLShowCommand::CommandBuilder ) ;

    BESDEBUG( "besxml", "    adding " << SET_CONTEXT_STR
			<< " command" << endl )
    BESXMLCommand::add_command( SET_CONTEXT_STR,
				BESXMLSetContextCommand::CommandBuilder ) ;

    BESDEBUG( "besxml", "    adding " << SETCONTAINER_STR
			<< " command" << endl )
    BESXMLCommand::add_command( SETCONTAINER_STR,
				BESXMLSetContainerCommand::CommandBuilder);

    BESDEBUG( "besxml", "    adding " << DEFINE_RESPONSE_STR
			<< " command" << endl )
    BESXMLCommand::add_command( DEFINE_RESPONSE_STR,
				BESXMLDefineCommand::CommandBuilder ) ;

    BESDEBUG( "besxml", "    adding " << GET_RESPONSE
			<< " command" << endl )
    BESXMLCommand::add_command( GET_RESPONSE,
				BESXMLGetCommand::CommandBuilder ) ;

    BESDEBUG( "besxml", "    adding " << DELETE_CONTAINER_STR
			<< " command" << endl )
    BESXMLCommand::add_command( DELETE_CONTAINER_STR,
				BESXMLDeleteContainerCommand::CommandBuilder ) ;

    BESDEBUG( "besxml", "    adding " << DELETE_CONTAINERS_STR
			<< " command" << endl )
    BESXMLCommand::add_command( DELETE_CONTAINERS_STR,
				BESXMLDeleteContainersCommand::CommandBuilder );

    BESDEBUG( "besxml", "    adding " << DELETE_DEFINITION_STR
			<< " command" << endl )
    BESXMLCommand::add_command( DELETE_DEFINITION_STR,
				BESXMLDeleteDefinitionCommand::CommandBuilder );

    BESDEBUG( "besxml", "    adding " << DELETE_DEFINITIONS_STR
			<< " command" << endl )
    BESXMLCommand::add_command( DELETE_DEFINITIONS_STR,
				BESXMLDeleteDefinitionsCommand::CommandBuilder);

    BESDEBUG( "besxml", "Done Initializing default commands:" << endl )

    return 0;
}

int
BESXMLDefaultCommands::terminate( void )
{
    BESDEBUG( "besxml", "Removing default commands:" << endl )

    BESXMLCommand::del_command( GET_RESPONSE ) ;
    BESXMLCommand::del_command( SHOW_CONTEXT_STR ) ;
    BESXMLCommand::del_command( SHOWDEFS_RESPONSE_STR ) ;
    BESXMLCommand::del_command( SHOWCONTAINERS_RESPONSE_STR ) ;
    BESXMLCommand::del_command( HELP_RESPONSE_STR ) ;
#ifdef BES_DEVELOPER
    BESXMLCommand::del_command( PROCESS_RESPONSE_STR ) ;
    BESXMLCommand::del_command( CONFIG_RESPONSE_STR ) ;
#endif
    BESXMLCommand::del_command( VERS_RESPONSE_STR ) ;
    BESXMLCommand::del_command( STATUS_RESPONSE_STR ) ;
    BESXMLCommand::del_command( SET_CONTEXT_STR ) ;
    BESXMLCommand::del_command( SETCONTAINER_STR ) ;
    BESXMLCommand::del_command( DEFINE_RESPONSE_STR ) ;
    BESXMLCommand::del_command( DELETE_CONTAINER_STR ) ;
    BESXMLCommand::del_command( DELETE_CONTAINERS_STR ) ;
    BESXMLCommand::del_command( DELETE_DEFINITION_STR ) ;

    BESDEBUG( "besxml", "Done Removing default commands:" << endl )

    return true;
}

