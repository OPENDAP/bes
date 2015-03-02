// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include "config.h"

#include <vector>

#include <Error.h>

#include "Odometer.h"

using namespace std;

namespace libdap {

// documentation in the header file
unsigned int
Odometer::next()
{
	if (d_offset == end())
		throw Error("Attempt to move beyond the end of an array in the indexing software.");

#if 1
	// About 3.1 seconds for 10^9 elements
	vector<unsigned int>::reverse_iterator si = d_shape.rbegin();
	for (vector<unsigned int>::reverse_iterator i = d_indices.rbegin(), e = d_indices.rend(); i != e; ++i, ++si) {
		if (++(*i) == *si) {
			*i = 0;
		}
		else {
			break;
		}
	}
#else
	// ... about 3.2 seconds for this code
	// TODO Unroll this using a switch()
	unsigned int i = d_rank - 1;
	do {
		if (++d_indices[i] == d_shape[i]) {
			d_indices[i] = 0;
		}
		else {
			break;
		}

	} while (i-- > 0);
#endif

	return ++d_offset;
}

} // namespace libdap
