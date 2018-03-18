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
#include "GlobalMetadataStore.h"
#include "BESDebug.h"

using namespace bes;
using namespace std;

BESDMRResponseHandler::BESDMRResponseHandler(const string &name) :
        BESResponseHandler(name)
{
}

BESDMRResponseHandler::~BESDMRResponseHandler()
{
}

/**
 * @brief executes the command `<get type-"dmr" definition="..">` by executing
 * the request for each container in the specified definition.
 *
 * For each container in the specified definition go to the request
 * handler for that container and have it add to the OPeNDAP DMR response
 * object. The DMR response object is built within this method and passed
 * to the request handler list.
 *
 * @note This ResponseHandler does not work for multiple containers when using
 * the Global Metadata Store.
 *
 * @param dhi structure that holds request and response information
 *
 * @see BESDataHandlerInterface
 * @see BESDMRResponse
 * @see BESRequestHandlerList
 */
void BESDMRResponseHandler::execute(BESDataHandlerInterface &dhi)
{
    dhi.action_name = DMR_RESPONSE_STR;

    bool xml_base_found = false;
    string xml_base = BESContextManager::TheManager()->get_context("xml:base", xml_base_found);

    // Look in the MDS for dhi.container.get_real_name().
    // if found, use that response, else build it.
    // If the MDS is disabled, don't use it.
    GlobalMetadataStore *mds = GlobalMetadataStore::get_instance();

    GlobalMetadataStore::MDSReadLock lock;

    dhi.first_container();
    if (mds) lock = mds->is_dmr_available(dhi.container->get_real_name());

    if (mds && lock() && dhi.container->get_dap4_constraint().empty()) {
        // FIXME Does not work for constrained DMR requests
        BESDEBUG("dmr", __func__ << " Locked: " << dhi.container->get_real_name() << endl);
        // send the response
        mds->get_dmr_response(dhi.container->get_real_name(), dhi.get_output_stream());
        // suppress transmitting a ResponseObject in transmit()
        d_response_object = 0;
    }
    else {
        DMR *dmr = new DMR();

        if (xml_base_found && !xml_base.empty()) dmr->set_request_xml_base(xml_base);

        d_response_object = new BESDMRResponse(dmr);

        BESRequestHandlerList::TheList()->execute_each(dhi);

        if (mds) {
            dhi.first_container();  // must reset container; execute_each() iterates over all of them
            BESDEBUG("dmr", __func__ << " Storing: " << dhi.container->get_real_name() << endl);
            mds->add_responses(static_cast<BESDMRResponse*>(d_response_object)->get_dmr(),
                dhi.container->get_real_name());
        }
    }
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

