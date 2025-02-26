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

#include <memory>

#include <libdap/DMR.h>

#include "BESDap4ResponseHandler.h"
#include "BESDMRResponse.h"
#include "BESRequestHandlerList.h"
#include "BESDapNames.h"
#include "BESDapTransmit.h"
#include "BESContextManager.h"
#include "TheBESKeys.h"
#include "BESDebug.h"

#include "GlobalMetadataStore.h"

using namespace std;
using namespace bes;

BESDap4ResponseHandler::BESDap4ResponseHandler(const string &name)
    : BESResponseHandler(name), d_use_dmrpp(false), d_dmrpp_name(DMRPP_DEFAULT_NAME)
{
    d_use_dmrpp = TheBESKeys::TheKeys()->read_bool_key(USE_DMRPP_KEY, false);   // defined in BESDapNames.h
    d_dmrpp_name = TheBESKeys::TheKeys()->read_string_key(DMRPP_NAME_KEY, DMRPP_DEFAULT_NAME);
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

    if (d_use_dmrpp) {
        GlobalMetadataStore *mds = GlobalMetadataStore::get_instance(); // mds may be NULL

        GlobalMetadataStore::MDSReadLock lock;
        dhi.first_container();
        if (mds) lock = mds->is_dmrpp_available(*(dhi.container));

        // If we were able to lock the DMR++ it must exist; use it.
        if (mds && lock()) {
            BESDEBUG("dmrpp",
                "In BESDap4ResponseHandler::execute(): Found a DMR++ response for '" << dhi.container->get_relative_name() << "'" << endl);

            // Redirect the request to the DMR++ handler
            dhi.container->set_container_type(d_dmrpp_name);

            // Add information to the container so the dmrpp handler works
            // This tells DMR++ handler to look for this in the MDS
            dhi.container->set_attributes(MDS_HAS_DMRPP);
        }
    }

    unique_ptr<DMR> dmr(new DMR());

    bool found;
    int response_size_limit = BESContextManager::TheManager()->get_context_int("max_response_size", found);

	if (found)
	    dmr->set_response_limit_kb(response_size_limit);

	d_response_object = new BESDMRResponse(dmr.release());

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

