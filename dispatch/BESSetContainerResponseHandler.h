// BESSetContainerResponseHandler.h

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

#ifndef I_BESSetContainerResponseHandler_h
#define I_BESSetContainerResponseHandler_h 1

#include "BESResponseHandler.h"

/** @brief response handler that creates a container given the symbolic name,
 * real name, and data type.
 *
 * This request handler creates a new container, or replaces an already
 * existing container in the specified container storage given the symbolic
 * name, real name (in most cases a file name), and the type of data
 * represented by the real name (e.g. netcdf, cedar, cdf, hdf, etc...) The
 * request has the syntax:
 *
 * set container in &lt;store_name&gt; values &lt;sym_name&gt;,&lt;real_name&gt;,&lt;data_type&gt;;
 *
 * It returns whether the container was created or replaces successfully in an
 * informational response object and transmits that response object.
 *
 * @see BESResponseObject
 * @see BESContainer
 * @see BESTransmitter
 */
class BESSetContainerResponseHandler : public BESResponseHandler {
public:
    BESSetContainerResponseHandler(const std::string &name);
    virtual ~BESSetContainerResponseHandler(void);

    virtual void execute(BESDataHandlerInterface &dhi);
    virtual void transmit(BESTransmitter *transmitter, BESDataHandlerInterface &dhi);

    virtual void dump(std::ostream &strm) const;

    static BESResponseHandler *SetContainerResponseBuilder(const std::string &name);
};

#endif // I_BESSetContainerResponseHandler_h
