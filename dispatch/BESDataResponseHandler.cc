// BESDataResponseHandler.cc

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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include "BESDataResponseHandler.h"
#include "BESDataDDSResponse.h"
#include "BESRequestHandlerList.h"
#include "BESDapNames.h"
#include "BESDapTransmit.h"

BESDataResponseHandler::BESDataResponseHandler( const string &name )
    : BESResponseHandler( name )
{
}

BESDataResponseHandler::~BESDataResponseHandler( )
{
}

/** @brief executes the command 'get data for &lt;def_name&gt;' by
 * executing the request for each container in the specified definition
 *
 * For each container in the specified defnition go to the request
 * handler for that container and have it add to the OPeNDAP DataDDS data
 * response object. The data response object is created within this method
 * and passed to the request handler list.
 *
 * @param dhi structure that holds request and response information
 * @see BESDataHandlerInterface
 * @see BESDataDDSResponse
 * @see BESRequestHandlerList
 * @see BESDefine
 */
void
BESDataResponseHandler::execute( BESDataHandlerInterface &dhi )
{
    dhi.action_name = DATA_RESPONSE_STR ;
    // NOTE: It is the responsibility of the specific request handler to set
    // the BaseTypeFactory. It is set to NULL here
    DataDDS *dds = new DataDDS( NULL, "virtual" ) ;
    _response = new BESDataDDSResponse( dds ) ;
    BESRequestHandlerList::TheList()->execute_each( dhi ) ;
}

/** @brief transmit the response object built by the execute command
 *
 * If a response object was built then transmit it using the send_data
 * method on the specified transmitter object.
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see BESDataDDSResponse
 * @see BESTransmitter
 * @see BESDataHandlerInterface
 */
void
BESDataResponseHandler::transmit( BESTransmitter *transmitter,
                                  BESDataHandlerInterface &dhi )
{
    if( _response )
    {
	transmitter->send_response( DATA_SERVICE, _response, dhi ) ;
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESDataResponseHandler::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESDataResponseHandler::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESResponseHandler::dump( strm ) ;
    BESIndent::UnIndent() ;
}

BESResponseHandler *
BESDataResponseHandler::DataResponseBuilder( const string &name )
{
    return new BESDataResponseHandler( name ) ;
}

