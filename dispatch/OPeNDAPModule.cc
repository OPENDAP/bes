// OPeNDAPModule.cc

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

#include "OPeNDAPModule.h"
#include "DODSLog.h"

#include "DODSResponseNames.h"
#include "DODSResponseHandlerList.h"

#include "DASResponseHandler.h"
#include "DDSResponseHandler.h"
#include "DataResponseHandler.h"
#include "DDXResponseHandler.h"

#include "ShowContainersResponseHandler.h"
#include "ShowDefsResponseHandler.h"
#include "CatalogResponseHandler.h"

#include "DefineResponseHandler.h"
#include "SetContainerResponseHandler.h"
#include "DelContainerResponseHandler.h"
#include "DelContainersResponseHandler.h"
#include "DelDefResponseHandler.h"
#include "DelDefsResponseHandler.h"

#include "ContainerStorageList.h"
#include "ContainerStorageVolatile.h"
#include "DefinitionStorageList.h"
#include "DefinitionStorageVolatile.h"

void
OPeNDAPModule::initialize()
{
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "Initializing OPeNDAP modules:" << endl;

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
	(*DODSLog::TheLog()) << "    adding " << SHOWCONTAINERS_RESPONSE << " response handler" << endl;
    DODSResponseHandlerList::TheList()->add_handler( SHOWCONTAINERS_RESPONSE, ShowContainersResponseHandler::ShowContainersResponseBuilder ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << SHOWDEFS_RESPONSE << " response handler" << endl;
    DODSResponseHandlerList::TheList()->add_handler( SHOWDEFS_RESPONSE, ShowDefsResponseHandler::ShowDefsResponseBuilder ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << CATALOG_RESPONSE << " response handler" << endl;
    DODSResponseHandlerList::TheList()->add_handler( CATALOG_RESPONSE, CatalogResponseHandler::CatalogResponseBuilder ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << DEFINE_RESPONSE << " response handler" << endl;
    DODSResponseHandlerList::TheList()->add_handler( DEFINE_RESPONSE, DefineResponseHandler::DefineResponseBuilder ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << SETCONTAINER << " response handler" << endl;
    DODSResponseHandlerList::TheList()->add_handler( SETCONTAINER, SetContainerResponseHandler::SetContainerResponseBuilder ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << DELETE_CONTAINER << " response handler" << endl;
    DODSResponseHandlerList::TheList()->add_handler( DELETE_CONTAINER, DelContainerResponseHandler::DelContainerResponseBuilder ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << DELETE_CONTAINERS << " response handler" << endl;
    DODSResponseHandlerList::TheList()->add_handler( DELETE_CONTAINERS, DelContainersResponseHandler::DelContainersResponseBuilder ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << DELETE_DEFINITION << " response handler" << endl;
    DODSResponseHandlerList::TheList()->add_handler( DELETE_DEFINITION, DelDefResponseHandler::DelDefResponseBuilder ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << DELETE_DEFINITIONS << " response handler" << endl;
    DODSResponseHandlerList::TheList()->add_handler( DELETE_DEFINITIONS, DelDefsResponseHandler::DelDefsResponseBuilder ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << PERSISTENCE_VOLATILE << " container persistence" << endl ;
    ContainerStorageList::TheList()->add_persistence( new ContainerStorageVolatile( PERSISTENCE_VOLATILE ) ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << PERSISTENCE_VOLATILE << " definition persistence" << endl ;
    DefinitionStorageList::TheList()->add_persistence( new DefinitionStorageVolatile( PERSISTENCE_VOLATILE ) ) ;

}

void
OPeNDAPModule::terminate()
{
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "Removing OPeNDAP modules" << endl;

    DODSResponseHandlerList::TheList()->remove_handler( DAS_RESPONSE ) ;
    DODSResponseHandlerList::TheList()->remove_handler( DDS_RESPONSE ) ;
    DODSResponseHandlerList::TheList()->remove_handler( DDX_RESPONSE ) ;
    DODSResponseHandlerList::TheList()->remove_handler( DATA_RESPONSE ) ;
    DODSResponseHandlerList::TheList()->remove_handler( SHOWCONTAINERS_RESPONSE ) ;
    DODSResponseHandlerList::TheList()->remove_handler( SHOWDEFS_RESPONSE ) ;
    DODSResponseHandlerList::TheList()->remove_handler( CATALOG_RESPONSE ) ;
    DODSResponseHandlerList::TheList()->remove_handler( DEFINE_RESPONSE ) ;
    DODSResponseHandlerList::TheList()->remove_handler( SETCONTAINER ) ;
    DODSResponseHandlerList::TheList()->remove_handler( DELETE_RESPONSE ) ;

    ContainerStorageList::TheList()->del_persistence( PERSISTENCE_VOLATILE ) ;
    DefinitionStorageList::TheList()->del_persistence( PERSISTENCE_VOLATILE ) ;
}

extern "C"
{
    OPeNDAPAbstractModule *maker()
    {
	return new OPeNDAPModule ;
    }
}

