// BESDap4ResponseHandler.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2013 OPeNDAP
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
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

#ifndef I_BESDap4ResponseHandler_h
#define I_BESDap4ResponseHandler_h 1

#include "BESResponseHandler.h"

/** @brief response handler that builds an OPeNDAP Dap4 data response
 *
 * @see DMR
 * @see BESContainer
 * @see BESTransmitter
 */
class BESDap4ResponseHandler: public BESResponseHandler {

    friend class Dap4ResponseHandlerTest;

public:
	BESDap4ResponseHandler(const string &name);
	virtual ~BESDap4ResponseHandler();

	virtual void execute(BESDataHandlerInterface &dhi);
	virtual void transmit(BESTransmitter *transmitter, BESDataHandlerInterface &dhi);

	virtual void dump(ostream &strm) const;

	static BESResponseHandler *Dap4ResponseBuilder(const string &name);
};

#endif // I_BESDap4ResponseHandler_h
