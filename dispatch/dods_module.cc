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

#include "DODSInitList.h"
#include "DODSInitOrder.h"
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

#include "DefineResponseHandler.h"

#include "SetResponseHandler.h"

#include "DeleteResponseHandler.h"

#include "DODSReturnManager.h"
#include "DODSBasicTransmitter.h"
#include "DODSContainerPersistenceList.h"
#include "DODSContainerPersistenceVolatile.h"

#include "DODSLog.h"

static bool
DODSModuleInit(int, char**) {
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
	(*DODSLog::TheLog()) << "    adding " << PERSISTENCE_VOLATILE << " persistence" << endl;
    DODSContainerPersistenceList::TheList()->add_persistence( new DODSContainerPersistenceVolatile( PERSISTENCE_VOLATILE ) ) ;
    return true;
}

static bool
DODSModuleTerm(void) {
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "Removing DODS Response Handlers" << endl;
    DODSResponseHandlerList::TheList()->remove_handler( DAS_RESPONSE ) ;
    DODSResponseHandlerList::TheList()->remove_handler( DDS_RESPONSE ) ;
    DODSResponseHandlerList::TheList()->remove_handler( DDX_RESPONSE ) ;
    DODSResponseHandlerList::TheList()->remove_handler( DATA_RESPONSE ) ;
    DODSResponseHandlerList::TheList()->remove_handler( HELP_RESPONSE ) ;
    DODSResponseHandlerList::TheList()->remove_handler( VERS_RESPONSE ) ;
    return true;
}

FUNINITQUIT(DODSModuleInit, DODSModuleTerm, DODSMODULE_INIT);

// $Log: dods_module.cc,v $
// Revision 1.7  2005/04/19 17:53:34  pwest
// added keys handler for show keys command
//
// Revision 1.6  2005/03/17 19:26:22  pwest
// added delete command to delete a specific container, a specific definition, or all definitions
//
// Revision 1.5  2005/03/15 19:55:36  pwest
// show containers and show definitions
//
// Revision 1.4  2005/02/09 19:46:55  pwest
// added basic transmitter and http transmitter to return manager
//
// Revision 1.3  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
