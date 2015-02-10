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

#include "BESDapSequenceAggregationServer.h"

/** @brief Aggregate the Sequences in one or more containers
 *
 * Assume this is called as part of a <define> that has a number of containers.
 * Each container is represented as a Structure and within that is a single
 * Sequence. Each 'container's Sequence' has columns with the same number, type
 * and name along with all of its values. Build an 'aggregated sequence' that
 * uses those columns and the concatenation of all of the values.
 * @param dhi
 */
void BESDapSequenceAggregationServer::aggregate(BESDataHandlerInterface &dhi)
{
    // Print out the dhi info
    dhi.dump(std::cerr);
#if 0
    // Get the response object
    BESResponseObject *response = dhi.response_handler->get_response_object() ;
    BESDataDDSResponse *bes_data_dds = dynamic_cast < BESDataDDSResponse * >(response) ;
    if( !bes_data_dds )
       throw BESInternalError( "Expected a DataDDS", __FILE__, __LINE__ ) ;

    // Test for a valid DataDDS - must have zero or more Structures and
    // each Structure must contain one Sequence and each of those Sequences'
    // elements must match in terms of number, type and name. Name is maybe
    // not required. The Sequences can have different numbers of rows.

    // Instead of building a new Sequence for the result, use the first
    // Sequence and append values to it from the N-1 subsequent sequences
#endif
}

void BESDapSequenceAggregationServer::dump(ostream &strm) const
{
    BESAggregationServer::dump(strm);
}
