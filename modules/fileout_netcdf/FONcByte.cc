// FONcByte.cc

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
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <BESInternalError.h>
#include <BESDebug.h>

#include "FONcByte.h"
#include "FONcUtils.h"
#include "FONcAttributes.h"

/** @brief Constructor for FONcByte that takes a DAP Byte
 *
 * This constructor takes a DAP BaseType and makes sure that it is a DAP
 * Byte instance. If not, it throws an exception
 *
 * @param b A DAP BaseType that should be a byte
 * @throws BESInternalError if the BaseType is not a Byte
 */
FONcByte::FONcByte(BaseType *b)
        : FONcBaseType(), _b(nullptr) {
    _b = dynamic_cast<Byte *>(b);
    if (!_b) {
        string s = (string) "File out netcdf, FONcByte was passed a "
                   + "variable that is not a DAP Byte";
        throw BESInternalError(s, __FILE__, __LINE__);
    }
}

/** @brief define the DAP Byte in the netcdf file
 *
 * The definition actually takes place in FONcBaseType. This function
 * adds the attributes for the Byte instance as well as an attribute if
 * the name of the Byte had to be modified.
 *
 * @param ncid The id of the NetCDF file
 * @throws BESInternalError if there is a problem defining the
 * Byte
 */
void
FONcByte::define(int ncid) {
    FONcBaseType::define(ncid);

    if (!_defined) {
        if (is_dap4) {
            D4Attributes *d4_attrs = _b->attributes();
            updateD4AttrType(d4_attrs, NC_UBYTE);
        }
        else {
            AttrTable &attrs = _b->get_attr_table();
            updateAttrType(attrs, NC_UBYTE);
        }

        FONcAttributes::add_variable_attributes(ncid, _varid, _b, isNetCDF4_ENHANCED(), is_dap4);
        FONcAttributes::add_original_name(ncid, _varid, _varname, _orig_varname);

        _defined = true;
    }
}

/** @brief Write the byte out to the netcdf file
 *
 * Once the byte is defined, the value of the byte can be written out
 * as well.
 *
 * @param ncid The id of the netcdf file
 * @throws BESInternalError if there is a problem writing the value out
 * to the netcdf file
 */
void
FONcByte::write(int ncid) {
    BESDEBUG("fonc", "FOncByte::write for var " << _varname << endl);

    if (is_dap4)
        _b->intern_data();
    else
        _b->intern_data(*get_eval(), *get_dds());

    // For scalar types, assign the value to a local variable. Eliminate the
    // allocation of dynamic memory as well as the delete call. The amount of
    // memory used in this case is too small to warrant any more optimization.
    unsigned char data = _b->value();
    size_t var_index[] = {0};
    int stax = nc_put_var1_uchar(ncid, _varid, var_index, &data);
    if (stax != NC_NOERR) {
        string err = string("fileout.netcdf - Failed to write byte data for ") + _varname;
        FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
    }
}

/** @brief dumps information about this object for debugging purposes
 *
 * Displays the pointer value of this instance plus instance data
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
FONcByte::dump(ostream &strm) const {
    strm << BESIndent::LMarg << "FONcByte::dump - ("
         << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "name = " << _b->name() << endl;
    BESIndent::UnIndent();
}

