// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES

// Copyright (c) 2016 OPeNDAP, Inc.
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

#ifndef _dmrpp_array_dc_h
#define _dmrpp_array_dc_h 1

#include "DmrppArray.h"

class DmrppArrayDC : public dmrpp::DmrppArray{

private:
    bool direct_io_flag = false;
public:
    bool read();
    void set_dio_flag(bool);
    bool check_dio_flag();
    bool get_dio_flag() { return direct_io_flag; };
    DmrppArrayDC(const string &n, BaseType *v, shared_ptr<dmrpp::DMZ> dmz) :
            DmrppArray(n,v,dmz)
            { }
};

#endif // _dmrpp_array_dc_h

