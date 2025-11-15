// BESDataResponseHandler.h

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

#ifndef I_BESDataResponseHandler_H
#define I_BESDataResponseHandler_H

#include "BESResponseHandler.h"

/** @brief response handler that builds an OPeNDAP Data DDS object
 *
 * A request 'get data for <def_name>' will be handled by this
 * response handler. Given a definition name it determines what containers 
 * are to be used to build the response object. It then transmits the 
 * response object using the method send_data on the transmitter object.
 *
 * As for BESDap4ResponseHandler, this redirects accesses to the DMR++
 * handler when DMR++ responses exist in the MDS for the given item in
 * the container. Control this using BES.Use.Dmrpp and BES.Dmrpp.Name
 * as for BESDap4ResponseHandler.
 *
 * @see DDS
 * @see BESContainer
 * @see BESTransmitter
 * @see BESDefine
 * @see BESDap4ResponseHandler
 */
class BESDataResponseHandler: public BESResponseHandler {

    bool d_use_dmrpp;           ///< Check for DMR++ responses and redirect?
    std::string d_dmrpp_name;   ///< The name of the DMR++ module

public:
    BESDataResponseHandler(const std::string &name);
    virtual ~BESDataResponseHandler(void);

    virtual void execute(BESDataHandlerInterface &dhi);
    virtual void transmit(BESTransmitter *transmitter, BESDataHandlerInterface &dhi);

    void dump(std::ostream &strm) const override;

    static BESResponseHandler *DataResponseBuilder(const std::string &name);
};

#endif

