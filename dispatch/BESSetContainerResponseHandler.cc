// BESSetContainerResponseHandler.cc

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

#include "BESSetContainerResponseHandler.h"

#include "BESCatalogList.h"
#include "BESContainerStorage.h"
#include "BESContainerStorageList.h"
#include "BESDataHandlerInterface.h"
#include "BESDataNames.h"
#include "BESDebug.h"
#include "BESResponseNames.h"
#include "BESSyntaxUserError.h"
#include "BESUtil.h"

using std::endl;
using std::ostream;
using std::string;

#define MODULE "bes"
#define prolog std::string("BESSetContainerResponseHandler::").append(__func__).append("() - ")

BESSetContainerResponseHandler::BESSetContainerResponseHandler(const string &name) : BESResponseHandler(name) {}

BESSetContainerResponseHandler::~BESSetContainerResponseHandler() = default;

/** @brief executes the command to create a new container or replaces an
 * already existing container based on the provided symbolic name.
 *
 * The symbolic name is first looked up in the specified container storage
 * object, volatile if no container storage name is specified. If the
 * symbolic name already exists (the container already exists) then the
 * already existing container is removed and the new one added using the
 * given real name (usually a file name) and the type of data represented
 * by this container (e.g. cedar, cdf, netcdf, hdf, etc...)
 *
 *
 * @param dhi structure that holds request and response information
 * @throws BESSyntaxUserError if the specified store name does not exist
 * @see BESDataHandlerInterface
 * @see BESInfo
 * @see BESContainerStorageList
 * @see BESContainerStorage
 * @see BESContainer
 */
void BESSetContainerResponseHandler::execute(BESDataHandlerInterface &dhi) {
    BESDEBUG(MODULE, prolog << "store = " << dhi.data[STORE_NAME] << endl);
    BESDEBUG(MODULE, prolog << "symbolic = " << dhi.data[SYMBOLIC_NAME] << endl);
    BESDEBUG(MODULE, prolog << "real = " << dhi.data[REAL_NAME] << endl);
    BESDEBUG(MODULE, prolog << "type = " << dhi.data[CONTAINER_TYPE] << endl);

    BESContainerStorageList::TheList()->delete_container(dhi.data[SYMBOLIC_NAME]);

    BESContainerStorage *cp = BESContainerStorageList::TheList()->find_persistence(dhi.data[STORE_NAME]);
    if (cp) {
        cp->del_container(dhi.data[SYMBOLIC_NAME]);
        // FIXME Change this so that we make a container and then add it. Do not depend on the store
        // to make the container. This will require re-design of the Catalog/Container/ContainerStorage
        // classes and maybe the handlers. jhrg 1/7/19
        cp->add_container(dhi.data[SYMBOLIC_NAME], dhi.data[REAL_NAME], dhi.data[CONTAINER_TYPE]);
    } else {
        string ret = (string) "Unable to add container '" + dhi.data[SYMBOLIC_NAME] + "' to container storage '" +
                     dhi.data[STORE_NAME] + "'. Store does not exist.";
        throw BESSyntaxUserError(ret, __FILE__, __LINE__);
    }
}

/** @brief The setContainer command does not return a response.
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 *
 * @see BESInfo
 * @see BESTransmitter
 * @see BESDataHandlerInterface
 */
void BESSetContainerResponseHandler::transmit(BESTransmitter * /*transmitter*/, BESDataHandlerInterface & /*dhi*/) {}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESSetContainerResponseHandler::dump(ostream &strm) const {
    strm << BESIndent::LMarg << "BESSetContainerResponseHandler::dump - (" << (void *)this << ")" << endl;
    BESIndent::Indent();
    BESResponseHandler::dump(strm);
    BESIndent::UnIndent();
}

BESResponseHandler *BESSetContainerResponseHandler::SetContainerResponseBuilder(const string &name) {
    return new BESSetContainerResponseHandler(name);
}
