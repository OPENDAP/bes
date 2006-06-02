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

#include "DODSResponseNames.h"
#include "DODSLog.h"

#include "OPeNDAPCommand.h"
#include "OPeNDAPSetContainerCommand.h"
#include "OPeNDAPDefineCommand.h"
#include "OPeNDAPDelContainerCommand.h"
#include "OPeNDAPDelContainersCommand.h"
#include "OPeNDAPDelDefCommand.h"
#include "OPeNDAPDelDefsCommand.h"
#include "OPeNDAPCatalogCommand.h"

#include "OPeNDAPParserException.h"
#include "DODS.h"


void
DAPCommandModule::initialize()
{
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "Initializing DAP Commands" << endl;

    OPeNDAPCommand *cmd = NULL ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << DAS_RESPONSE << " command" << endl;
    OPeNDAPCommand::add_command( DAS_RESPONSE, OPeNDAPCommand::TermCommand ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << DDS_RESPONSE << " command" << endl;
    OPeNDAPCommand::add_command( DDS_RESPONSE, OPeNDAPCommand::TermCommand ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << DDX_RESPONSE << " command" << endl;
    OPeNDAPCommand::add_command( DDX_RESPONSE, OPeNDAPCommand::TermCommand ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << DATA_RESPONSE << " command" << endl;
    OPeNDAPCommand::add_command( DATA_RESPONSE, OPeNDAPCommand::TermCommand ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << SHOWCONTAINERS_RESPONSE << " command" << endl;
    OPeNDAPCommand::add_command( SHOWCONTAINERS_RESPONSE, OPeNDAPCommand::TermCommand ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << SHOWDEFS_RESPONSE << " command" << endl;
    OPeNDAPCommand::add_command( SHOWDEFS_RESPONSE, OPeNDAPCommand::TermCommand ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << CATALOG_RESPONSE << " command" << endl;
    cmd = new OPeNDAPCatalogCommand( CATALOG_RESPONSE ) ;
    OPeNDAPCommand::add_command( CATALOG_RESPONSE, cmd ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << SHOW_INFO_RESPONSE << " command" << endl;
    cmd = new OPeNDAPCatalogCommand( SHOW_INFO_RESPONSE ) ;
    OPeNDAPCommand::add_command( SHOW_INFO_RESPONSE, cmd ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << DEFINE_RESPONSE << " command" << endl;
    cmd = new OPeNDAPDefineCommand( DEFINE_RESPONSE ) ;
    OPeNDAPCommand::add_command( DEFINE_RESPONSE, cmd ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << SETCONTAINER << " command" << endl;
    cmd = new OPeNDAPSetContainerCommand( SETCONTAINER ) ;
    OPeNDAPCommand::add_command( SETCONTAINER, cmd ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << DELETE_CONTAINER << " command" << endl;
    cmd = new OPeNDAPDelContainerCommand( DELETE_CONTAINER ) ;
    OPeNDAPCommand::add_command( DELETE_CONTAINER, cmd ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << DELETE_CONTAINERS << " command" << endl;
    cmd = new OPeNDAPDelContainersCommand( DELETE_CONTAINERS ) ;
    OPeNDAPCommand::add_command( DELETE_CONTAINERS, cmd ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << DELETE_DEFINITION << " command" << endl;
    cmd = new OPeNDAPDelDefCommand( DELETE_DEFINITION ) ;
    OPeNDAPCommand::add_command( DELETE_DEFINITION, cmd ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << DELETE_DEFINITIONS << " command" << endl;
    cmd = new OPeNDAPDelDefsCommand( DELETE_DEFINITIONS ) ;
    OPeNDAPCommand::add_command( DELETE_DEFINITIONS, cmd ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding parser exception callback" << endl ;
    DODS::add_ehm_callback( OPeNDAPParserException::handleException ) ;
}

void
DAPCommandModule::terminate()
{
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "Removing DAP Commands" << endl;

    OPeNDAPCommand *cmd = NULL ;

    (*DODSLog::TheLog()) << "Removing DEFINE_RESPONSE" << endl;
    cmd = OPeNDAPCommand::rem_command( DEFINE_RESPONSE ) ;
    if( cmd ) delete cmd ;

    (*DODSLog::TheLog()) << "Removing SHOWCONTAINERS_RESPONSE" << endl;
    cmd = OPeNDAPCommand::rem_command( SHOWCONTAINERS_RESPONSE ) ;
    if( cmd ) delete cmd ;

    (*DODSLog::TheLog()) << "Removing DELETE_CONTAINER" << endl;
    cmd = OPeNDAPCommand::rem_command( DELETE_CONTAINER ) ;
    if( cmd ) delete cmd ;

    (*DODSLog::TheLog()) << "Removing DELETE_CONTAINERS" << endl;
    cmd = OPeNDAPCommand::rem_command( DELETE_CONTAINERS ) ;
    if( cmd ) delete cmd ;

    (*DODSLog::TheLog()) << "Removing DELETE_DEFINITION" << endl;
    cmd = OPeNDAPCommand::rem_command( DELETE_DEFINITION ) ;
    if( cmd ) delete cmd ;

    (*DODSLog::TheLog()) << "Removing DELETE_DEFINITIONS" << endl;
    cmd = OPeNDAPCommand::rem_command( DELETE_DEFINITIONS ) ;
    if( cmd ) delete cmd ;
}

extern "C"
{
    OPeNDAPAbstractModule *maker()
    {
	return new DAPCommandModule ;
    }
}

