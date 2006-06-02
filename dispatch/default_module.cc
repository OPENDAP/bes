// default_module.cc

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

#include "default_module.h"

#include "DODSResponseNames.h"
#include "DODSResponseHandlerList.h"

#include "HelpResponseHandler.h"
#include "ProcIdResponseHandler.h"
#include "VersionResponseHandler.h"
#include "KeysResponseHandler.h"
#include "StatusResponseHandler.h"

#include "OPeNDAPTransmitterNames.h"
#include "DODSReturnManager.h"
#include "DODSBasicTransmitter.h"
#include "DODSBasicHttpTransmitter.h"

#include "DODSLog.h"

int
default_module::initialize(int, char**)
{
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "Initializing default modules:" << endl;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << HELP_RESPONSE << " response handler" << endl;
    DODSResponseHandlerList::TheList()->add_handler( HELP_RESPONSE, HelpResponseHandler::HelpResponseBuilder ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << PROCESS_RESPONSE << " response handler" << endl;
    DODSResponseHandlerList::TheList()->add_handler( PROCESS_RESPONSE, ProcIdResponseHandler::ProcIdResponseBuilder ) ;

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
	(*DODSLog::TheLog()) << "    adding " << BASIC_TRANSMITTER << " transmitter" << endl;
    DODSReturnManager::TheManager()->add_transmitter( BASIC_TRANSMITTER, new DODSBasicTransmitter ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << HTTP_TRANSMITTER << " transmitter" << endl;
    DODSReturnManager::TheManager()->add_transmitter( HTTP_TRANSMITTER, new DODSBasicHttpTransmitter ) ;

    return 0 ;
}

int
default_module::terminate(void)
{
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "Removing default modules" << endl ;

    DODSResponseHandlerList::TheList()->remove_handler( HELP_RESPONSE ) ;
    DODSResponseHandlerList::TheList()->remove_handler( VERS_RESPONSE ) ;
    DODSResponseHandlerList::TheList()->remove_handler( PROCESS_RESPONSE ) ;
    DODSResponseHandlerList::TheList()->remove_handler( KEYS_RESPONSE ) ;
    DODSResponseHandlerList::TheList()->remove_handler( STATUS_RESPONSE ) ;

    DODSReturnManager::TheManager()->del_transmitter( BASIC_TRANSMITTER ) ;
    DODSReturnManager::TheManager()->del_transmitter( HTTP_TRANSMITTER ) ;

    return 0 ;
}

