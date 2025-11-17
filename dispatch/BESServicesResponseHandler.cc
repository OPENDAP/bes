// BESServicesResponseHandler.cc

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

#include "BESServicesResponseHandler.h"
#include "BESInfo.h"
#include "BESInfoList.h"
#include "BESResponseNames.h"
#include "BESServiceRegistry.h"

using std::endl;
using std::ostream;
using std::string;

BESServicesResponseHandler::BESServicesResponseHandler(const string &name) : BESResponseHandler(name) {}

BESServicesResponseHandler::~BESServicesResponseHandler() {}

/** @brief executes the command 'show services;' by returning the list of
 * all registered services for this BES.
 *
 * This response handler knows how to retrieve the list of services in
 * the BES. A BESInfo informational response object is built to hold all
 * of the services
 *
 * @param dhi structure that holds request and response information
 * @see BESDataHandlerInterface
 * @see BESInfo
 * @see BESServiceRegistry
 */
void BESServicesResponseHandler::execute(BESDataHandlerInterface &dhi) {
    BESInfo *info = BESInfoList::TheList()->build_info();
    d_response_object = info;

    dhi.action_name = SERVICE_RESPONSE_STR;
    info->begin_response(SERVICE_RESPONSE_STR, dhi);
    BESServiceRegistry::TheRegistry()->show_services(*info);
    info->end_response();
}

/** @brief transmit the response object built by the execute command
 * using the specified transmitter object
 *
 * If a response object was built then transmit it as text using the specified
 * transmitter object.
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see BESInfo
 * @see BESTransmitter
 * @see BESDataHandlerInterface
 */
void BESServicesResponseHandler::transmit(BESTransmitter *transmitter, BESDataHandlerInterface &dhi) {
    if (d_response_object) {
        BESInfo *info = dynamic_cast<BESInfo *>(d_response_object);
        if (!info)
            throw BESInternalError("cast error", __FILE__, __LINE__);
        info->transmit(transmitter, dhi);
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESServicesResponseHandler::dump(ostream &strm) const {
    strm << BESIndent::LMarg << "BESServicesResponseHandler::dump - (" << (void *)this << ")" << endl;
    BESIndent::Indent();
    BESResponseHandler::dump(strm);
    BESIndent::UnIndent();
}

BESResponseHandler *BESServicesResponseHandler::ResponseBuilder(const string &name) {
    return new BESServicesResponseHandler(name);
}
