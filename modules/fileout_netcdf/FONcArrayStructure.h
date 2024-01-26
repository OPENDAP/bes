// FONcArrayStructure.h

// This file is part of BES Netcdf File Out Module

// // Copyright (c) The HDF Group, Inc. and OPeNDAP, Inc.
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


#ifndef FONcArrayStructure_h_
#define FONcArrayStructure_h_ 1

#include <libdap/Structure.h>
#include "FONcBaseType.h"
#include "FONcArrayStructureField.h"

namespace libdap {
    class Structure;
    class BaseType;
    class Array;
}


/** @brief A array of DAP Structure with file out netcdf information included
 *
 * This class represents an array of DAP Structure with additional information
 * needed to write it out to a netcdf file. 
 * Keeps the list of the structure members as a vector of FONcArrayStructureField 
 * instances.
 */
class FONcArrayStructure : public FONcBaseType
{
private:
    libdap::Array *			_as = nullptr;
    vector<FONcArrayStructureField *>	_vars ;
public:
    explicit FONcArrayStructure( libdap::BaseType *b ) ;
    ~FONcArrayStructure() override;

    void convert(vector<string> embed, bool _dap4=true, bool is_dap4_group=false) override;
    void		define( int ncid ) override;
    void		write( int ncid ) override;

    string 		name() override;

    void		dump( ostream &strm ) const override;
} ;

#endif // FONcArrayStructure_h_

