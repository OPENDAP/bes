// -*- mode: c++; c-basic-offset:4 -*-
//
// W10NResponseHandler.h
//
// This file is part of BES w10n handler
//
// Copyright (c) 2015v OPeNDAP, Inc.
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

#ifndef I_ShowPathInfoResponseHandler_h
#define I_ShowPathInfoResponseHandler_h 1

#include <string>
#include <ostream>

#include "BESResponseHandler.h"

#define SHOW_PATH_INFO_RESPONSE "show.pathInfo"

/** @brief response handler that returns nodes or leaves within the catalog
 * either at the root or at a specified node.
 *
 * A request 'show catalog [for &lt;node&gt;];' or
 * 'show leaves for &lt;node&gt;;
 * will be handled by this response handler. It returns nodes or leaves either
 * at the root level if no node is specified in the request, or the nodes or
 * leaves under the specified node.
 *
 * @see BESResponseObject
 * @see BESContainer
 * @see BESTransmitter
 */
class ShowPathInfoResponseHandler: public BESResponseHandler {
public:
    void eval_resource_path(const std::string &resource_path, const std::string &catalog_root, const bool follow_sym_links,
        std::string &validPath, bool &isFile, bool &isDir, long long &size, long long &lastModifiedTime, bool &canRead,
        std::string &remainder);

public:
    ShowPathInfoResponseHandler(const std::string &name);
    virtual ~ShowPathInfoResponseHandler(void);

    virtual void execute(BESDataHandlerInterface &dhi);
    virtual void transmit(BESTransmitter *transmitter, BESDataHandlerInterface &dhi);

    virtual void dump(std::ostream &strm) const;

    static BESResponseHandler *ShowPathInfoResponseBuilder(const std::string &name);
};

#endif // I_ShowPathInfoResponseHandler_h

