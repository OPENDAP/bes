// BESDefineResponseHandler.cc

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

using std::cout ;
using std::endl ;

#include "BESDefineResponseHandler.h"
#include "BESSilentInfo.h"
#include "BESDefine.h"
#include "BESDefinitionStorageList.h"
#include "BESDefinitionStorage.h"
#include "BESDataNames.h"
#include "BESHandlerException.h"
#include "BESResponseNames.h"

BESDefineResponseHandler::BESDefineResponseHandler( string name )
    : BESResponseHandler( name )
{
}

BESDefineResponseHandler::~BESDefineResponseHandler( )
{
}

/** @brief executes the command to create a new definition.
 *
 * A BESDefine object is created and added to the list of definitions. If
 * a definition already exists with the given name then it is removed and
 * the new one added.
 *
 * The BESDefine object is created using the containers, constraints,
 * attribute lists and aggregation command parsed in the parse method.
 *
 * The response object built for this command is a BESInfo response
 * object. It will contain one of two possible responses:
 *
 * Successfully added definition &lt;def_name&gt;
 * <BR />
 * Successfully replaced definition &lt;def_name&gt;
 *
 * If the keyword SILENT is set within the data handler interface then no
 * information is added.
 *
 * @param dhi structure that holds request and response information
 * @throws BESHandlerException if there is a problem building the
 * response object
 * @throws BESResponseException upon fatal error building the response
 * object
 * @see _BESDataHandlerInterface
 * @see BESInfo
 * @see BESDefine
 * @see DefintionStorageList
 */
void
BESDefineResponseHandler::execute( BESDataHandlerInterface &dhi )
{
    dhi.action_name = DEFINE_RESPONSE_STR ;
    BESInfo *info = new BESSilentInfo() ;
    _response = info ;

    string def_name = dhi.data[DEF_NAME] ;
    string store_name = dhi.data[STORE_NAME] ;
    if( store_name == "" )
	store_name = PERSISTENCE_VOLATILE ;
    BESDefinitionStorage *store =
	BESDefinitionStorageList::TheList()->find_persistence( store_name ) ;
    if( store )
    {
	bool deleted = store->del_definition( def_name ) ;

	BESDefine *dd = new BESDefine ;
	dhi.first_container() ;
	while( dhi.container )
	{
	    dd->containers.push_back( *dhi.container ) ;
	    dhi.next_container() ;
	}
	dd->aggregation_command = dhi.data[AGG_CMD] ;
	dd->aggregation_handler = dhi.data[AGG_HANDLER] ;
	dhi.data[AGG_CMD] = "" ;
	dhi.data[AGG_HANDLER] = "" ;

	store->add_definition( def_name, dd ) ;
    }
    else
    {
	string err_str = (string)"Uanble to add definition \"" + def_name 
	                 + "\" to \"" + store_name
			 + "\" store. Store does not exist" ;
	throw BESHandlerException( err_str, __FILE__, __LINE__ ) ;
    }
}

/** @brief transmit the response object built by the execute command
 * using the specified transmitter object
 *
 * A BESInfo response object was built in the exeucte command to inform
 * the user whether the definition was successfully added or replaced. The
 * method send_text is called on the BESTransmitter object.
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see BESInfo
 * @see BESTransmitter
 * @see _BESDataHandlerInterface
 */
void
BESDefineResponseHandler::transmit( BESTransmitter *transmitter,
                                  BESDataHandlerInterface &dhi )
{
    if( _response )
    {
        // If this dynamic_cast is to a reference an not a pointer, then if
        // _response is not a BESInfo the cast will throw bad_cast. 
        // Casting to a pointer will make ti null on error. jhrg 11/10/2005
	BESInfo *info = dynamic_cast<BESInfo *>(_response) ;
	info->transmit( transmitter, dhi ) ;
    }
}

BESResponseHandler *
BESDefineResponseHandler::DefineResponseBuilder( string handler_name )
{
    return new BESDefineResponseHandler( handler_name ) ;
}
