// BESModule.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu>
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

#include "BESModule.h"
#include "BESLog.h"

#include "BESResponseNames.h"
#include "BESResponseHandlerList.h"

#include "BESDASResponseHandler.h"
#include "BESDDSResponseHandler.h"
#include "BESDataResponseHandler.h"
#include "BESDDXResponseHandler.h"

#include "BESShowContainersResponseHandler.h"
#include "BESShowDefsResponseHandler.h"
#include "BESCatalogResponseHandler.h"

#include "BESDefineResponseHandler.h"
#include "BESSetContainerResponseHandler.h"
#include "BESDelContainerResponseHandler.h"
#include "BESDelContainersResponseHandler.h"
#include "BESDelDefResponseHandler.h"
#include "BESDelDefsResponseHandler.h"

#include "BESContainerStorageList.h"
#include "BESContainerStorageVolatile.h"
#include "BESDefinitionStorageList.h"
#include "BESDefinitionStorageVolatile.h"

void
BESModule::initialize( const string &modname )
{
    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "Initializing OPeNDAP modules:" << endl;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << DAS_RESPONSE << " response handler" << endl;
    BESResponseHandlerList::TheList()->add_handler( DAS_RESPONSE, BESDASResponseHandler::DASResponseBuilder ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << DDS_RESPONSE << " response handler" << endl;
    BESResponseHandlerList::TheList()->add_handler( DDS_RESPONSE, BESDDSResponseHandler::DDSResponseBuilder ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << DDX_RESPONSE << " response handler" << endl;
    BESResponseHandlerList::TheList()->add_handler( DDX_RESPONSE, BESDDXResponseHandler::DDXResponseBuilder ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << DATA_RESPONSE << " response handler" << endl;
    BESResponseHandlerList::TheList()->add_handler( DATA_RESPONSE, BESDataResponseHandler::DataResponseBuilder ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << SHOWCONTAINERS_RESPONSE << " response handler" << endl;
    BESResponseHandlerList::TheList()->add_handler( SHOWCONTAINERS_RESPONSE, BESShowContainersResponseHandler::ShowContainersResponseBuilder ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << SHOWDEFS_RESPONSE << " response handler" << endl;
    BESResponseHandlerList::TheList()->add_handler( SHOWDEFS_RESPONSE, BESShowDefsResponseHandler::ShowDefsResponseBuilder ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << CATALOG_RESPONSE << " response handler" << endl;
    BESResponseHandlerList::TheList()->add_handler( CATALOG_RESPONSE, BESCatalogResponseHandler::CatalogResponseBuilder ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << DEFINE_RESPONSE << " response handler" << endl;
    BESResponseHandlerList::TheList()->add_handler( DEFINE_RESPONSE, BESDefineResponseHandler::DefineResponseBuilder ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << SETCONTAINER << " response handler" << endl;
    BESResponseHandlerList::TheList()->add_handler( SETCONTAINER, BESSetContainerResponseHandler::SetContainerResponseBuilder ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << DELETE_CONTAINER << " response handler" << endl;
    BESResponseHandlerList::TheList()->add_handler( DELETE_CONTAINER, BESDelContainerResponseHandler::DelContainerResponseBuilder ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << DELETE_CONTAINERS << " response handler" << endl;
    BESResponseHandlerList::TheList()->add_handler( DELETE_CONTAINERS, BESDelContainersResponseHandler::DelContainersResponseBuilder ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << DELETE_DEFINITION << " response handler" << endl;
    BESResponseHandlerList::TheList()->add_handler( DELETE_DEFINITION, BESDelDefResponseHandler::DelDefResponseBuilder ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << DELETE_DEFINITIONS << " response handler" << endl;
    BESResponseHandlerList::TheList()->add_handler( DELETE_DEFINITIONS, BESDelDefsResponseHandler::DelDefsResponseBuilder ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << PERSISTENCE_VOLATILE << " container persistence" << endl ;
    BESContainerStorageList::TheList()->add_persistence( new BESContainerStorageVolatile( PERSISTENCE_VOLATILE ) ) ;

    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "    adding " << PERSISTENCE_VOLATILE << " definition persistence" << endl ;
    BESDefinitionStorageList::TheList()->add_persistence( new BESDefinitionStorageVolatile( PERSISTENCE_VOLATILE ) ) ;

}

void
BESModule::terminate( const string &modname )
{
    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "Removing OPeNDAP modules" << endl;

    BESResponseHandlerList::TheList()->remove_handler( DAS_RESPONSE ) ;
    BESResponseHandlerList::TheList()->remove_handler( DDS_RESPONSE ) ;
    BESResponseHandlerList::TheList()->remove_handler( DDX_RESPONSE ) ;
    BESResponseHandlerList::TheList()->remove_handler( DATA_RESPONSE ) ;
    BESResponseHandlerList::TheList()->remove_handler( SHOWCONTAINERS_RESPONSE ) ;
    BESResponseHandlerList::TheList()->remove_handler( SHOWDEFS_RESPONSE ) ;
    BESResponseHandlerList::TheList()->remove_handler( CATALOG_RESPONSE ) ;
    BESResponseHandlerList::TheList()->remove_handler( DEFINE_RESPONSE ) ;
    BESResponseHandlerList::TheList()->remove_handler( SETCONTAINER ) ;
    BESResponseHandlerList::TheList()->remove_handler( DELETE_RESPONSE ) ;

    BESContainerStorageList::TheList()->del_persistence( PERSISTENCE_VOLATILE ) ;
    BESDefinitionStorageList::TheList()->del_persistence( PERSISTENCE_VOLATILE ) ;
}

extern "C"
{
    BESAbstractModule *maker()
    {
	return new BESModule ;
    }
}

