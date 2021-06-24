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

#include <AttrTable.h>
#include <D4Attributes.h>
#include <D4AttributeType.h>
#include <BESObj.h>

#define RETURN_AS_NETCDF "netcdf"
#define RETURN_AS_NETCDF4 "netcdf-4"
#define NC4_CLASSIC_MODEL "NC4_CLASSIC_MODEL"
#define NC4_ENHANCED "NC4_ENHANCED"
// May add netCDF-3 CDF-5 in the future.

namespace libdap {
class BaseType;
class DDS;
class ConstraintEvaluator;
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
    std::string _nc4_datamodel;
    bool is_dap4;

    //This is to handle the name clashing of dimension names of string type
    bool is_dap4_group;

    libdap::DDS *d_dds;
    libdap::ConstraintEvaluator *d_eval;

    FONcBaseType() : _varid(0), _defined(false), is_dap4(false), is_dap4_group(false), d_dds(nullptr), d_eval(nullptr) { }

public:
    virtual ~FONcBaseType() = default; // { }

    libdap::DDS *get_dds() const {return d_dds;}
    void set_dds(libdap::DDS *dds) {d_dds = dds;}

    libdap::ConstraintEvaluator *get_eval() const {return d_eval;}
    void set_eval(libdap::ConstraintEvaluator *eval) {d_eval = eval;}

    virtual void convert(std::vector<std::string> embed, bool is_dap4= false, bool is_dap4_group=false);
    virtual void define(int ncid);
    virtual void write(int ncid) = 0;

    virtual std::string name() = 0;
    virtual nc_type type();
    virtual void clear_embedded();
    virtual int varid() const { return _varid; }

    virtual void dump(std::ostream &strm) const = 0;

    virtual void setVersion(const std::string &version);
    virtual void setNC4DataModel(const string &nc4_datamodel);
    virtual bool isNetCDF4();
    virtual bool isNetCDF4_ENHANCED();
    virtual void set_is_dap4(bool set_dap4) {is_dap4 = set_dap4;}
    virtual libdap::AttrType getAttrType(nc_type t);
    virtual D4AttributeType getD4AttrType(nc_type t);
    virtual void updateD4AttrType(libdap::D4Attributes *d4_attrs, nc_type t);
    virtual void updateAttrType(libdap::AttrTable&  attrs, nc_type t);
};

#endif // FONcBaseType_h_

