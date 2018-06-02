// BESDap4ResponseHandler.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2013 OPeNDAP
// Author: James Gallagher <jgallagher@opendap.org>
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

#include "config.h"

#include <DMR.h>

#include "BESDap4ResponseHandler.h"
#include "BESDMRResponse.h"
#include "BESRequestHandlerList.h"
#include "BESDapNames.h"
#include "BESDapTransmit.h"
#include "BESContextManager.h"
#include "BESDebug.h"

#include "GlobalMetadataStore.h"

using namespace bes;

BESDap4ResponseHandler::BESDap4ResponseHandler(const string &name) : BESResponseHandler(name)
{
}

BESDap4ResponseHandler::~BESDap4ResponseHandler()
{
}

/** @brief executes the command 'get dap for def_name;'
 *
 * @param dhi structure that holds request and response information
 * @see BESDataHandlerInterface
 * @see BESDMRResponse
 * @see BESRequestHandlerList
 */
void BESDap4ResponseHandler::execute(BESDataHandlerInterface &dhi)
{
	dhi.action_name = DAP4DATA_RESPONSE_STR;

    bool found;
    // This throws on error; hence before the 'new DMR()'
    int response_size_limit = BESContextManager::TheManager()->get_context_int("max_response_size", found);

    GlobalMetadataStore *mds = GlobalMetadataStore::get_instance(); // mds may be NULL

    GlobalMetadataStore::MDSReadLock lock;
    dhi.first_container();
    if (mds) lock = mds->is_dmrpp_available(dhi.container->get_relative_name());

    // If we were able to lock the DMR++ it must exist; use it.
    if (mds && lock()) {
        BESDEBUG("dmrpp", "In BESDap4ResponseHandler::execute(): Found a DMR++ response for '"
            << dhi.container->get_relative_name() << "'" << endl);

        // Redirect the request to the DMR++ handler
        // FIXME How do we get this value in a repeatable way? From bes.conf, of course jhrg 5/31/18
        dhi.container->set_container_type("dmrpp");

        // Add information to the container so the dmrpp handler works
        // This tells DMR++ handler to look for this in the MDS
        dhi.container->set_attributes(MDS_HAS_DMRPP);
    }

	DMR *dmr = new DMR();

	if (found)
	    dmr->set_response_limit(response_size_limit);

    string xml_base = BESContextManager::TheManager()->get_context("xml:base", found);
	if (found && !xml_base.empty())
		dmr->set_request_xml_base(xml_base);

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
void BESDap4ResponseHandler::transmit(BESTransmitter *transmitter, BESDataHandlerInterface &dhi)
{
	if (d_response_object) {
		transmitter->send_response(DAP4DATA_SERVICE, d_response_object, dhi);
	}
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESDap4ResponseHandler::dump(ostream &strm) const
{
	strm << BESIndent::LMarg << "BESDap4ResponseHandler::dump - (" << (void *) this << ")" << endl;
	BESIndent::Indent();
	BESResponseHandler::dump(strm);
	BESIndent::UnIndent();
}

BESResponseHandler *
BESDap4ResponseHandler::Dap4ResponseBuilder(const string &name)
{
	return new BESDap4ResponseHandler(name);
}

