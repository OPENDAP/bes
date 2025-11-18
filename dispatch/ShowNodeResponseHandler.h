// ShowNodeResponseHandler.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2018 OPeNDAP, Inc
// Author: James Gallagher <jgallagher@opendap.org>
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

#ifndef I_ShowNodeResponseHandler_h
#define I_ShowNodeResponseHandler_h 1

#include <ostream>
#include <string>

#include "BESResponseHandler.h"

class BESDataHandlerInterface;
class BESTransmitter;

namespace bes {

/**
 * @brief Evaluate a showNode command
 *
 * Evaluate a showNode command using information parsed by the ShowNodeCommand
 * class. The execute() method builds an info object that is then returned
 * to the caller using the transmit() method.
 *
 * @see BESResponseObject
 * @see BESContainer
 * @see BESTransmitter
 */
class ShowNodeResponseHandler : public BESResponseHandler {
private:
public:
    ShowNodeResponseHandler(const std::string &name) : BESResponseHandler(name) {}

    ~ShowNodeResponseHandler() override = default;

    void execute(BESDataHandlerInterface &dhi) override;
    void transmit(BESTransmitter *transmitter, BESDataHandlerInterface &dhi) override;

    void dump(std::ostream &strm) const override;

    static BESResponseHandler *ShowNodeResponseBuilder(const std::string &name);
};

} // namespace bes

#endif // I_ShowNodeResponseHandler_h
