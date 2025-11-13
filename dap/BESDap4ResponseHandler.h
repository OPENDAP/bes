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
 * This ResponseHandler is used to build DAP4 data responses.
 *
 * This class looks in the MDS for cached/stored DMR++ responses and, if found,
 * will re-direct the request to the DMR++ requestHandler, regardless
 * of the dataset's format and/or RequestHandler as specified by the
 * bes.conf TypeMatch configuration. However, this can be suppressed
 * using a configuration parameter in the bes.conf file. If the bes
 * key BES.Use.Dmrpp is not yes, the handler does not look in the MDS
 * for DMR++ responses and no attempt to redirect the request is made.
 * The bes key BES.Dmrpp.Name should be set to the name of the DMR++
 * handler used in the key BES.modules.
 *
 * @see DMR
 * @see BESContainer
 * @see BESTransmitter
 */
class BESDap4ResponseHandler: public BESResponseHandler {

    bool d_use_dmrpp;           ///< Check for DMR++ responses and redirect?
    std::string d_dmrpp_name;   ///< The name of the DMR++ module

    friend class Dap4ResponseHandlerTest;

public:
	BESDap4ResponseHandler(const std::string &name);
	virtual ~BESDap4ResponseHandler();

	virtual void execute(BESDataHandlerInterface &dhi);
	virtual void transmit(BESTransmitter *transmitter, BESDataHandlerInterface &dhi);

	/**
	 * @brief Is the BES.Use.Dmrpp key set in the bes.conf?
	 * @return True or false.
	 */
	virtual bool get_use_dmrpp() const {
	    return d_use_dmrpp;
	}

	/**
	 * @brief Get the name of the DMR++ handler.
	 * @return The value of the BES.Dmrpp.Name bes key.
	 */
	virtual std::string get_dmrpp_name() const {
	    return d_dmrpp_name;
	}

	void dump(std::ostream &strm) const override;

	static BESResponseHandler *Dap4ResponseBuilder(const std::string &name);
};

#endif // I_BESDap4ResponseHandler_h
