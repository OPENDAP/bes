// -*- mode: c++; c-basic-offset:4 -*-
//
// ShowBesKeyResponseHandler.h
//
// This file is part of the BES default command set
//
// Copyright (c) 2018 OPeNDAP, Inc.
// Author: Nathan Potter <ndp@opendap.org>
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
// Please read the full copyright statement in the file COPYRIGHT_URI.
//

#ifndef I_ShowBesKeyResponseHandler_h
#define I_ShowBesKeyResponseHandler_h 1

#include <string>
#include <vector>
#include <ostream>
#include "BESResponseHandler.h"
#include "ShowBesKeyCommand.h"

/** @brief Response handler that returns the value(s) of a BES key
 *
 * @see BESResponseObject
 * @see BESContainer
 * @see BESTransmitter
 */
class ShowBesKeyResponseHandler: public BESResponseHandler {
public:
    void eval_resource_path(const std::string &resource_path, const std::string &catalog_root, const bool follow_sym_links,
        std::string &validPath, bool &isFile, bool &isDir, long long &size, long long &lastModifiedTime, bool &canRead,
        std::string &remainder);

    ShowBesKeyResponseHandler(const std::string &name);
    virtual ~ShowBesKeyResponseHandler();

    virtual void execute(BESDataHandlerInterface &dhi);
    virtual void transmit(BESTransmitter *transmitter, BESDataHandlerInterface &dhi);

    void dump(std::ostream &strm) const override;

    static BESResponseHandler *ShowBesKeyResponseBuilder(const std::string &name);

    virtual void getBesKeyValue(std::string key,  std::vector<std::string> &values);
};

#endif // I_ShowBesKeyResponseHandler_h

