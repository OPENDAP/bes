// BESDapNullAggregationServer.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2015 OPeNDAP, Inc.
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include <iostream>

#include "BESDapNullAggregationServer.h"

/** @brief Simple aggregation method that prints the DHI
 * To learn more about the stuff that an aggregation can do and what
 * it has to work with, I've written this 'null' aggregation method.
 * It doesn't perform any aggregation, but it does dump the DHI it
 * gets as a parameter.
 * @param dhi
 */
void BESDapNullAggregationServer::aggregate(BESDataHandlerInterface &dhi)
{
    // Print out the dhi info
    dhi.dump(std::cerr);
}

void BESDapNullAggregationServer::dump(ostream &strm) const
{
    BESAggregationServer::dump(strm);
}
