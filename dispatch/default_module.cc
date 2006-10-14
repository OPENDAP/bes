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

#include "BESResponseNames.h"
#include "BESResponseHandlerList.h"

#include "BESHelpResponseHandler.h"
#include "BESProcIdResponseHandler.h"
#include "BESVersionResponseHandler.h"
#include "BESKeysResponseHandler.h"
#include "BESStatusResponseHandler.h"
#include "BESStreamResponseHandler.h"

#include "BESTransmitterNames.h"
#include "BESReturnManager.h"
#include "BESBasicTransmitter.h"
#include "BESBasicHttpTransmitter.h"

#include "BESLog.h"

#include "BESTextInfo.h"
#include "BESHTMLInfo.h"
#include "BESXMLInfo.h"
#include "BESInfoList.h"
#include "BESInfoNames.h"

int
default_module::initialize(int, char**)
{
    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "Initializing default modules:" << endl;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << HELP_RESPONSE << " response handler" << endl;
    BESResponseHandlerList::TheList()->add_handler( HELP_RESPONSE, BESHelpResponseHandler::HelpResponseBuilder ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << PROCESS_RESPONSE << " response handler" << endl;
    BESResponseHandlerList::TheList()->add_handler( PROCESS_RESPONSE, BESProcIdResponseHandler::ProcIdResponseBuilder ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << KEYS_RESPONSE << " response handler" << endl;
    BESResponseHandlerList::TheList()->add_handler( KEYS_RESPONSE, BESKeysResponseHandler::KeysResponseBuilder ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << VERS_RESPONSE << " response handler" << endl;
    BESResponseHandlerList::TheList()->add_handler( VERS_RESPONSE, BESVersionResponseHandler::VersionResponseBuilder ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << STATUS_RESPONSE << " response handler" << endl;
    BESResponseHandlerList::TheList()->add_handler( STATUS_RESPONSE, BESStatusResponseHandler::StatusResponseBuilder ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << STREAM_RESPONSE << " response handler" << endl;
    BESResponseHandlerList::TheList()->add_handler( STREAM_RESPONSE, BESStreamResponseHandler::BESStreamResponseBuilder ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << BASIC_TRANSMITTER << " transmitter" << endl;
    BESReturnManager::TheManager()->add_transmitter( BASIC_TRANSMITTER, new BESBasicTransmitter ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << HTTP_TRANSMITTER << " transmitter" << endl;
    BESReturnManager::TheManager()->add_transmitter( HTTP_TRANSMITTER, new BESBasicHttpTransmitter ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << BES_TEXT_INFO << " info builder" << endl;
    BESInfoList::TheList()->add_info_builder( BES_TEXT_INFO,
					      BESTextInfo::BuildTextInfo ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << BES_HTML_INFO << " info builder" << endl;
    BESInfoList::TheList()->add_info_builder( BES_HTML_INFO,
					      BESHTMLInfo::BuildHTMLInfo ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << BES_XML_INFO << " info builder" << endl;
    BESInfoList::TheList()->add_info_builder( BES_XML_INFO,
					      BESXMLInfo::BuildXMLInfo ) ;

    return 0 ;
}

int
default_module::terminate(void)
{
    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "Removing default modules" << endl ;

    BESResponseHandlerList::TheList()->remove_handler( HELP_RESPONSE ) ;
    BESResponseHandlerList::TheList()->remove_handler( VERS_RESPONSE ) ;
    BESResponseHandlerList::TheList()->remove_handler( PROCESS_RESPONSE ) ;
    BESResponseHandlerList::TheList()->remove_handler( KEYS_RESPONSE ) ;
    BESResponseHandlerList::TheList()->remove_handler( STATUS_RESPONSE ) ;

    BESReturnManager::TheManager()->del_transmitter( BASIC_TRANSMITTER ) ;
    BESReturnManager::TheManager()->del_transmitter( HTTP_TRANSMITTER ) ;

    return 0 ;
}

