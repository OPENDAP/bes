// BESDelDefResponseHandler.cc

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

#include "BESDelDefResponseHandler.h"
#include "BESSilentInfo.h"
#include "BESDefinitionStorageList.h"
#include "BESDefinitionStorage.h"
#include "BESDefine.h"
#include "BESContainerStorageList.h"
#include "BESContainerStorage.h"
#include "BESContainer.h"
#include "BESDataNames.h"
#include "BESSyntaxUserError.h"
#include "BESResponseNames.h"

BESDelDefResponseHandler::BESDelDefResponseHandler( const string &name )
    : BESResponseHandler( name )
{
}

BESDelDefResponseHandler::~BESDelDefResponseHandler( )
{
}

/** @brief executes the command to delete a definition
 *
 * Removes a definition from a specified definition storage found in 
 * BESDefinitionStorageList::TheList(). If no definition store is specified,
 * the default is volatile.
 *
 * The response built is a silent informational object. The only response
 * that a client would receive would be if there were an exception thrown
 * attempting to delete the definition.
 *
 * @param dhi structure that holds request and response information
 * @throws BESSyntaxUserError if unable to delete the specified definition
 * from the specified definition store.
 * object
 * @see BESDataHandlerInterface
 * @see BESSilentInfo
 * @see BESDefine
 * @see BESDefinitionStorage
 * @see BESDefinitionStorageList
 */
void
BESDelDefResponseHandler::execute( BESDataHandlerInterface &dhi )
{
    dhi.action_name = DELETE_DEFINITION_STR ;
    BESInfo *info = new BESSilentInfo() ;
    _response = info ;

    string def_name = dhi.data[DEF_NAME] ;
    string store_name = dhi.data[STORE_NAME] ;
    if( def_name != "" )
    {
	if( store_name == "" )
	    store_name = PERSISTENCE_VOLATILE ;
	BESDefinitionStorage *store =
	    BESDefinitionStorageList::TheList()->find_persistence( store_name ) ;
	if( store )
	{
	    bool deleted = store->del_definition( dhi.data[DEF_NAME] ) ;
	    if( !deleted )
	    {
		string line = (string)"Definition \""
			      + dhi.data[DEF_NAME]
			      + "\" does not exist. Unable to delete." ;
		throw BESSyntaxUserError( line, __FILE__, __LINE__ ) ;
	    }
	}
	else
	{
	    string line = (string)"Definition store \""
			  + store_name
			  + "\" does not exist.  Unable to delete." ;
	    throw BESSyntaxUserError( line, __FILE__, __LINE__ ) ;
	}
    }
    else
    {
	string line = (string)"No definition specified. "
		      + "Unable to complete request." ;
	throw BESSyntaxUserError( line, __FILE__, __LINE__ ) ;
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
 * @see BESDataHandlerInterface
 */
void
BESDelDefResponseHandler::transmit( BESTransmitter *transmitter,
                               BESDataHandlerInterface &dhi )
{
    if( _response )
    {
	BESInfo *info = dynamic_cast<BESInfo *>(_response) ;
	if( !info )
	    throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;
	info->transmit( transmitter, dhi ) ;
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESDelDefResponseHandler::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESDelDefResponseHandler::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESResponseHandler::dump( strm ) ;
    BESIndent::UnIndent() ;
}

BESResponseHandler *
BESDelDefResponseHandler::DelDefResponseBuilder( const string &name )
{
    return new BESDelDefResponseHandler( name ) ;
}

