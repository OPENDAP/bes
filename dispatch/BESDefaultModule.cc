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

#include "config.h"

#include <iostream>

#include "BESDefaultModule.h"

#include "BESNames.h"
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

#include "BESCatalogResponseHandler.h"
#include "ShowNodeResponseHandler.h"

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

using namespace std;
using namespace bes;

int BESDefaultModule::initialize(int, char**)
{
    BESDEBUG("bes", "Initializing default modules:" << endl);

    // Default places to store containers and definitions. Support both "default" and "catalog"
    //BESContainerStorageList::TheList()->add_persistence(new BESContainerStorageVolatile( DEFAULT ));
    //BESContainerStorageList::TheList()->add_persistence(new BESContainerStorageVolatile( CATALOG ));

    // This is the only place the Definition Storage is set. I set both DEFAULT and CATALOG so that
    // code that uses those names will work.
    // TODO Remove 'catalog' and change the way define command works to use DEFAULT by
    BESDefinitionStorageList::TheList()->add_persistence(new BESDefinitionStorageVolatile( DEFAULT ));
    BESDefinitionStorageList::TheList()->add_persistence(new BESDefinitionStorageVolatile( CATALOG ));

    BESResponseHandlerList::TheList()->add_handler( HELP_RESPONSE, BESHelpResponseHandler::HelpResponseBuilder);

#ifdef BES_DEVELOPER
    BESResponseHandlerList::TheList()->add_handler( PROCESS_RESPONSE, BESProcIdResponseHandler::ProcIdResponseBuilder );
    BESResponseHandlerList::TheList()->add_handler( CONFIG_RESPONSE, BESConfigResponseHandler::ConfigResponseBuilder );
#endif

    BESResponseHandlerList::TheList()->add_handler( VERS_RESPONSE, BESVersionResponseHandler::VersionResponseBuilder);
    BESResponseHandlerList::TheList()->add_handler( STATUS_RESPONSE, BESStatusResponseHandler::StatusResponseBuilder);
    BESResponseHandlerList::TheList()->add_handler( SERVICE_RESPONSE, BESServicesResponseHandler::ResponseBuilder);
    BESResponseHandlerList::TheList()->add_handler( STREAM_RESPONSE, BESStreamResponseHandler::BESStreamResponseBuilder);
    BESResponseHandlerList::TheList()->add_handler( SETCONTAINER, BESSetContainerResponseHandler::SetContainerResponseBuilder);
    BESResponseHandlerList::TheList()->add_handler( SHOWCONTAINERS_RESPONSE, BESShowContainersResponseHandler::ShowContainersResponseBuilder);
    BESResponseHandlerList::TheList()->add_handler( DELETE_CONTAINER, BESDelContainerResponseHandler::DelContainerResponseBuilder);
    BESResponseHandlerList::TheList()->add_handler( DELETE_CONTAINERS, BESDelContainersResponseHandler::DelContainersResponseBuilder);

    BESResponseHandlerList::TheList()->add_handler( CATALOG_RESPONSE, BESCatalogResponseHandler::CatalogResponseBuilder);
    BESResponseHandlerList::TheList()->add_handler( NODE_RESPONSE, ShowNodeResponseHandler::ShowNodeResponseBuilder);


    BESResponseHandlerList::TheList()->add_handler( DEFINE_RESPONSE, BESDefineResponseHandler::DefineResponseBuilder);
    BESResponseHandlerList::TheList()->add_handler( SHOWDEFS_RESPONSE, BESShowDefsResponseHandler::ShowDefsResponseBuilder);
    BESResponseHandlerList::TheList()->add_handler( DELETE_DEFINITION, BESDelDefResponseHandler::DelDefResponseBuilder);
    BESResponseHandlerList::TheList()->add_handler( DELETE_DEFINITIONS, BESDelDefsResponseHandler::DelDefsResponseBuilder);

    BESResponseHandlerList::TheList()->add_handler( SET_CONTEXT, BESSetContextResponseHandler::SetContextResponseBuilder);

#if 0
    // Moved this to the xmlcommand code that loads the commands. It can be in either place, but
    // it's easier to see how the commands are built if they are in there. jhrg 2/9/18
    BESResponseHandlerList::TheList()->add_handler(SET_CONTEXTS_ACTION, SetContextsResponseHandler::SetContextsResponseBuilder );
#endif

    BESResponseHandlerList::TheList()->add_handler( SHOW_CONTEXT, BESShowContextResponseHandler::ShowContextResponseBuilder);
    BESResponseHandlerList::TheList()->add_handler( SHOW_ERROR, BESShowErrorResponseHandler::ResponseBuilder);

    BESReturnManager::TheManager()->add_transmitter( BASIC_TRANSMITTER, new BESTransmitter);

    BESInfoList::TheList()->add_info_builder( BES_TEXT_INFO, BESTextInfo::BuildTextInfo);
    BESInfoList::TheList()->add_info_builder( BES_HTML_INFO, BESHTMLInfo::BuildHTMLInfo);
    BESInfoList::TheList()->add_info_builder( BES_XML_INFO, BESXMLInfo::BuildXMLInfo);

    BESDebug::Register("bes");

    BESDEBUG("bes", "Done Initializing default modules:" << endl);

    return 0;
}

int BESDefaultModule::terminate(void)
{
    BESDEBUG("bes", "Removing default modules" << endl);

#ifdef BES_DEVELOPER
    BESResponseHandlerList::TheList()->remove_handler( PROCESS_RESPONSE );
    BESResponseHandlerList::TheList()->remove_handler( CONFIG_RESPONSE );
#endif

    BESResponseHandlerList::TheList()->remove_handler( VERS_RESPONSE );
    BESResponseHandlerList::TheList()->remove_handler( STATUS_RESPONSE );
    BESResponseHandlerList::TheList()->remove_handler( SERVICE_RESPONSE );
    BESResponseHandlerList::TheList()->remove_handler( STREAM_RESPONSE );
    BESResponseHandlerList::TheList()->remove_handler( SETCONTAINER );
    BESResponseHandlerList::TheList()->remove_handler( SHOWCONTAINERS_RESPONSE );
    BESResponseHandlerList::TheList()->remove_handler( DELETE_CONTAINER );
    BESResponseHandlerList::TheList()->remove_handler( DELETE_CONTAINERS );

    BESResponseHandlerList::TheList()->remove_handler(CATALOG_RESPONSE );

    BESResponseHandlerList::TheList()->remove_handler( DEFINE_RESPONSE );
    BESResponseHandlerList::TheList()->remove_handler( SHOWDEFS_RESPONSE );
    BESResponseHandlerList::TheList()->remove_handler( DELETE_DEFINITION );
    BESResponseHandlerList::TheList()->remove_handler( DELETE_DEFINITIONS );

    BESResponseHandlerList::TheList()->remove_handler( SET_CONTEXT );
    BESResponseHandlerList::TheList()->remove_handler( SHOW_CONTEXT );
    BESResponseHandlerList::TheList()->remove_handler( SHOW_ERROR );

    BESReturnManager::TheManager()->del_transmitter( BASIC_TRANSMITTER );

    BESInfoList::TheList()->rem_info_builder( BES_TEXT_INFO );
    BESInfoList::TheList()->rem_info_builder( BES_HTML_INFO );
    BESInfoList::TheList()->rem_info_builder( BES_XML_INFO );

    BESContainerStorageList::TheList()->deref_persistence( DEFAULT );
    //BESContainerStorageList::TheList()->deref_persistence( CATALOG );

    BESDefinitionStorageList::TheList()->deref_persistence( DEFAULT );
    //BESDefinitionStorageList::TheList()->deref_persistence( CATALOG );

    BESDEBUG("bes", "Done Removing default modules" << endl);

    return 0;
}

