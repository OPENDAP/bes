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

#include "config.h"

#include <sstream>
#include <memory>

#include <DMR.h>
#include <D4Group.h>
#include <D4Attributes.h>
#include <D4BaseTypeFactory.h>

#include "BESDMRResponseHandler.h"
#include "BESDMRResponse.h"
#include "BESRequestHandlerList.h"
#include "BESDapNames.h"
#include "BESDapTransmit.h"
#include "BESContextManager.h"
#include "GlobalMetadataStore.h"

#include "BESLog.h"
#include "BESDebug.h"

using namespace bes;
using namespace std;

#define MODULE "dap"
#define prolog std::string("BESDMRResponseHandler::").append(__func__).append("() - ")

BESDMRResponseHandler::BESDMRResponseHandler(const string &name) :
        BESResponseHandler(name)
{
}

BESDMRResponseHandler::~BESDMRResponseHandler()
{
}

/**
 * @brief executes the command `<get type-"dmr" definition="..">`
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

    // Look in the MDS for dhi.container.get_real_name().
    // if found, use that response, else build it.
    // If the MDS is disabled, don't use it.
    GlobalMetadataStore *mds = GlobalMetadataStore::get_instance();

    GlobalMetadataStore::MDSReadLock lock;

    dhi.first_container();
    if (mds) lock = mds->is_dmr_available(*(dhi.container));

    if (mds && lock() && dhi.container->get_dap4_constraint().empty() && dhi.container->get_dap4_function().empty()) {    // no CE
        // send the response
        mds->write_dmr_response(dhi.container->get_relative_name(), dhi.get_output_stream());
        // suppress transmitting a ResponseObject in transmit()
        d_response_object = 0;
    }
    else {
        DMR *dmr = 0;
        if (mds && lock() && dhi.container->get_dap4_function().empty()) {
            // If mds and lock(), the DDS is in the cache, get the _object_
            dmr = mds->get_dmr_object(dhi.container->get_relative_name());

            BESDMRResponse *bdmr = new BESDMRResponse(dmr);
            BESDEBUG(MODULE, prolog << "dmr->request_xml_base(): '"<< dmr->request_xml_base() <<
                "' (dmr: "<< (void *)  dmr  << ")" << endl);

            // This method sets the constraint for the current container. It does nothing
            // if there is no 'current container.'
            bdmr->set_dap4_constraint(dhi);
            bdmr->clear_container();

            d_response_object = bdmr;
            BESDEBUG(MODULE, prolog << "d_response_object: "<< (void *) d_response_object << endl);
        }
        else {
            dmr = new DMR();

            // if (xml_base_found && !xml_base.empty()) dmr->set_request_xml_base(xml_base);
            BESDMRResponse *bdmr = new BESDMRResponse(dmr);
            BESDEBUG(MODULE, prolog << "dmr->request_xml_base(): '"<< dmr->request_xml_base()
                << "' (dmr: "<< (void *)dmr << ")" << endl);

            d_response_object = bdmr;

            BESDEBUG(MODULE, prolog << "d_response_object: "<< (void *) d_response_object << endl);

            // The RequestHandlers set the constraint and reset the container(s)
            BESRequestHandlerList::TheList()->execute_each(dhi);

            dhi.first_container();  // must reset container; execute_each() iterates over all of them

#if ANNOTATION_SYSTEM
            // Support for the experimental Dataset Annotation system. jhrg 12/19/18
            if (!d_annotation_service_url.empty()) {
                unique_ptr<D4Attribute> annotation_url(new D4Attribute(DODS_EXTRA_ANNOTATION_ATTR, attr_str_c));
                annotation_url->add_value(d_annotation_service_url);

                // If there is already a DODS_EXTRA container, use it
                if (dmr->root() && dmr->root()->attributes()->get(DODS_EXTRA_ATTR_TABLE)) {
                    dmr->root()->attributes()->get(DODS_EXTRA_ATTR_TABLE)->attributes()->add_attribute_nocopy(annotation_url.release());
                }
                else {
                    // Make DODS_EXTRA and load the attribute into it.
                    unique_ptr<D4Attribute> dods_extra(new D4Attribute(DODS_EXTRA_ATTR_TABLE, attr_container_c));
                    dods_extra->attributes()->add_attribute_nocopy(annotation_url.release());

                    // If the root group is null, set the factory (this is an edge case!)
                    if (!dmr->root()) {
                        unique_ptr<D4BaseTypeFactory> factory(new D4BaseTypeFactory);
                        dmr->set_factory(factory.get());
                        dmr->root()->attributes()->add_attribute_nocopy(dods_extra.release());
                        dmr->set_factory(0);
                    }
                    else {
                        dmr->root()->attributes()->add_attribute_nocopy(dods_extra.release());
                    }
                }
            }
#endif
            // Cache the DMR if the MDS is not null but the response was not present, and..
            // This request does not contain a server function call.
            if (mds && !lock() && dhi.container->get_dap4_function().empty()) {
                mds->add_responses(static_cast<BESDMRResponse*>(d_response_object)->get_dmr(), dhi.container->get_relative_name());
            }
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

