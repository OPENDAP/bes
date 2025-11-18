// BESShowContainersResponseHandler.cc

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

#include "BESShowContainersResponseHandler.h"
#include "BESContainerStorageList.h"
#include "BESInfo.h"
#include "BESInfoList.h"
#include "BESResponseNames.h"

using std::endl;
using std::ostream;
using std::string;

BESShowContainersResponseHandler::BESShowContainersResponseHandler(const string &name) : BESResponseHandler(name) {}

BESShowContainersResponseHandler::~BESShowContainersResponseHandler() = default;

/** @brief executes the command 'show containers;' by returning the list of
 * currently defined containers in all container stores
 *
 * This response handler knows how to retrieve information about all
 * containers stored with any container storage objects. This response handler
 * creates a BESInfo response object to store the container information.
 *
 * @param dhi structure that holds request and response information
 * @see BESDataHandlerInterface
 * @see BESInfo
 * @see BESContainerStorageList
 * @see BESContainerStorage
 * @see BESContainer
 */
void BESShowContainersResponseHandler::execute(BESDataHandlerInterface &dhi) {
    BESInfo *info = BESInfoList::TheList()->build_info();
    d_response_object = info;
    dhi.action_name = SHOWCONTAINERS_RESPONSE_STR;
    info->begin_response(SHOWCONTAINERS_RESPONSE_STR, dhi);
    BESContainerStorageList::TheList()->show_containers(*info);
    info->end_response();
}

/** @brief transmit the response object built by the execute command
 * using the specified transmitter object
 *
 * If a response object was built (BESInfo) then transmit it as text.
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see BESInfo
 * @see BESTransmitter
 * @see BESDataHandlerInterface
 */
void BESShowContainersResponseHandler::transmit(BESTransmitter *transmitter, BESDataHandlerInterface &dhi) {
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
void BESShowContainersResponseHandler::dump(ostream &strm) const {
    strm << BESIndent::LMarg << "BESShowContainersResponseHandler::dump - (" << (void *)this << ")" << endl;
    BESIndent::Indent();
    BESResponseHandler::dump(strm);
    BESIndent::UnIndent();
}

BESResponseHandler *BESShowContainersResponseHandler::ShowContainersResponseBuilder(const string &name) {
    return new BESShowContainersResponseHandler(name);
}
