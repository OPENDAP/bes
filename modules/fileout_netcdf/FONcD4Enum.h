// FONcD4Enum.h

// This file is part of BES Netcdf File Out Module

// Author: Kent Yang
// <myang6@hdfgroup.org> 

// Copyright (c)  The HDF Group, Inc. and OPeNDAP, Inc.
//
// This is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your
// option) any later version.
//
// This software is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// You can contact The HDF Group, Inc. at 410 E University Ave,
// Suite 200, Champaign, IL 61820



#ifndef FONcD4Enum_h_
#define FONcD4Enum_h_ 

#include <libdap/D4Enum.h>
#include <libdap/D4EnumDefs.h>
#include <libdap/D4Group.h>
#include "FONcBaseType.h"

namespace libdap {
    class BaseType;
    class D4Enum;
}


/** @brief A DAP4 Enum with file out netcdf information included
 *
 * This class represents a DAP4 Enum with additional information
 * needed to write it out to a netcdf file. Includes a reference to the
 * actual DAP4 Enum being converted
 */
class FONcD4Enum : public FONcBaseType
{
private:
    libdap::D4Enum *			d_f = nullptr;
    nc_type                             d_basetype = NC_NAT;
    int                                 d_nc_enum_type_id = NC_EBADTYPID;
public:
    FONcD4Enum( libdap::BaseType *b, nc_type d4_enum_basetype, int nc_type_id ) ;
    ~FONcD4Enum() override = default;

    void define(int ncid) override;
    void write( int ncid ) override;

    string name() override;
    
    void dump( ostream &strm ) const override;

} ;

#endif // FONcD4Enum_h_

