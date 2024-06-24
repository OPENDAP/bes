// FONcArrayStructureField.h

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


#ifndef FONcArrayStructureField_h_
#define FONcArrayStructureField_h_ 1

#include <vector>
#include <string>

#include <netcdf.h>

#include <libdap/Array.h>
#include "FONcBaseType.h"

class FONcDim;
class FONcMap;

namespace libdap {
class BaseType;
}

/** @brief A DAP Array with file out netcdf information included
 *
 * This class represents a DAP Array with additional information
 * needed to write it out to a netcdf file. Includes a reference to the
 * actual DAP Array being converted.
 */
class FONcArrayStructureField: public FONcBaseType {
private:

    // The structure array being converted, not the structure array field
    libdap::Array *d_a = nullptr;
    // The type of data stored in the array
    nc_type d_array_type = NC_NAT;
    string var_name;
    std::vector<FONcDim *> struct_dims;
    vector<size_t> struct_dim_sizes;

    // The following variables are not used for structure string handling.
    size_t total_nelements = 1;
    size_t field_nelements = 1;
    size_t d_array_type_size;

    // The netcdf dimension ids for this array
    std::vector<int> d_dim_ids{};

    FONcDim * find_sdim(const std::string &name, int64_t size);
    void obtain_scalar_data(char *data_buf_ptr, libdap::BaseType* b) const;
    size_t obtain_maximum_string_length();
    void handle_structure_string_field(libdap::BaseType *b);
    void write_str(int ncid);
public:

    explicit FONcArrayStructureField(libdap::BaseType *b, libdap::Array* a, bool is_netCDF4_enhanced);
    ~FONcArrayStructureField();

    void convert(vector<string> embed, bool _dap4=true, bool is_dap4_group=false) override;
    void define(int ncid) override;
    void write(int ncid) override;

    std::string name() override;
    nc_type type() override;

    void dump(std::ostream &strm) const override;

    static std::vector<FONcDim *> SDimensions;

};

#endif // FONcArrayStructureField_h_

