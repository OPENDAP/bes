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

#include "BESSetContainerResponseHandler.h"

#if 0
#include "BESSilentInfo.h"
#endif


#include "BESContainerStorageList.h"
#include "BESContainerStorage.h"
#include "BESDataNames.h"
#include "BESSyntaxUserError.h"
#include "BESResponseNames.h"
#include "BESDebug.h"

BESSetContainerResponseHandler::BESSetContainerResponseHandler(const string &name) :
        BESResponseHandler(name)
{
}

BESSetContainerResponseHandler::~BESSetContainerResponseHandler()
{
}

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
void BESSetContainerResponseHandler::execute(BESDataHandlerInterface &dhi)
{
#if 0
	dhi.action_name = SETCONTAINER_STR;
    BESInfo *info = new BESSilentInfo();
    d_response_object = info;
#endif

    string store_name = dhi.data[STORE_NAME];
    string symbolic_name = dhi.data[SYMBOLIC_NAME];
    string real_name = dhi.data[REAL_NAME];
    string container_type = dhi.data[CONTAINER_TYPE];

    BESDEBUG("bes", "BESSetContainerResponseHandler::execute store = " << dhi.data[STORE_NAME] << endl);
    BESDEBUG("bes", "BESSetContainerResponseHandler::execute symbolic = " << dhi.data[SYMBOLIC_NAME] << endl);
    BESDEBUG("bes", "BESSetContainerResponseHandler::execute real = " << dhi.data[REAL_NAME] << endl);
    BESDEBUG("bes", "BESSetContainerResponseHandler::execute type = " << dhi.data[CONTAINER_TYPE] << endl);

    BESContainerStorage *cp = BESContainerStorageList::TheList()->find_persistence(store_name);
    if (cp) {
        BESDEBUG("bes", "BESSetContainerResponseHandler::execute adding the container..." << endl);

        cp->del_container(symbolic_name);
        cp->add_container(symbolic_name, real_name, container_type);

        BESDEBUG("bes", "BESSetContainerResponseHandler::execute Done" << endl);
    }
    else {
        string ret = (string) "Unable to add container '" + symbolic_name + "' to container storage '" + store_name
                + "'. Store does not exist.";
        throw BESSyntaxUserError(ret, __FILE__, __LINE__);
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
void BESSetContainerResponseHandler::transmit(BESTransmitter */*transmitter*/, BESDataHandlerInterface &/*dhi*/)
{
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
void BESSetContainerResponseHandler::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESSetContainerResponseHandler::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESResponseHandler::dump(strm);
    BESIndent::UnIndent();
}

BESResponseHandler *
BESSetContainerResponseHandler::SetContainerResponseBuilder(const string &name)
{
    return new BESSetContainerResponseHandler(name);
}

