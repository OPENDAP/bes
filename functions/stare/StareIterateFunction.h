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

#include <string>

#include <dods-datatypes.h>

#include "ServerFunction.h"

namespace libdap {
class BaseType;

class DDS;

class D4RValueList;

class DMR;
}

namespace functions {

const std::string STARE_STORAGE_PATH = "FUNCTIONS.stareStoragePath";

bool hasValue(libdap::BaseType *bt, std::vector<libdap::dods_uint64> stareIndices);

unsigned int count(libdap::BaseType *bt, std::vector<libdap::dods_uint64> stareIndices);

libdap::BaseType *stare_intersection_dap4_function(libdap::D4RValueList *args, libdap::DMR &dmr);

#if 0
libdap::BaseType *stare_count_dap4_function(libdap::D4RValueList *args, libdap::DMR &dmr);
#endif

class StareIterateFunction : public libdap::ServerFunction {
public:
    static string get_sidecar_file_pathname(const string &pathName);
    static unsigned int count(libdap::BaseType *bt, std::vector<libdap::dods_uint64> stareIndices);

    friend class StareFunctionsTest;

public:
    StareIterateFunction() {
        setName("stare_intersection");
        setDescriptionString("The stare_intersection TODO: ");
        //setUsageString("linear_scale(var) | linear_scale(var,scale_factor,add_offset) | linear_scale(var,scale_factor,add_offset,missing_value)");
        //setRole("http://services.opendap.org/dap4/server-side-function/linear-scale");
        //setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions#linear_scale");
        setFunction(stare_intersection_dap4_function);
        setVersion("0.1");
    }

    virtual ~StareIterateFunction() {
    }
};

} // functions namespace
