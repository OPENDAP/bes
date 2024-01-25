// FONcDim.cc

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

using std::ostringstream;

#include <netcdf.h>
#include <BESDebug.h>

#include "FONcDim.h"
#include "FONcUtils.h"

int FONcDim::DimNameNum = 0;
int FONcDim::StructDimNameNum = 0;
/** @brief Constructor for FOncDim that defines the dimension of an
 * array
 *
 * @param name The name of the dimension
 * @param size The size of the dimension
 */
FONcDim::FONcDim(const string &name, int64_t size) :
    _name(name), _size(size)
{
}

/** @brief Decrement the reference count for this dimension
 *
 * Since dimensions can be shared, there might be more than one array
 * pointing to it. For this reason, we use reference counting for the
 * instance. if the reference count gets to zero, then the instance is
 * deleted
 */
void FONcDim::decref()
{
    _ref--;
    if (!_ref) delete this;
}

void FONcDim::struct_decref()
{
    _struct_ref--;
    if (!_struct_ref) delete this;
}
/** @brief define the DAP dimension in the netcdf file
 *
 * If the dimension has not already been created by an array that shares
 * this dimension, then define the dimension in the netcdf file.
 *
 * If the dimension name is empty, the create a default one using an
 * incremented counter.
 *
 * @param ncid The id of the NetCDF file
 * @throws BESInternalError if there is a problem defining the
 * dimension
 */
void FONcDim::define(int ncid)
{
    if (!_defined) {
        if (_name.empty()) {
            ostringstream dimname_strm;
            dimname_strm << "dim" << FONcDim::DimNameNum + 1;
            FONcDim::DimNameNum++;
            _name = dimname_strm.str();
        }
        else {
            _name = FONcUtils::id2netcdf(_name);
        }

        BESDEBUG("fonc",  "FONcDim:: dimension size is "<<_size <<endl);
        BESDEBUG("fonc",  "FONcDim:: dimension name is "<<_name <<endl);
        int stax = nc_def_dim(ncid, _name.c_str(), _size, &_dimid);
        if (stax != NC_NOERR) {
            string err = (string) "fileout.netcdf - " + "Failed to add dimension " + _name;
            FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
        }
        _defined = true;
    }
}

void FONcDim::define_struct(int ncid)
{
    if (!_defined) {
        if (_name.empty()) {
            FONcDim::StructDimNameNum++;
            _name ="sdim" + to_string(FONcDim::StructDimNameNum);
        }
        else {
            _name = FONcUtils::id2netcdf(_name);
        }

        BESDEBUG("fonc",  "FONcDim:: dimension size is "<<_size <<endl);
        BESDEBUG("fonc",  "FONcDim:: dimension name is "<<_name <<endl);
        int stax = nc_def_dim(ncid, _name.c_str(), _size, &_dimid);
        if (stax != NC_NOERR) {
            string err = (string) "fileout.netcdf - " + "Failed to add dimension " + _name;
            FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
        }
        _defined = true;
    }
}

/** @brief dumps information about this object for debugging purposes
 *
 * Displays the pointer value of this instance plus instance data
 *
 * @param strm C++ i/o stream to dump the information to
 */
void FONcDim::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "FONcDim::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "name = " << _name << endl;
    strm << BESIndent::LMarg << "size = " << _size << endl;
    strm << BESIndent::LMarg << "dimid = " << _dimid << endl;
    strm << BESIndent::LMarg << "already defined? ";
    if (_defined)
        strm << "true";
    else
        strm << "false";
    strm << endl;
    BESIndent::UnIndent();
}

