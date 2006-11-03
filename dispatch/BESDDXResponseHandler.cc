// BESDDXResponseHandler.cc

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

#include "BESDDXResponseHandler.h"
#include "BESDASResponse.h"
#include "BESDDSResponse.h"
#include "BESResponseNames.h"
#include "BESRequestHandlerList.h"
#include "BESDapTransmit.h"

BESDDXResponseHandler::BESDDXResponseHandler( string name )
    : BESResponseHandler( name )
{
}

BESDDXResponseHandler::~BESDDXResponseHandler( )
{
}

/** @brief executes the command 'get ddx for &lt;def_name&gt;;'
 *
 * For each container in the specified defnition go to the request
 * handler for that container and have it first add to the OPeNDAP DDS response
 * object. Oncew the DDS object has been filled in, repeat the process but
 * this time for the OPeNDAP DAS response object. Then add the attributes from
 * the DAS object to the DDS object.
 *
 * @param dhi structure that holds request and response information
 * @throws BESHandlerException if there is a problem building the
 * response object
 * @throws BESResponseException upon fatal error building the response
 * object
 * @see _BESDataHandlerInterface
 * @see BESDDSResponse
 * @see BESDASResponse
 * @see BESRequestHandlerList
 */
void
BESDDXResponseHandler::execute( BESDataHandlerInterface &dhi )
{
    dhi.action_name = DDX_RESPONSE_STR ;
    // Create the DDS.
    // NOTE: It is the responsbility of the specific request handler to set
    // the BaseTypeFactory. It is set to NULL here
    DDS *dds = new DDS( NULL, "virtual" ) ;
    BESDDSResponse *bdds = new BESDDSResponse( dds ) ;
    _response = bdds ;
    _response_name = DDS_RESPONSE ;
    dhi.action = DDS_RESPONSE ;
    BESRequestHandlerList::TheList()->execute_each( dhi ) ;

    // Fill the DAS
    DAS *das = new DAS ;
    BESDASResponse *bdas = new BESDASResponse( das ) ;
    _response = bdas ;
    _response_name = DAS_RESPONSE ;
    dhi.action = DAS_RESPONSE ;
    BESRequestHandlerList::TheList()->execute_each( dhi ) ;

    // Transfer the DAS to the DDS
    dds->transfer_attributes( das ) ;

    dhi.action = DDX_RESPONSE ;
    _response = bdds ;
}

/** @brief transmit the response object built by the execute command
 *
 * If a response object was built then transmit it using the send_ddx method
 * on the specified transmitter object.
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see DDS
 * @see DAS
 * @see BESTransmitter
 * @see _BESDataHandlerInterface
 */
void
BESDDXResponseHandler::transmit( BESTransmitter *transmitter,
                              BESDataHandlerInterface &dhi )
{
    if( _response )
    {
	transmitter->send_response( DDX_TRANSMITTER, _response, dhi ) ;
    }
}

BESResponseHandler *
BESDDXResponseHandler::DDXResponseBuilder( string handler_name )
{
    return new BESDDXResponseHandler( handler_name ) ;
}

