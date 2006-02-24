// DDXResponseHandler.cc

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

#include "DDXResponseHandler.h"
#include "DAS.h"
#include "DDS.h"
#include "cgi_util.h"
#include "DODSResponseNames.h"
#include "DODSRequestHandlerList.h"

DDXResponseHandler::DDXResponseHandler( string name )
    : DODSResponseHandler( name )
{
}

DDXResponseHandler::~DDXResponseHandler( )
{
}

/** @brief executes the command 'get ddx for &lt;def_name&gt;;' by executing
 * the request for each container in the specified defnition def_name.
 *
 * For each container in the specified defnition def_name go to the request
 * handler for that container and have it first add to the OPeNDAP DDS response
 * object. Oncew the DDS object has been filled in, repeat the process but
 * this time for the OPeNDAP DAS response object. Then add the attributes from
 * the DAS object to the DDS object.
 *
 * @param dhi structure that holds request and response information
 * @throws DODSResponseException if there is a problem building the
 * response object
 * @see _DODSDataHandlerInterface
 * @see DDS
 * @see DAS
 * @see DODSRequestHandlerList
 */
void
DDXResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    // Create the DDS.
    // NOTE: It is the responsbility of the specific request handler to set
    // the BaseTypeFactory. It is set to NULL here
    DDS *dds = new DDS( NULL, "virtual" ) ;
    _response = dds ;
    _response_name = DDS_RESPONSE ;
    dhi.action = DDS_RESPONSE ;
    DODSRequestHandlerList::TheList()->execute_each( dhi ) ;

    // Fill the DAS
    DAS *das = new DAS ;
    _response = das ;
    _response_name = DAS_RESPONSE ;
    dhi.action = DAS_RESPONSE ;
    DODSRequestHandlerList::TheList()->execute_each( dhi ) ;

    // Transfer the DAS to the DDS
    dds->transfer_attributes( das ) ;

    dhi.action = DDX_RESPONSE ;
    _response = dds ;
}

/** @brief transmit the response object built by the execute command
 * using the specified transmitter object
 *
 * If a response object was built then transmit it using the send_ddx method.
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see DDS
 * @see DAS
 * @see DODSTransmitter
 * @see _DODSDataHandlerInterface
 */
void
DDXResponseHandler::transmit( DODSTransmitter *transmitter,
                              DODSDataHandlerInterface &dhi )
{
    if( _response )
    {
	DDS *dds = dynamic_cast<DDS *>(_response) ;
	transmitter->send_ddx( *dds, dhi ) ;
    }
}

DODSResponseHandler *
DDXResponseHandler::DDXResponseBuilder( string handler_name )
{
    return new DDXResponseHandler( handler_name ) ;
}

// $Log: DDXResponseHandler.cc,v $
// Revision 1.3  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
