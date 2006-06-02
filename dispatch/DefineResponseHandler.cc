// DefineResponseHandler.cc

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

#include "DefineResponseHandler.h"
#include "DODSInfo.h"
#include "OPeNDAPSilentInfo.h"
#include "DODSDefine.h"
#include "DefinitionStorageList.h"
#include "DefinitionStorage.h"
#include "OPeNDAPDataNames.h"

DefineResponseHandler::DefineResponseHandler( string name )
    : DODSResponseHandler( name )
{
}

DefineResponseHandler::~DefineResponseHandler( )
{
}

/** @brief executes the command to create a new definition.
 *
 * A DODSDefine object is created and added to the list of definitions. If
 * a definition already exists with the given name then it is removed and
 * the new one added.
 *
 * The DODSDefine object is created using the containers, constraints,
 * attribute lists and aggregation command parsed in the parse method.
 *
 * The response object built for this command is a DODSInfo response
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
 * @throws DODSHandlerException if there is a problem building the
 * response object
 * @throws DODSResponseException upon fatal error building the response
 * object
 * @see _DODSDataHandlerInterface
 * @see DODSInfo
 * @see DODSDefine
 * @see DefintionStorageList
 */
void
DefineResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    DODSInfo *info = 0 ;
    if( dhi.data[SILENT] == "yes" )
    {
	info = new OPeNDAPSilentInfo() ;
    }
    else
    {
	info = new DODSInfo( dhi.transmit_protocol == "HTTP" ) ;
    }
    _response = info ;
    
    string def_name = dhi.data[DEF_NAME] ;
    string store_name = dhi.data[STORE_NAME] ;
    if( store_name == "" )
	store_name = PERSISTENCE_VOLATILE ;
    DefinitionStorage *store =
	DefinitionStorageList::TheList()->find_persistence( store_name ) ;
    if( store )
    {
	string def_type = "added" ;
	bool deleted = store->del_definition( def_name ) ;
	if( deleted == true )
	{
	    def_type = "replaced" ;
	}

	DODSDefine *dd = new DODSDefine ;
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
	string ret = (string)"Successfully " + def_type + " definition \""
		     + def_name + "\ to \"" + store_name + "\" store\n" ;
	info->add_data( ret ) ;
    }
    else
    {
	string ret = (string)"Unable to add definition \"" + def_name
		     + "\" to \"" + store_name
		     + "\" store. Store does not exist\n" ;
	info->add_data( ret ) ;
    }
}

/** @brief transmit the response object built by the execute command
 * using the specified transmitter object
 *
 * A DODSInfo response object was built in the exeucte command to inform
 * the user whether the definition was successfully added or replaced. The
 * method send_text is called on the DODSTransmitter object.
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see DODSInfo
 * @see DODSTransmitter
 * @see _DODSDataHandlerInterface
 */
void
DefineResponseHandler::transmit( DODSTransmitter *transmitter,
                                  DODSDataHandlerInterface &dhi )
{
    if( _response )
    {
        // If this dynamic_cast is to a reference an not a pointer, then if
        // _response is not a DODSInfo the cast will throw bad_cast. 
        // Casting to a pointer will make ti null on error. jhrg 11/10/2005
	DODSInfo *ti = dynamic_cast<DODSInfo *>(_response) ;
	transmitter->send_text( *ti, dhi ) ;
    }
}

DODSResponseHandler *
DefineResponseHandler::DefineResponseBuilder( string handler_name )
{
    return new DefineResponseHandler( handler_name ) ;
}

