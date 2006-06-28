// BESDelContainersResponseHandler.cc

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

#include "BESDelContainersResponseHandler.h"
#include "BESSilentInfo.h"
#include "BESDefinitionStorageList.h"
#include "BESDefinitionStorage.h"
#include "BESDefine.h"
#include "BESContainerStorageList.h"
#include "BESContainerStorage.h"
#include "BESContainer.h"
#include "BESDataNames.h"
#include "BESHandlerException.h"

BESDelContainersResponseHandler::BESDelContainersResponseHandler( string name )
    : BESResponseHandler( name )
{
}

BESDelContainersResponseHandler::~BESDelContainersResponseHandler( )
{
}

/** @brief executes the command to delete all containers from a specified
 * container store.
 *
 * Removes all containers from a specified container storage found in 
 * BESContainerStorageList::TheList().
 *
 * The response object built is a BESInfo object. Status of the deletion
 * will be added to the informational object, one of:
 *
 * Successfully deleted all containers
 * <BR />
 * Container storage "&lt;store_name&gt;" does not exist. Unable to delete containers
 *
 * @param dhi structure that holds request and response information
 * @throws BESHandlerException if there is a problem building the
 * response object
 * @throws BESResponseException upon fatal error building the response
 * object
 * @see _BESDataHandlerInterface
 * @see BESInfo
 * @see BESDefinitionStorageList
 * @see BESDefine
 * @see BESContainerStorage
 * @see BESContainerStorageList
 */
void
BESDelContainersResponseHandler::execute( BESDataHandlerInterface &dhi )
{
    BESInfo *info = new BESSilentInfo() ;
    _response = info ;

    string store_name = dhi.data[STORE_NAME] ;
    if( store_name == "" )
    {
	store_name = PERSISTENCE_VOLATILE ;
    }
    BESContainerStorage *cp =
	BESContainerStorageList::TheList()->find_persistence( store_name ) ;
    if( cp )
    {
	bool deleted =  cp->del_containers( ) ;
	if( !deleted )
	{
	    string line = (string)"Unable to delete containers from \""
			  + dhi.data[STORE_NAME]
			  + "\ container store" ;
	    throw BESHandlerException( line, __FILE__, __LINE__ ) ;
	}
    }
    else
    {
	string line = (string)"Container storage \""
		      + dhi.data[STORE_NAME]
		      + "\" does not exist. "
		      + "Unable to delete containers" ;
	throw BESHandlerException( line, __FILE__, __LINE__ ) ;
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
 * @see BESInfo
 * @see BESTransmitter
 * @see _BESDataHandlerInterface
 */
void
BESDelContainersResponseHandler::transmit( BESTransmitter *transmitter,
                               BESDataHandlerInterface &dhi )
{
    if( _response )
    {
	BESInfo *info = dynamic_cast<BESInfo *>(_response) ;
	info->transmit( transmitter, dhi ) ;
    }
}

BESResponseHandler *
BESDelContainersResponseHandler::DelContainersResponseBuilder( string handler_name )
{
    return new BESDelContainersResponseHandler( handler_name ) ;
}
