// BESDDSResponseHandler.cc

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

#include <DDS.h>

#include "BESDDSResponseHandler.h"
#include "BESDDSResponse.h"
#include "BESRequestHandlerList.h"
#include "BESDapNames.h"
#include "BESDataNames.h"
#include "GlobalMetadataStore.h"
#include "BESDebug.h"

using namespace libdap;
using namespace bes;

BESDDSResponseHandler::BESDDSResponseHandler(const string &name) :
		BESResponseHandler(name)
{
}

BESDDSResponseHandler::~BESDDSResponseHandler()
{
}

/**
 * @brief executes the command `<get type="dds" definition=...>`
 *
 * For each container in the specified definition, go to the request
 * handler for that container and have it add to the OPeNDAP DDS response
 * object. The DDS response object is created within this method and passed
 * to the request handler list. The ResponseHandler now supports the Metadata
 * store, so cached/stored DDS responses will be used if found there and if
 * the MDS is configured in the bes.conf file.
 *
 * @note This ResponseHandler does not work for multiple containers when using
 * the Global Metadata Store.
 *
 * @param dhi structure that holds request and response information
 *
 * @see BESDataHandlerInterface
 * @see BESDDSResponse
 * @see BESRequestHandlerList
 */
void BESDDSResponseHandler::execute(BESDataHandlerInterface &dhi)
{
    dhi.action_name = DDS_RESPONSE_STR;

    GlobalMetadataStore *mds = GlobalMetadataStore::get_instance();

    GlobalMetadataStore::MDSReadLock lock;

    dhi.first_container();
    if (mds) lock = mds->is_dds_available(dhi.container->get_real_name());

    if (mds && lock() && dhi.container->get_constraint().empty()) {
        // FIXME Does not work for constrained DDS requests
        // send the stored response
        mds->get_dds_response(dhi.container->get_real_name(), dhi.get_output_stream());
        // suppress transmitting a ResponseObject in transmit()
        d_response_object = 0;
    }
    else {
        DDS *dds = new DDS(NULL, "virtual");

#if 0
        // Keywords were a hack to the protocol and have been dropped. We can get rid of
        // this keyword code. jhrg 11/6/13
        dhi.container->set_constraint(dds->get_keywords().parse_keywords(dhi.container->get_constraint()));

        if (dds->get_keywords().has_keyword("dap")) {
            dds->set_dap_version(dds->get_keywords().get_keyword_value("dap"));
        }
        else if (!bdds->get_dap_client_protocol().empty()) {
            dds->set_dap_version(bdds->get_dap_client_protocol());
        }
#endif

        d_response_object = new BESDDSResponse(dds);

        BESRequestHandlerList::TheList()->execute_each(dhi);

        if (mds) {
            dhi.first_container();  // must reset container; execute_each() iterates over all of them
            mds->add_responses(static_cast<BESDDSResponse*>(d_response_object)->get_dds(),
                dhi.container->get_real_name());
        }
    }
}

/** @brief transmit the response object built by the execute command
 * using the specified transmitter object
 *
 * If a response object was built then transmit it using the send_dds method
 * on the specified transmitter object.
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see BESDDSResponse
 * @see BESTransmitter
 * @see BESDataHandlerInterface
 */
void BESDDSResponseHandler::transmit(BESTransmitter *transmitter, BESDataHandlerInterface &dhi)
{
	if (d_response_object) {
		transmitter->send_response(DDS_SERVICE, d_response_object, dhi);
	}
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESDDSResponseHandler::dump(ostream &strm) const
{
	strm << BESIndent::LMarg << "BESDDSResponseHandler::dump - (" << (void *) this << ")" << endl;
	BESIndent::Indent();
	BESResponseHandler::dump(strm);
	BESIndent::UnIndent();
}

BESResponseHandler *
BESDDSResponseHandler::DDSResponseBuilder(const string &name)
{
	return new BESDDSResponseHandler(name);
}

