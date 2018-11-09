// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES, A C++ implementation of the OPeNDAP
// Hyrax data server

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

#include <ServerFunction.h>

namespace libdap {
class BaseType;
class DDS;

class D4RValueList;
class DMR;
}

namespace functions {

void function_dap2_bbox_comb(int argc, libdap::BaseType *argv[], libdap::DDS &dds, libdap::BaseType **btpp);

class BBoxCombFunction: public libdap::ServerFunction
{
public:
    BBoxCombFunction(){
        setName("bbox_comb");
        setDescriptionString("The bbox_comb() function combines two bounding boxes with different shapes, forming their combination.");
        setUsageString("bbox_comb(<bb1>, <bb2>)");
        setRole("http://services.opendap.org/dap4/server-side-function/bbox_comb");
        setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions#bbox_comb");
        setFunction(function_dap2_bbox_comb);
        setVersion("1.0");
    }
    virtual ~BBoxCombFunction(){
    }
};

} /* namespace functions */


