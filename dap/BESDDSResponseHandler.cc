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

#include "config.h"

#include <DDS.h>

#include "BESDDSResponseHandler.h"
#include "BESDDSResponse.h"
#include "BESRequestHandlerList.h"
#include "BESDapNames.h"
#include "BESDataNames.h"
#include "BESTransmitter.h"

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
 * Is there a function call in this CE? This function tests for a left paren
 * to indicate that a function is present. The CE will still be encoded at
 * this point, so test for the escape characters (%28) too.
 *
 * @param ce The Constraint expression to test
 * @return True if there is a function call, false otherwise
 */
static bool
function_in_ce(const string &ce)
{
    // 0x28 is '('
    return ce.find("(") != string::npos || ce.find("%28") != string::npos;   // hack
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
    if (mds) lock = mds->is_dds_available(dhi.container->get_relative_name());

    if (mds && lock() && dhi.container->get_constraint().empty()) {
        // Unconstrained DDS requests; send the stored response
        mds->write_dds_response(dhi.container->get_relative_name(), dhi.get_output_stream());
        // suppress transmitting a ResponseObject in transmit()
        d_response_object = 0;
    }
    else {
        DDS *dds = 0;

        // Constrained DDS request (but not a CE with a function).
        if (mds && lock() && !function_in_ce(dhi.container->get_constraint())) {
            // If mds and lock(), the DDS is in the cache, get the _object_
            dds = mds->get_dds_object(dhi.container->get_relative_name());
            BESDDSResponse *bdds = new BESDDSResponse(dds);
            bdds->set_constraint(dhi);
            bdds->clear_container();
            d_response_object = bdds;
        }
        else {
            dds = new DDS(NULL, "virtual");

#if ANNOTATION_SYSTEM
            // Support for the experimental Dataset Annotation system. jhrg 12/19/18
            if (!d_annotation_service_url.empty()) {
                auto_ptr<AttrTable> dods_extra(new AttrTable);
                dods_extra->append_attr(DODS_EXTRA_ANNOTATION_ATTR, "String", d_annotation_service_url);

                dds->get_attr_table().append_container(dods_extra.release(), DODS_EXTRA_ATTR_TABLE);
            }
#endif

            d_response_object = new BESDDSResponse(dds);

            BESRequestHandlerList::TheList()->execute_each(dhi);

            dhi.first_container();  // must reset container; execute_each() iterates over all of them

            // Cache the DDS if the MDS is not null but the DDS response was not in the cache
            if (mds && !lock() && !function_in_ce(dhi.container->get_constraint())) {
                // moved dhi.first_container();  // must reset container; execute_each() iterates over all of them
                mds->add_responses(static_cast<BESDDSResponse*>(d_response_object)->get_dds(),
                    dhi.container->get_relative_name());
            }
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

