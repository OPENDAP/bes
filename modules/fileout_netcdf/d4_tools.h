// d4_tools.h

// This file is part of BES Netcdf File Out Module

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
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
//      slloyd      Samuel Lloyd <slloyd@opendap.org>

#ifndef d4_tools_h_
#define d4_tools_h_ 1

#include <string>
#include <vector>
#include <map>
#include <set>

#include <BESObj.h>

namespace libdap {
    class DDS;
    class DMR;
    class D4Group;
    class D4Attribute;
    class D4Attributes;
    class BaseType;
}

class d4_tools: public BESObj {
public:
    d4_tools();
    virtual ~d4_tools();
    bool is_dap4_projected(libdap::D4Group *group, std::vector<libdap::BaseType *> &projected_dap4_variable_inventory);
    bool has_dap4_types(libdap::D4Attribute   *attr);
    bool has_dap4_types(libdap::D4Attributes *attrs);
};



#endif // d4_tools_h_