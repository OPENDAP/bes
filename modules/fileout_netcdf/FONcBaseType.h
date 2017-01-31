// FONcBaseType.h

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

#ifndef FONcBaseType_h_
#define FONcBaseType_h_ 1

#include <netcdf.h>
#include <vector>
#include <string>


#include <BESObj.h>
//#include <BaseType.h>

#define RETURNAS_NETCDF "netcdf"
#define RETURNAS_NETCDF4 "netcdf-4"

namespace libdap {
class BaseType;
}

//using namespace libdap;

/** @brief A DAP BaseType with file out netcdf information included
 *
 * This class represents a DAP BaseType with additional information
 * needed to write it out to a netcdf file. Includes a reference to the
 * actual DAP BaseType being converted
 */
class FONcBaseType: public BESObj {
protected:
    int _varid;
    std::string _varname;
    std::string _orig_varname;
    std::vector<std::string> _embed;
    bool _defined;
    std::string _ncVersion;

    FONcBaseType() : _varid(0), _defined(false) { }

public:
    virtual ~FONcBaseType() { }

    virtual void convert(std::vector<std::string> embed);
    virtual void define(int ncid);
    virtual void write(int /*ncid*/) {  }

    virtual std::string name() = 0;
    virtual nc_type type();
    virtual void clear_embedded();
    virtual int varid() const { return _varid; }

    virtual void dump(std::ostream &strm) const = 0;

    virtual void setVersion(std::string version);
    virtual bool isNetCDF4();

};

#endif // FONcBaseType_h_

