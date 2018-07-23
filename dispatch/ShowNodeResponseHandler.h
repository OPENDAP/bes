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

#include "BESResponseHandler.h"

namespace bes {

}
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
class ShowNodeResponseHandler: public BESResponseHandler {
private:
public:
    ShowNodeResponseHandler(const string &name);
    virtual ~ShowNodeResponseHandler(void);

    virtual void execute(BESDataHandlerInterface &dhi);
    virtual void transmit(BESTransmitter *transmitter, BESDataHandlerInterface &dhi);

    virtual void dump(ostream &strm) const;

    static BESResponseHandler *CatalogResponseBuilder(const string &name);
};

}

#endif // I_ShowNodeResponseHandler_h

