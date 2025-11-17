// BESStatusResponseHandler.h

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

#ifndef I_BESStatusResponseHandler_h
#define I_BESStatusResponseHandler_h 1

#include "BESResponseHandler.h"

/** @brief response handler that returns the status of the server process
 * serving the requesting client
 *
 * A request 'show status;' will be handled by this response handler. It
 * returns the status of the server process handling the clients requests in
 * an informational response object.
 *
 * @see BESResponseObject
 * @see BESContainer
 * @see BESTransmitter
 */
class BESStatusResponseHandler : public BESResponseHandler {
public:
    BESStatusResponseHandler() = delete;
    BESStatusResponseHandler(const BESStatusResponseHandler &) = delete;
    BESStatusResponseHandler &operator=(const BESStatusResponseHandler &) = delete;

    explicit BESStatusResponseHandler(const std::string &name) : BESResponseHandler(name) {}

    ~BESStatusResponseHandler() override = default;

    void execute(BESDataHandlerInterface &dhi) override;

    void transmit(BESTransmitter *transmitter, BESDataHandlerInterface &dhi) override;

    void dump(std::ostream &strm) const override;

    static BESResponseHandler *StatusResponseBuilder(const std::string &name) {
        return new BESStatusResponseHandler(name);
    }
};

#endif // I_BESStatusResponseHandler_h
