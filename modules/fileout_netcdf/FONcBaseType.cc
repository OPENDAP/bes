// FONcBaseType.cc

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

#include <libdap/D4Attributes.h>

#include <BESInternalError.h>
#include <BESDebug.h>

#include "FONcBaseType.h"
#include "FONcUtils.h"

using namespace libdap;

void FONcBaseType::convert(const vector<string> embed, bool _dap4, bool dap4_group)
{
    d_embed = embed;
    d_varname = name();
    d_is_dap4_group = dap4_group;
    d_is_dap4 = _dap4;
}

/** @brief Define the variable in the netcdf file
 *
 * This method creates this variable in the netcdf file. This
 * implementation is used for only simple types (byte, short, int,
 * float, double), and not for the complex types (str, structure, array,
 * grid, sequence)
 *
 * @param ncid Id of the NetCDF file
 * @throws BESInternalError if defining the variable fails
 */
void FONcBaseType::define(int ncid)
{
    if (!d_defined) {
        d_varname = FONcUtils::gen_name(d_embed, d_varname, d_orig_varname);
        BESDEBUG("fonc", "FONcBaseType::define - defining '" << d_varname << "'" << endl);
        int stax = nc_def_var(ncid, d_varname.c_str(), type(), 0, nullptr, &d_varid);
        if (stax != NC_NOERR) {
            string err = (string) "fileout.netcdf - " + "Failed to define variable " + d_varname;
            FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
        }
        stax = nc_def_var_fill(ncid, d_varid, NC_NOFILL, NULL );
        if (stax != NC_NOERR) {
            string err = (string) "fileout.netcdf - " + "Failed to clear fill value for " + d_varname;
            FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
        }

        BESDEBUG("fonc", "FONcBaseType::define - done defining " << d_varname << endl);
    }
}

/** @brief Returns the type of data of this variable
 *
 * This implementation of the method returns the default type of data.
 * Subclasses of FONcBaseType will return the specific type of data for
 * simple types
 */
nc_type FONcBaseType::type()
{
    return NC_NAT; // the constant ncdf uses to define simple type
}

/** @brief Clears the list of embedded variable names
 */
void FONcBaseType::clear_embedded()
{
    d_embed.clear();
}

/** @brief Identifies variable with use of NetCDF4 features
 */
void FONcBaseType::setVersion(const string &version)
{
    d_ncVersion = version;

    BESDEBUG("fonc", "FONcBaseType::setVersion() - version: '" << d_ncVersion << "'" << endl);
}

/** @brief Identifies the netCDF4 data model (CLASSIC or ENHANCED)
 */
void FONcBaseType::setNC4DataModel(const string &nc4_datamodel)
{
    d_nc4_datamodel = nc4_datamodel;

    BESDEBUG("fonc", "FONcBaseType::setNC4DataModel() - data model: '" << d_nc4_datamodel << "'" << endl);
}

/** @brief Returns true if NetCDF4 features will be required
 */
bool FONcBaseType::isNetCDF4()
{
    return FONcBaseType::d_ncVersion == FONC_RETURN_AS_NETCDF4;
}

bool FONcBaseType::isNetCDF4_ENHANCED()
{
    return FONcBaseType::d_nc4_datamodel == FONC_NC4_ENHANCED;
}

void FONcBaseType::updateD4AttrType(libdap::D4Attributes *d4_attrs, nc_type t)
{
    for (auto ii = d4_attrs->attribute_begin(), ee = d4_attrs->attribute_end(); ii != ee; ++ii) {
        if ((*ii)->name() == "_FillValue") {
            BESDEBUG("fonc", "FONcBaseType - attrtype " << getD4AttrType(t) << endl);
            BESDEBUG("fonc", "FONcBaseType - attr_type " << (*ii)->type() << endl);
            D4AttributeType correct_d4_attr_type = getD4AttrType(t);
            if (correct_d4_attr_type != (*ii)->type())
                (*ii)->set_type(correct_d4_attr_type);
            break;
        }
    }


}

void FONcBaseType::updateAttrType(libdap::AttrTable &attrs, nc_type t)
{
    if (attrs.get_size()) {
        for (auto iter = attrs.attr_begin(); iter != attrs.attr_end(); iter++) {
            if (attrs.get_name(iter) == "_FillValue") {
                BESDEBUG("fonc", "FONcBaseType - attrtype " << getAttrType(t) << endl);
                BESDEBUG("fonc", "FONcBaseType - attr_type " << attrs.get_attr_type(iter) << endl);
                if (getAttrType(t) != attrs.get_attr_type(iter)) {
                    (*iter)->type = getAttrType(t);
                }
                break;
            }
        }
    }

}

// This function is only used for handling _FillValue now. But it is a general routine that can be
// used for other purposes. 
libdap::AttrType FONcBaseType::getAttrType(nc_type nct)
{
    BESDEBUG("fonc", "FONcArray getAttrType " << endl);
    libdap::AttrType atype = Attr_unknown;
    switch (nct) {

        case NC_SHORT:
            // The original code maps to Attr_byte. This is not right. Attr_byte is uint8, NC_BYTE is int8.
            // Change to 16-bit integer to be consistent with other parts for the classic model. 
            // Note; In DAP2, no 8-bit integer type. So regardless the netCDF model, this has to be
            // Attr_int16.
            atype = Attr_int16;
            break;
        case NC_INT:
            atype = Attr_int32;
            break;
        case NC_FLOAT:
            atype = Attr_float32;
            break;
        case NC_DOUBLE:
            atype = Attr_float64;
            break;
        case NC_UBYTE:
            atype = Attr_byte;
            break;
        case NC_USHORT:
            if (isNetCDF4_ENHANCED())
                atype = Attr_uint16;
            else
                atype = Attr_int32;
            break;
        case NC_UINT:
            if (isNetCDF4_ENHANCED())
                atype = Attr_uint32;
            break;
        case NC_CHAR:
        case NC_STRING:
            atype = Attr_string;
            break;
        default:;
    }
    //Note: For DAP2, NC_BYTE(8-bit integer),NC_INT64,NC_UINT64 are not supported. So they should not
    //      appear here. NC_UINT is not supported by the classic model. 
    //      So here we also treat it unknown type.
    return atype;
}

// Obtain DAP4 attribute type for both classic and enhanced model..
D4AttributeType FONcBaseType::getD4AttrType(nc_type nct)
{
    D4AttributeType atype = attr_null_c;
    switch (nct) {
        case NC_BYTE:
            // netCDF-classic also supports 8-bit signed integer
            atype = attr_int8_c;
            break;
        case NC_SHORT:
            atype = attr_int16_c;
            break;
        case NC_INT:
            atype = attr_int32_c;
            break;
        case NC_FLOAT:
            atype = attr_float32_c;
            break;
        case NC_DOUBLE:
            atype = attr_float64_c;
            break;
        case NC_UBYTE:
            atype = attr_byte_c;
            break;
        case NC_USHORT:
            if (isNetCDF4_ENHANCED())
                atype = attr_uint16_c;
            else
                atype = attr_int32_c;
            break;
        case NC_UINT:
            if (isNetCDF4_ENHANCED())
                atype = attr_uint32_c;
            break;
        case NC_INT64:
            if (isNetCDF4_ENHANCED())
                atype = attr_int64_c;
            break;
        case NC_UINT64:
            if (isNetCDF4_ENHANCED())
                atype = attr_uint64_c;
            break;
        case NC_CHAR:
        case NC_STRING:
            atype = attr_str_c;
            break;
        default:;
    }

    if(atype == attr_null_c) 
        throw BESInternalError("Cannot convert unknown netCDF attribute type", __FILE__, __LINE__);

    return atype;
}
