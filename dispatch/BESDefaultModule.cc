// BESDefaultModule.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
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

#include "BESDefaultModule.h"

#include "BESResponseNames.h"
#include "BESResponseHandlerList.h"

#include "BESHelpResponseHandler.h"
#include "BESVersionResponseHandler.h"

#ifdef BES_DEVELOPER
#include "BESProcIdResponseHandler.h"
#include "BESConfigResponseHandler.h"
#endif

#include "BESStatusResponseHandler.h"
#include "BESServicesResponseHandler.h"
#include "BESStreamResponseHandler.h"

#include "BESSetContainerResponseHandler.h"
#include "BESShowContainersResponseHandler.h"
#include "BESDelContainerResponseHandler.h"
#include "BESDelContainersResponseHandler.h"
#include "BESContainerStorageList.h"
#include "BESContainerStorageVolatile.h"

#include "BESDefineResponseHandler.h"
#include "BESShowDefsResponseHandler.h"
#include "BESDelDefResponseHandler.h"
#include "BESDelDefsResponseHandler.h"
#include "BESDefinitionStorageList.h"
#include "BESDefinitionStorageVolatile.h"

#include "BESSetContextResponseHandler.h"
#include "BESShowContextResponseHandler.h"
#include "BESShowErrorResponseHandler.h"

#include "BESTransmitterNames.h"
#include "BESReturnManager.h"
#include "BESTransmitter.h"

#include "BESDebug.h"

#include "BESTextInfo.h"
#include "BESHTMLInfo.h"
#include "BESXMLInfo.h"
#include "BESInfoList.h"
#include "BESInfoNames.h"

#include "RemoteAccess.h"

int
BESDefaultModule::initialize(int, char**)
{
    BESDEBUG( "bes", "Initializing default modules:" << endl ) ;

    BESDEBUG( "bes", "    adding " << HELP_RESPONSE << " response handler" << endl ) ;
    BESResponseHandlerList::TheList()->add_handler( HELP_RESPONSE, BESHelpResponseHandler::HelpResponseBuilder ) ;

#ifdef BES_DEVELOPER
    BESDEBUG( "bes", "    adding " << PROCESS_RESPONSE << " response handler" << endl ) ;
    BESResponseHandlerList::TheList()->add_handler( PROCESS_RESPONSE, BESProcIdResponseHandler::ProcIdResponseBuilder ) ;

    BESDEBUG( "bes", "    adding " << CONFIG_RESPONSE << " response handler" << endl ) ;
    BESResponseHandlerList::TheList()->add_handler( CONFIG_RESPONSE, BESConfigResponseHandler::ConfigResponseBuilder ) ;
#endif

    BESDEBUG( "bes", "    adding " << VERS_RESPONSE << " response handler" << endl ) ;
    BESResponseHandlerList::TheList()->add_handler( VERS_RESPONSE, BESVersionResponseHandler::VersionResponseBuilder ) ;

    BESDEBUG( "bes", "    adding " << STATUS_RESPONSE << " response handler" << endl ) ;
    BESResponseHandlerList::TheList()->add_handler( STATUS_RESPONSE, BESStatusResponseHandler::StatusResponseBuilder ) ;

    BESDEBUG( "bes", "    adding " << SERVICE_RESPONSE << " response handler" << endl ) ;
    BESResponseHandlerList::TheList()->add_handler( SERVICE_RESPONSE, BESServicesResponseHandler::ResponseBuilder ) ;

    BESDEBUG( "bes", "    adding " << STREAM_RESPONSE << " response handler" << endl ) ;
    BESResponseHandlerList::TheList()->add_handler( STREAM_RESPONSE, BESStreamResponseHandler::BESStreamResponseBuilder ) ;

    BESDEBUG( "bes", "    adding " << SETCONTAINER << " response handler" << endl ) ;
    BESResponseHandlerList::TheList()->add_handler( SETCONTAINER, BESSetContainerResponseHandler::SetContainerResponseBuilder ) ;

    BESDEBUG( "bes", "    adding " << SHOWCONTAINERS_RESPONSE << " response handler" << endl ) ;
    BESResponseHandlerList::TheList()->add_handler( SHOWCONTAINERS_RESPONSE, BESShowContainersResponseHandler::ShowContainersResponseBuilder ) ;

    BESDEBUG( "bes", "    adding " << DELETE_CONTAINER << " response handler" << endl ) ;
    BESResponseHandlerList::TheList()->add_handler( DELETE_CONTAINER, BESDelContainerResponseHandler::DelContainerResponseBuilder ) ;

    BESDEBUG( "bes", "    adding " << DELETE_CONTAINERS << " response handler" << endl) ;
    BESResponseHandlerList::TheList()->add_handler( DELETE_CONTAINERS, BESDelContainersResponseHandler::DelContainersResponseBuilder ) ;

    BESDEBUG( "bes", "    adding " << PERSISTENCE_VOLATILE << " container persistence" << endl ) ;
    BESContainerStorageList::TheList()->add_persistence( new BESContainerStorageVolatile( PERSISTENCE_VOLATILE ) ) ;

    BESDEBUG( "bes", "    adding " << DEFINE_RESPONSE << " response handler" << endl ) ;
    BESResponseHandlerList::TheList()->add_handler( DEFINE_RESPONSE, BESDefineResponseHandler::DefineResponseBuilder ) ;

    BESDEBUG( "bes", "    adding " << SHOWDEFS_RESPONSE << " response handler" << endl) ;
    BESResponseHandlerList::TheList()->add_handler( SHOWDEFS_RESPONSE, BESShowDefsResponseHandler::ShowDefsResponseBuilder ) ;

    BESDEBUG( "bes", "    adding " << DELETE_DEFINITION << " response handler" << endl) ;
    BESResponseHandlerList::TheList()->add_handler( DELETE_DEFINITION, BESDelDefResponseHandler::DelDefResponseBuilder ) ;

    BESDEBUG( "bes", "    adding " << DELETE_DEFINITIONS << " response handler" << endl ) ;
    BESResponseHandlerList::TheList()->add_handler( DELETE_DEFINITIONS, BESDelDefsResponseHandler::DelDefsResponseBuilder ) ;

    BESDEBUG( "bes", "    adding " << PERSISTENCE_VOLATILE << " definition persistence" << endl ) ;
    BESDefinitionStorageList::TheList()->add_persistence( new BESDefinitionStorageVolatile( PERSISTENCE_VOLATILE ) ) ;

    BESDEBUG( "bes", "    adding " << SET_CONTEXT << " response handler" << endl) ;
    BESResponseHandlerList::TheList()->add_handler( SET_CONTEXT, BESSetContextResponseHandler::SetContextResponseBuilder ) ;

#if 0
    // Moved this to the xmlcommand code that loads the commands. It can be in either place, but
    // it's easier to see how the commands are built if they are in there. jhrg 2/9/18
    BESDEBUG( "bes", "    adding " << SET_CONTEXTS_ACTION << " response handler" << endl) ;
    BESResponseHandlerList::TheList()->add_handler(SET_CONTEXTS_ACTION, SetContextsResponseHandler::SetContextsResponseBuilder ) ;
#endif

    BESDEBUG( "bes", "    adding " << SHOW_CONTEXT << " response handler" << endl) ;
    BESResponseHandlerList::TheList()->add_handler( SHOW_CONTEXT, BESShowContextResponseHandler::ShowContextResponseBuilder ) ;

    BESDEBUG( "bes", "    adding " << SHOW_ERROR << " response handler" << endl) ;
    BESResponseHandlerList::TheList()->add_handler( SHOW_ERROR, BESShowErrorResponseHandler::ResponseBuilder ) ;

    BESDEBUG( "bes", "    adding " << BASIC_TRANSMITTER << " transmitter" << endl ) ;
    BESReturnManager::TheManager()->add_transmitter( BASIC_TRANSMITTER, new BESTransmitter ) ;

    BESDEBUG( "bes", "    adding " << BES_TEXT_INFO << " info builder" << endl ) ;
    BESInfoList::TheList()->add_info_builder( BES_TEXT_INFO,
					      BESTextInfo::BuildTextInfo ) ;

    BESDEBUG( "bes", "    adding " << BES_HTML_INFO << " info builder" << endl ) ;
    BESInfoList::TheList()->add_info_builder( BES_HTML_INFO,
					      BESHTMLInfo::BuildHTMLInfo ) ;

    BESDEBUG( "bes", "    adding " << BES_XML_INFO << " info builder" << endl ) ;
    BESInfoList::TheList()->add_info_builder( BES_XML_INFO,
					      BESXMLInfo::BuildXMLInfo ) ;

    BESDEBUG( "bes", "    adding bes debug context" << endl ) ;
    BESDebug::Register( "bes" ) ;


    BESDEBUG( "bes", "    initializing RemoteAccess" << endl ) ;
    bes::RemoteAccess::Initialize();



    BESDEBUG( "bes", "Done Initializing default modules:" << endl ) ;

    return 0 ;
}

int
BESDefaultModule::terminate(void)
{
    BESDEBUG( "bes", "Removing default modules" << endl ) ;

#ifdef BES_DEVELOPER
    BESResponseHandlerList::TheList()->remove_handler( PROCESS_RESPONSE ) ;
    BESResponseHandlerList::TheList()->remove_handler( CONFIG_RESPONSE ) ;
#endif

    BESResponseHandlerList::TheList()->remove_handler( VERS_RESPONSE ) ;
    BESResponseHandlerList::TheList()->remove_handler( STATUS_RESPONSE ) ;
    BESResponseHandlerList::TheList()->remove_handler( SERVICE_RESPONSE ) ;
    BESResponseHandlerList::TheList()->remove_handler( STREAM_RESPONSE ) ;
    BESResponseHandlerList::TheList()->remove_handler( SETCONTAINER ) ;
    BESResponseHandlerList::TheList()->remove_handler( SHOWCONTAINERS_RESPONSE ) ;
    BESResponseHandlerList::TheList()->remove_handler( DELETE_CONTAINER ) ;
    BESResponseHandlerList::TheList()->remove_handler( DELETE_CONTAINERS ) ;

    BESContainerStorageList::TheList()->deref_persistence( PERSISTENCE_VOLATILE ) ;

    BESResponseHandlerList::TheList()->remove_handler( DEFINE_RESPONSE ) ;
    BESResponseHandlerList::TheList()->remove_handler( SHOWDEFS_RESPONSE ) ;
    BESResponseHandlerList::TheList()->remove_handler( DELETE_DEFINITION ) ;
    BESResponseHandlerList::TheList()->remove_handler( DELETE_DEFINITIONS ) ;

    BESDefinitionStorageList::TheList()->deref_persistence( PERSISTENCE_VOLATILE ) ;

    BESResponseHandlerList::TheList()->remove_handler( SET_CONTEXT ) ;
    BESResponseHandlerList::TheList()->remove_handler( SHOW_CONTEXT ) ;
    BESResponseHandlerList::TheList()->remove_handler( SHOW_ERROR ) ;

    BESReturnManager::TheManager()->del_transmitter( BASIC_TRANSMITTER ) ;


    BESInfoList::TheList()->rem_info_builder( BES_TEXT_INFO ) ;
    BESInfoList::TheList()->rem_info_builder( BES_HTML_INFO ) ;
    BESInfoList::TheList()->rem_info_builder( BES_XML_INFO ) ;

    BESDEBUG( "bes", "Done Removing default modules" << endl ) ;

    return 0 ;
}

