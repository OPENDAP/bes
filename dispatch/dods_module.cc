// dods_module.cc

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

#include "dods_module.h"

#include "DODSResponseNames.h"
#include "DODSResponseHandlerList.h"

#include "DASResponseHandler.h"
#include "DDSResponseHandler.h"
#include "DataResponseHandler.h"
#include "DDXResponseHandler.h"

#include "HelpResponseHandler.h"
#include "ProcIdResponseHandler.h"
#include "VersionResponseHandler.h"
#include "ContainersResponseHandler.h"
#include "DefinitionsResponseHandler.h"
#include "KeysResponseHandler.h"
#include "StatusResponseHandler.h"
#include "CatalogResponseHandler.h"

#include "DefineResponseHandler.h"

#include "SetResponseHandler.h"

#include "DeleteResponseHandler.h"

#include "OPeNDAPTransmitterNames.h"
#include "DODSReturnManager.h"
#include "DODSBasicTransmitter.h"
#include "DODSBasicHttpTransmitter.h"
#include "ContainerStorageList.h"
#include "ContainerStorageVolatile.h"

#include "DODSLog.h"

int
dods_module::initialize(int, char**)
{
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "Initializing DODS:" << endl;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << DAS_RESPONSE << " response handler" << endl;
    DODSResponseHandlerList::TheList()->add_handler( DAS_RESPONSE, DASResponseHandler::DASResponseBuilder ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << DDS_RESPONSE << " response handler" << endl;
    DODSResponseHandlerList::TheList()->add_handler( DDS_RESPONSE, DDSResponseHandler::DDSResponseBuilder ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << DDX_RESPONSE << " response handler" << endl;
    DODSResponseHandlerList::TheList()->add_handler( DDX_RESPONSE, DDXResponseHandler::DDXResponseBuilder ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << DATA_RESPONSE << " response handler" << endl;
    DODSResponseHandlerList::TheList()->add_handler( DATA_RESPONSE, DataResponseHandler::DataResponseBuilder ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << HELP_RESPONSE << " response handler" << endl;
    DODSResponseHandlerList::TheList()->add_handler( HELP_RESPONSE, HelpResponseHandler::HelpResponseBuilder ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << PROCESS_RESPONSE << " response handler" << endl;
    DODSResponseHandlerList::TheList()->add_handler( PROCESS_RESPONSE, ProcIdResponseHandler::ProcIdResponseBuilder ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << CONTAINERS_RESPONSE << " response handler" << endl;
    DODSResponseHandlerList::TheList()->add_handler( CONTAINERS_RESPONSE, ContainersResponseHandler::ContainersResponseBuilder ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << DEFINITIONS_RESPONSE << " response handler" << endl;
    DODSResponseHandlerList::TheList()->add_handler( DEFINITIONS_RESPONSE, DefinitionsResponseHandler::DefinitionsResponseBuilder ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << KEYS_RESPONSE << " response handler" << endl;
    DODSResponseHandlerList::TheList()->add_handler( KEYS_RESPONSE, KeysResponseHandler::KeysResponseBuilder ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << VERS_RESPONSE << " response handler" << endl;
    DODSResponseHandlerList::TheList()->add_handler( VERS_RESPONSE, VersionResponseHandler::VersionResponseBuilder ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << STATUS_RESPONSE << " response handler" << endl;
    DODSResponseHandlerList::TheList()->add_handler( STATUS_RESPONSE, StatusResponseHandler::StatusResponseBuilder ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << CATALOG_RESPONSE << " response handler" << endl;
    DODSResponseHandlerList::TheList()->add_handler( CATALOG_RESPONSE, CatalogResponseHandler::CatalogResponseBuilder ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << DEFINE_RESPONSE << " response handler" << endl;
    DODSResponseHandlerList::TheList()->add_handler( DEFINE_RESPONSE, DefineResponseHandler::DefineResponseBuilder ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << SET_RESPONSE << " response handler" << endl;
    DODSResponseHandlerList::TheList()->add_handler( SET_RESPONSE, SetResponseHandler::SetResponseBuilder ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << DELETE_RESPONSE << " response handler" << endl;
    DODSResponseHandlerList::TheList()->add_handler( DELETE_RESPONSE, DeleteResponseHandler::DeleteResponseBuilder ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << BASIC_TRANSMITTER << " transmitter" << endl;
    DODSReturnManager::TheManager()->add_transmitter( BASIC_TRANSMITTER, new DODSBasicTransmitter ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << HTTP_TRANSMITTER << " transmitter" << endl;
    DODSReturnManager::TheManager()->add_transmitter( HTTP_TRANSMITTER, new DODSBasicHttpTransmitter ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << PERSISTENCE_VOLATILE << " persistence" << endl;
    ContainerStorageList::TheList()->add_persistence( new ContainerStorageVolatile( PERSISTENCE_VOLATILE ) ) ;
    return 0 ;
}

int
dods_module::terminate(void)
{
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "Removing DODS Response Handlers" << endl;

    DODSResponseHandlerList::TheList()->remove_handler( DAS_RESPONSE ) ;
    DODSResponseHandlerList::TheList()->remove_handler( DDS_RESPONSE ) ;
    DODSResponseHandlerList::TheList()->remove_handler( DDX_RESPONSE ) ;
    DODSResponseHandlerList::TheList()->remove_handler( DATA_RESPONSE ) ;
    DODSResponseHandlerList::TheList()->remove_handler( HELP_RESPONSE ) ;
    DODSResponseHandlerList::TheList()->remove_handler( VERS_RESPONSE ) ;

    return 0;
}

