// DelContainersResponseHandler.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org> and Jose Garcia <jgarcia@ucar.org>
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

#include "DelContainersResponseHandler.h"
#include "DODSInfo.h"
#include "DefinitionStorageList.h"
#include "DefinitionStorage.h"
#include "DODSDefine.h"
#include "ContainerStorageList.h"
#include "ContainerStorage.h"
#include "DODSContainer.h"
#include "OPeNDAPDataNames.h"

DelContainersResponseHandler::DelContainersResponseHandler( string name )
    : DODSResponseHandler( name )
{
}

DelContainersResponseHandler::~DelContainersResponseHandler( )
{
}

/** @brief executes the command to delete all containers from a specified
 * container store.
 *
 * Removes all containers from a specified container storage found in 
 * ContainerStorageList::TheList().
 *
 * The response object built is a DODSInfo object. Status of the deletion
 * will be added to the informational object, one of:
 *
 * Successfully deleted all containers
 * <BR />
 * Container storage "&lt;store_name&gt;" does not exist. Unable to delete containers
 *
 * @param dhi structure that holds request and response information
 * @throws DODSHandlerException if there is a problem building the
 * response object
 * @throws DODSResponseException upon fatal error building the response
 * object
 * @see _DODSDataHandlerInterface
 * @see DODSInfo
 * @see DefinitionStorageList
 * @see DODSDefine
 * @see ContainerStorage
 * @see ContainerStorageList
 */
void
DelContainersResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    DODSInfo *info = new DODSInfo( dhi.transmit_protocol == "HTTP" ) ;
    _response = info ;

    string store_name = dhi.data[STORE_NAME] ;
    if( store_name == "" )
    {
	store_name = PERSISTENCE_VOLATILE ;
    }
    ContainerStorage *cp =
	ContainerStorageList::TheList()->find_persistence( store_name ) ;
    if( cp )
    {
	bool deleted =  cp->del_containers( ) ;
	if( deleted == true )
	{
	    string line = (string)"Successfully deleted container \""
			  + dhi.data[CONTAINER_NAME]
			  + "\" from container storage \""
			  + dhi.data[STORE_NAME]
			  + "\"\n" ;
	    info->add_data( line ) ;
	}
	else
	{
	    string line = (string)"Unable to delete containers from \""
			  + dhi.data[STORE_NAME]
			  + "\ container store\n" ;
	    info->add_data( line ) ;
	}
    }
    else
    {
	string line = (string)"Container storage \""
		      + dhi.data[STORE_NAME]
		      + "\" does not exist. "
		      + "Unable to delete containers\n" ;
	info->add_data( line ) ;
    }
}

/** @brief transmit the response object built by the execute command
 * using the specified transmitter object
 *
 * If a response object was built then transmit it as text using the specified
 * transmitter object.
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see DODSInfo
 * @see DODSTransmitter
 * @see _DODSDataHandlerInterface
 */
void
DelContainersResponseHandler::transmit( DODSTransmitter *transmitter,
                               DODSDataHandlerInterface &dhi )
{
    if( _response )
    {
	DODSInfo *info = dynamic_cast<DODSInfo *>(_response) ;
	transmitter->send_text( *info, dhi );
    }
}

DODSResponseHandler *
DelContainersResponseHandler::DelContainersResponseBuilder( string handler_name )
{
    return new DelContainersResponseHandler( handler_name ) ;
}

