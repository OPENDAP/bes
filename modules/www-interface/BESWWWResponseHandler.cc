// BESWWWResponseHandler.cc

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

#include <DDS.h>

#include "BESWWWResponseHandler.h"
#include "BESRequestHandlerList.h"
#include "BESDapNames.h"
#include "BESWWWNames.h"
#include "BESWWW.h"
#include "BESDDSResponse.h"
#include "BESWWWTransmit.h"

using namespace libdap;

BESWWWResponseHandler::BESWWWResponseHandler( const string &name )
:  BESResponseHandler(name)
{
}

BESWWWResponseHandler::~BESWWWResponseHandler()
{
}

/** @brief executes the command 'get html_form for &lt;def_name&gt;;' by
 * executing the request for each container in the specified defnition.
 *
 * For each container in the specified defnition go to the request
 * handler for that container and have it add to the OPeNDAP DAS response
 * object. The DAS response object is built within this method and passed
 * to the request handler list.
 *
 * Once the DAS has been filled in do the same for a DDS. We only need the
 * description of the data and not the data itself.
 *
 * @param dhi structure that holds request and response information
 * @see _BESDataHandlerInterface
 * @see DAS
 * @see BESRequestHandlerList
 */
void
 BESWWWResponseHandler::execute(BESDataHandlerInterface & dhi)
{
    dhi.action_name = WWW_RESPONSE_STR;

    // Create the DDS.
    // NOTE: It is the responsbility of the specific request handler to set
    // the BaseTypeFactory. It is set to NULL here
    DDS *dds = new DDS(NULL, "virtual");
    BESDDSResponse *bdds = new BESDDSResponse(dds);
    _response = bdds;
    _response_name = DDS_RESPONSE;
    dhi.action = DDS_RESPONSE;
    BESRequestHandlerList::TheList()->execute_each(dhi);
#if 0
    // Fill the DAS
    DAS *das = new DAS;
    BESDASResponse *bdas = new BESDASResponse(das);
    _response = bdas;
    _response_name = DAS_RESPONSE;
    dhi.action = DAS_RESPONSE;
    BESRequestHandlerList::TheList()->execute_each(dhi);
#endif
    BESWWW *www = new BESWWW(/*bdas,*/ bdds);
    _response = www;
    dhi.action = WWW_RESPONSE;
}

/** @brief transmit the response object built by the execute command
 * using the specified transmitter object
 *
 * If a response object was built then transmit it using the send_das method
 * on the transmitter object.
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see DAS
 * @see BESTransmitter
 * @see _BESDataHandlerInterface
 */
void BESWWWResponseHandler::transmit(BESTransmitter * transmitter,
                                     BESDataHandlerInterface & dhi)
{
    if (_response) {
        transmitter->send_response(WWW_TRANSMITTER, _response, dhi);
    }
}

BESResponseHandler *BESWWWResponseHandler::
WWWResponseBuilder( const string &handler_name )
{
    return new BESWWWResponseHandler( handler_name ) ;
}
