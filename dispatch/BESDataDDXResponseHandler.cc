// BESDataDDXResponseHandler.cc

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

#include "BESDataDDXResponseHandler.h"
#include "BESDataDDSResponse.h"
#include "BESRequestHandlerList.h"
#include "BESDapNames.h"
#include "BESDapTransmit.h"

#include "BESDebug.h"

BESDataDDXResponseHandler::BESDataDDXResponseHandler( const string &name )
    : BESResponseHandler( name )
{
}

BESDataDDXResponseHandler::~BESDataDDXResponseHandler( )
{
}

/** @brief executes the command 'get ddx for def_name;'
 *
 * For each container in the specified definition go to the request
 * handler for that container and have it first add to the OPeNDAP DDS response
 * object. Once the DDS object has been filled in, repeat the process but
 * this time for the OPeNDAP DAS response object. Then add the attributes from
 * the DAS object to the DDS object.
 *
 * @param dhi structure that holds request and response information
 * @see BESDataHandlerInterface
 * @see BESDDSResponse
 * @see BESDASResponse
 * @see BESRequestHandlerList
 */
void
BESDataDDXResponseHandler::execute( BESDataHandlerInterface &dhi )
{
    BESDEBUG( "dap", "Entering BESDataDDXResponseHandler::execute" << endl )

    dhi.action_name = DATADDX_RESPONSE_STR ;
    // Create the DDS.
    // NOTE: It is the responsibility of the specific request handler to set
    // the BaseTypeFactory. It is set to NULL here
    DataDDS *dds = new DataDDS( NULL, "virtual" ) ;
    BESDataDDSResponse *bdds = new BESDataDDSResponse( dds ) ;
    _response = bdds ;

    // we're actually going to get the data response, it just gets
    // displayed as a DataDDX
    _response_name = DATA_RESPONSE ;
    dhi.action = DATA_RESPONSE ;
    BESRequestHandlerList::TheList()->execute_each( dhi ) ;

    // we've got what we want, now set the action back to data ddx
    dhi.action = DATADDX_RESPONSE ;
    _response = bdds ;

    BESDEBUG( "dap", "Leaving BESDataDDXResponseHandler::execute" << endl )
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
 * @see BESDataHandlerInterface
 */
void
BESDataDDXResponseHandler::transmit( BESTransmitter * transmitter,
                                     BESDataHandlerInterface & dhi )
{
    if( _response )
    {
        transmitter->send_response( DATADDX_SERVICE, _response, dhi ) ;
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESDataDDXResponseHandler::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESDataDDXResponseHandler::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESResponseHandler::dump( strm ) ;
    BESIndent::UnIndent() ;
}

BESResponseHandler *
BESDataDDXResponseHandler::DataDDXResponseBuilder( const string &name )
{
    return new BESDataDDXResponseHandler( name ) ;
}

