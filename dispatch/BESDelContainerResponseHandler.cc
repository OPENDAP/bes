// BESDelContainerResponseHandler.cc

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

#include "BESDelContainerResponseHandler.h"

#if 0
#include "BESSilentInfo.h"
#endif

#include "BESContainer.h"
#include "BESContainerStorage.h"
#include "BESContainerStorageList.h"
#include "BESDataHandlerInterface.h"
#include "BESDataNames.h"
#include "BESDefine.h"
#include "BESDefinitionStorage.h"
#include "BESDefinitionStorageList.h"
#include "BESResponseNames.h"
#include "BESSyntaxUserError.h"

using std::endl;
using std::ostream;
using std::string;

BESDelContainerResponseHandler::BESDelContainerResponseHandler(const string &name) : BESResponseHandler(name) {}

BESDelContainerResponseHandler::~BESDelContainerResponseHandler() = default;

/** @brief executes the command to delete a container
 *
 * Removes a specified container from a specified container store. If no
 * container store is specified, the default is volatile.
 *
 * The response built is a silent informational object. The only response
 * that a client would receive would be if there were an exception thrown
 * attempting to delete the container.
 *
 * @param dhi structure that holds request and response information
 * @throws BESSyntaxUserError if unable to delete the specified container
 * from the specified container store
 * @see BESDataHandlerInterface
 * @see BESSilentInfo
 * @see BESContainer
 * @see BESContainerStorage
 * @see BESContainerStorageList
 */
void BESDelContainerResponseHandler::execute(BESDataHandlerInterface &dhi) {
#if 0
	dhi.action_name = DELETE_CONTAINER_STR;
    BESInfo *info = new BESSilentInfo();
    d_response_object = info;
#endif

    string container_name = dhi.data[CONTAINER_NAME];
    string store_name = dhi.data[STORE_NAME];
    if (container_name != "") {
        if (store_name == "")
            store_name = CATALOG /* DEFAULT jhrg 12/27/18 */;
        BESContainerStorage *cp = BESContainerStorageList::TheList()->find_persistence(store_name);
        if (cp) {
            bool deleted = cp->del_container(dhi.data[CONTAINER_NAME]);
            if (!deleted) {
                string err_str = (string) "Unable to delete container. " + "The container \"" +
                                 dhi.data[CONTAINER_NAME] + "\" does not exist in container storage \"" +
                                 dhi.data[STORE_NAME] + "\"";
                throw BESSyntaxUserError(err_str, __FILE__, __LINE__);
            }
        } else {
            string err_str = (string) "Container storage \"" + dhi.data[STORE_NAME] + "\" does not exist. " +
                             "Unable to delete container \"" + dhi.data[CONTAINER_NAME] + "\"";
            throw BESSyntaxUserError(err_str, __FILE__, __LINE__);
        }
    } else {
        string err_str = (string) "No container is specified. " + "Unable to complete request.";
        throw BESSyntaxUserError(err_str, __FILE__, __LINE__);
    }
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
void BESDelContainerResponseHandler::transmit(BESTransmitter * /*transmitter*/, BESDataHandlerInterface & /*dhi*/) {
#if 0
    if (d_response_object) {
        BESInfo *info = dynamic_cast<BESInfo *>(d_response_object);
        if (!info)
            throw BESInternalError("cast error", __FILE__, __LINE__);
        info->transmit(transmitter, dhi);
    }
#endif
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESDelContainerResponseHandler::dump(ostream &strm) const {
    strm << BESIndent::LMarg << "BESDelContainerResponseHandler::dump - (" << (void *)this << ")" << endl;
    BESIndent::Indent();
    BESResponseHandler::dump(strm);
    BESIndent::UnIndent();
}

BESResponseHandler *BESDelContainerResponseHandler::DelContainerResponseBuilder(const string &name) {
    return new BESDelContainerResponseHandler(name);
}
