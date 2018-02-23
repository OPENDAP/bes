// BESDMRResponseHandler.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2013 OPeNDAP, Inc.
// Author: Patrick West <pwest@rpi.edu>
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
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include <DMR.h>

#include "BESDMRResponseHandler.h"
#include "BESDMRResponse.h"
#include "BESRequestHandlerList.h"
#include "BESDapNames.h"
#include "BESDapTransmit.h"
#include "BESContextManager.h"

BESDMRResponseHandler::BESDMRResponseHandler(const string &name) :
        BESResponseHandler(name)
{
}

BESDMRResponseHandler::~BESDMRResponseHandler()
{
}

/** @brief executes the command 'get dmr for def_name;' by executing
 * the request for each container in the specified definition.
 *
 * For each container in the specified definition go to the request
 * handler for that container and have it add to the OPeNDAP DMR response
 * object. The DMR response object is built within this method and passed
 * to the request handler list.
 *
 * @param dhi structure that holds request and response information
 * @see BESDataHandlerInterface
 * @see BESDMRResponse
 * @see BESRequestHandlerList
 */
void BESDMRResponseHandler::execute(BESDataHandlerInterface &dhi)
{
    dhi.action_name = DMR_RESPONSE_STR;
    DMR *dmr = new DMR();

    // Here we might set the dap and dmr version if they should be different from
    // 4.0 and 1.0. jhrg 11/6/13

    bool found = false;
    string xml_base = BESContextManager::TheManager()->get_context("xml:base", found);
    if (found && !xml_base.empty()) {
        dmr->set_request_xml_base(xml_base);
    }

    d_response_object = new BESDMRResponse(dmr);
    BESRequestHandlerList::TheList()->execute_each(dhi);
}

/** @brief transmit the response object built by the execute command
 * using the specified transmitter object
 *
 * If a response object was built then transmit it using the send_dmr method
 * on the transmitter object.
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see BESDMRResponse
 * @see BESTransmitter
 * @see BESDataHandlerInterface
 */
void BESDMRResponseHandler::transmit(BESTransmitter *transmitter, BESDataHandlerInterface &dhi)
{
    if (d_response_object) {
        transmitter->send_response(DMR_SERVICE, d_response_object, dhi);
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESDMRResponseHandler::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESDMRResponseHandler::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESResponseHandler::dump(strm);
    BESIndent::UnIndent();
}

BESResponseHandler *
BESDMRResponseHandler::DMRResponseBuilder(const string &name)
{
    return new BESDMRResponseHandler(name);
}

