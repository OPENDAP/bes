// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2013 OPeNDAP, Inc.
// Authors: Kodi Neumiller <kneumiller@opendap.org>
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
#include <stdint.h>

#include "ServerFunction.h"

namespace libdap {
class BaseType;
class DDS;
}

namespace functions {

bool hasValue(libdap::BaseType *bt, std::vector<uint64_t> stareIndices);

int count(libdap::BaseType *bt, std::vector<uint64_t> stareIndices);



libdap::BaseType *stare_dap4_function(libdap::D4RValueList *args, libdap::DMR &dmr);

class StareIterateFunction: public libdap::ServerFunction {
public:
	StareIterateFunction()
	{
		setName("stare_iterate");
		setDescriptionString("The stare_iterate() function will iterate over a given variable and generate two vectors: one for the stare indices and one for the matching data values");
		//setUsageString("linear_scale(var) | linear_scale(var,scale_factor,add_offset) | linear_scale(var,scale_factor,add_offset,missing_value)");
		//setRole("http://services.opendap.org/dap4/server-side-function/linear-scale");
		//setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions#linear_scale");
		setFunction(stare_dap4_function);
		setVersion("0.1");
	}
	virtual ~StareIterateFunction()
	{
	}
};

} // functions namespace
