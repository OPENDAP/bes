// FONcStr.cc

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

#include <sstream>
#include <libdap/Str.h>

#include <BESInternalError.h>
#include <BESDebug.h>

#include "FONcStr.h"
#include "FONcDim.h"
#include "FONcUtils.h"
#include "FONcAttributes.h"

using namespace libdap;

/** @brief Constructor for FONcStr that takes a DAP Str
 *
 * This constructor takes a DAP BaseType and makes sure that it is a DAP
 * Str instance. If not, it throws an exception
 *
 * @param b A DAP BaseType that should be a Str
 * @throws BESInternalError if the BaseType is not a Str
 */
FONcStr::FONcStr(BaseType *b) :
    FONcBaseType(), _str(0), _dimid(0), _data(0)
{
    _str = dynamic_cast<Str *>(b);
    if (!_str)
        throw BESInternalError("File out netcdf, FONcStr was passed a variable that is not a DAP Str", __FILE__, __LINE__);
}

/** @brief Destructor that cleans up the str
 *
 * The DAP Str instance does not belong to the FONcByte instance, so it
 * is not cleaned up. During definition, though, the data needed to be
 * extracted to calculate the length dimension. It is kept so that we
 * don't have to get the data twice. The string data is deleted here.
 */
FONcStr::~FONcStr()
{
    delete _data;
}

/** @brief Define the string variable in the netcdf file
 *
 * This method creates this string variable in the netcdf file. To do
 * this we define a dimension that specifies the length of the string.
 *
 * @param ncid Id of the NetCDF file
 * @throws BESInternalError if defining the variable fails
 */
void FONcStr::define(int ncid)
{
    if (!d_defined) {
        BESDEBUG("fonc", "FONcStr::define - defining " << d_varname << endl);

        d_varname = FONcUtils::gen_name(d_embed, d_varname, d_orig_varname);
        _data = new string;

        // FIXME This is a hack to get around the fact that the code moved away from setting the DDS and
        //  the ConstraintEvaluator in the constructor. This is a temporary fix. jhrg 2/14/24
        if (d_is_dap4|| get_eval() == nullptr || get_dds() == nullptr)
            _str->intern_data();
        else
            _str->intern_data(*get_eval(), *get_dds());

        _str->buf2val((void**) &_data);
        auto size = _data->size() + 1;

        string dimname;

        // For DAP4, we need to ensure the dimension name is unique.
        if(d_is_dap4_group == true) {
            ostringstream dim_suffix_strm;
            dim_suffix_strm <<"_len"<<FONcDim::DimNameNum +1;
            FONcDim::DimNameNum++;
            dimname = d_varname + dim_suffix_strm.str();
        }
        else 
            dimname = d_varname + "_len";
        int stax = nc_def_dim(ncid, dimname.c_str(), size, &_dimid);
        if (stax != NC_NOERR) {
            string err = (string) "fileout.netcdf - " + "Failed to define dim " + dimname + " for " + d_varname;
            FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
        }

        int var_dims[1];        // variable shape
        var_dims[0] = _dimid;
        stax = nc_def_var(ncid, d_varname.c_str(), NC_CHAR, 1, var_dims, &d_varid);
        if (stax != NC_NOERR) {
            string err = (string) "fileout.netcdf - " + "Failed to define var " + d_varname;
            FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
        }

        d_defined = true;

        FONcAttributes::add_variable_attributes(ncid, d_varid, _str, isNetCDF4_ENHANCED(), d_is_dap4);
        FONcAttributes::add_original_name(ncid, d_varid, d_varname, d_orig_varname);

        BESDEBUG("fonc", "FONcStr::define - done defining " << d_varname << endl);
    }
}

/** @brief Write the str out to the netcdf file
 *
 * Once the str is defined, the value of the str can be written out
 * as well using nc_put_vara_text
 *
 * @param ncid The id of the netcdf file
 * @throws BESInternalError if there is a problem writing the value out
 * to the netcdf file
 */
void FONcStr::write(int ncid)
{
    BESDEBUG("fonc", "FONcStr::write for var " << d_varname << endl);

    size_t var_start[1];	// variable start
    size_t var_count[1];	// variable count

    var_count[0] = _data->size() + 1;
    var_start[0] = 0;
    int stax = nc_put_vara_text(ncid, d_varid, var_start, var_count, _data->c_str());
    if (stax != NC_NOERR) {
        string err = (string) "fileout.netcdf - " + "Failed to write string data " + *_data + " for " + d_varname;
        delete _data;
        _data = 0;
        FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
    }
    delete _data;
    _data = 0;

    BESDEBUG("fonc", "FONcStr::done write for var " << d_varname << endl);
}

/** @brief returns the name of the DAP Str
 *
 * @returns The name of the DAP Str
 */
string FONcStr::name()
{
    return _str->name();
}

/** @brief returns the netcdf type of the DAP Str
 *
 * @returns The nc_type of NC_CHAR
 */
nc_type FONcStr::type()
{
    return NC_CHAR;
}

/** @brief dumps information about this object for debugging purposes
 *
 * Displays the pointer value of this instance plus instance data
 *
 * @param strm C++ i/o stream to dump the information to
 */
void FONcStr::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "FONcStr::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "name = " << _str->name() << endl;
    BESIndent::UnIndent();
}

