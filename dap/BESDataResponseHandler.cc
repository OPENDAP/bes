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

#include "config.h"

#include <DDS.h>
#include <DataDDS.h>

#include "BESDataResponseHandler.h"
#include "BESDataDDSResponse.h"
#include "BESRequestHandlerList.h"
#include "BESDapNames.h"
#include "BESDataNames.h"
#include "BESContextManager.h"
#include "TheBESKeys.h"
#include "BESInternalError.h"
#include "BESDebug.h"
#include "BESTransmitter.h"

#include "GlobalMetadataStore.h"

using namespace bes;
using namespace libdap;
using namespace std;

BESDataResponseHandler::BESDataResponseHandler(const string &name) :
    BESResponseHandler(name), d_use_dmrpp(false), d_dmrpp_name(DMRPP_DEFAULT_NAME)
{
    d_use_dmrpp = TheBESKeys::TheKeys()->read_bool_key(USE_DMRPP_KEY, false);   // defined in BESDapNames.h
    d_dmrpp_name = TheBESKeys::TheKeys()->read_string_key(DMRPP_NAME_KEY, DMRPP_DEFAULT_NAME);
}

BESDataResponseHandler::~BESDataResponseHandler()
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
void BESDataResponseHandler::execute(BESDataHandlerInterface &dhi)
{
    dhi.action_name = DATA_RESPONSE_STR;

    if (d_use_dmrpp) {
        GlobalMetadataStore *mds = GlobalMetadataStore::get_instance(); // mds may be NULL

        GlobalMetadataStore::MDSReadLock lock;
        dhi.first_container();
        if (mds) lock = mds->is_dmrpp_available(*(dhi.container));

        // If we were able to lock the DMR++ it must exist; use it.
        if (mds && lock()) {
            BESDEBUG("dmrpp",
                "In BESDataResponseHandler::execute(): Found a DMR++ response for '" << dhi.container->get_relative_name() << "'" << endl);

            // Redirect the request to the DMR++ handler
            dhi.container->set_container_type(d_dmrpp_name);

            // Add information to the container so the dmrpp handler works
            // This tells DMR++ handler to look for this in the MDS
            dhi.container->set_attributes(MDS_HAS_DMRPP);
        }
    }

#if 0
    GlobalMetadataStore *mds = GlobalMetadataStore::get_instance(); // mds may be NULL

    GlobalMetadataStore::MDSReadLock lock;
    dhi.first_container();
    if (mds) lock = mds->is_dmrpp_available(dhi.container->get_relative_name());

    // If we were able to lock the DMR++ it must exist; use it.
    if (mds && lock()) {
        BESDEBUG("dmrpp", "In BESDataResponseHandler::execute(): Found a DMR++ response for '"
            << dhi.container->get_relative_name() << "'" << endl);

        // Redirect the request to the DMR++ handler
        // FIXME How do we get this value in a repeatable way? From bes.conf, of course jhrg 5/31/18
        dhi.container->set_container_type("dmrpp");

        // Add information to the container so the dmrpp handler works
        // This tells DMR++ handler to look for this in the MDS
        dhi.container->set_attributes(MDS_HAS_DMRPP);
    }
#endif


    bool rsl_found;
    int response_size_limit = BESContextManager::TheManager()->get_context_int("max_response_size", rsl_found);

    // NOTE: It is the responsibility of the specific request handler to set
    // the BaseTypeFactory. It is set to NULL here
    DDS *dds = new DDS(NULL, "virtual");
    if (rsl_found)
        dds->set_response_limit(response_size_limit); // The default for this is zero

    BESDataDDSResponse *bdds = new BESDataDDSResponse(dds);

    dhi.first_container();
    // Set the DAP protocol version requested by the client. 2/25/11 jhrg
    if (!bdds->get_dap_client_protocol().empty()) {
        dds->set_dap_version(bdds->get_dap_client_protocol());
    }

    d_response_object = bdds;

    // This calls RequestHandlerList::execute_current()
    BESRequestHandlerList::TheList()->execute_each(dhi);
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
void BESDataResponseHandler::transmit(BESTransmitter *transmitter, BESDataHandlerInterface &dhi)
{
    if (d_response_object) {
        transmitter->send_response( DATA_SERVICE, d_response_object, dhi);
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESDataResponseHandler::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESDataResponseHandler::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESResponseHandler::dump(strm);
    BESIndent::UnIndent();
}

BESResponseHandler *
BESDataResponseHandler::DataResponseBuilder(const string &name)
{
    return new BESDataResponseHandler(name);
}

