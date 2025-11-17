// BESShowContainersResponseHandler.h

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

#ifndef I_BESShowContainersResponseHandler_h
#define I_BESShowContainersResponseHandler_h 1

#include "BESResponseHandler.h"

/** @brief response handler that returns list of continers currently defined
 * in all container storage lists.
 *
 * A request 'show containers;' will be handled by this response handler. It
 * returns the list of currently defined containers for each container
 * storage registered with the server and transmits the response as an
 * informational response.
 *
 * @see BESResponseObject
 * @see BESContainerStorage
 * @see BESContainerStorageList
 * @see BESContainer
 * @see BESTransmitter
 */
class BESShowContainersResponseHandler : public BESResponseHandler {
public:
    BESShowContainersResponseHandler(const std::string &name);
    ~BESShowContainersResponseHandler(void) override;

    void execute(BESDataHandlerInterface &dhi) override;
    void transmit(BESTransmitter *transmitter, BESDataHandlerInterface &dhi) override;

    void dump(std::ostream &strm) const override;

    static BESResponseHandler *ShowContainersResponseBuilder(const std::string &name);
};

#endif // I_BESShowContainersResponseHandler_h
