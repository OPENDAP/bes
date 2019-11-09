// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES, a component 
// of the Hyrax Data Server

// Copyright (c) 2016 OPeNDAP, Inc.
// Authors: Nathan Potter <ndp@opendap.org>
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


#ifndef DapFunctionUtils_h_
#define DapFunctionUtils_h_ 1

#include <string>

#include <DDS.h>
#include <ServerFunction.h>
#include "BESAbstractModule.h"

/** @brief Utilities used to help in the return of an OPeNDAP DataDDS
 * object as a netcdf file
 *
 * This class includes static functions to help with the conversion of
 * an OPeNDAP DataDDS object into a netcdf file.
 */
void promote_function_output_structures(libdap::DDS *fdds);
void function_dap2_wrapitup(int argc, libdap::BaseType *argv[], libdap::DDS &dds, libdap::BaseType **btpp);
libdap::BaseType *function_dap4_wrapitup(libdap::D4RValueList *dvl_args, libdap::DMR &dmr);


class WrapItUp: public libdap::ServerFunction {

private:

public:
    WrapItUp()
{
        setName("wrapitup");
        setDescriptionString(
            ((string)"This function returns a Structure whose name will invoke the '_unwrap' content activity"));
        setUsageString("wrapitup()");
        setRole("http://services.opendap.org/dap4/server-side-function/dap_function_utils/wrapitup");
        setDocUrl("http://docs.opendap.org/index.php/DapUtilFunctions");
        setFunction(function_dap2_wrapitup);
        setFunction(function_dap4_wrapitup);
        setVersion("1.0");
}
    virtual ~WrapItUp()
    {
    }

};



#endif // DapFunctionUtils_h_

