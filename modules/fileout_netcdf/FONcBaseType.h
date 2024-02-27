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

#include <libdap/AttrTable.h>
#include <libdap/D4Attributes.h>
#include <libdap/D4AttributeType.h>
#include <BESObj.h>
#include "FONcNames.h"


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
    int d_varid = 0;
    std::string d_varname;
    std::string d_orig_varname;
    std::vector<std::string> d_embed;
    bool d_defined = false;
    std::string d_ncVersion;
    std::string d_nc4_datamodel;
    // This is set by transform_dap4() in FONCtranform.cc. Look for the call to convert()
    // and/or the comment lines that mention d_is_dap with the date 2/14/24. jhrg 2/14/24
    bool d_is_dap4 = false;

    //This is to handle the name clashing of dimension names of string type
    bool d_is_dap4_group = false;

    libdap::DDS *d_dds = nullptr;
    libdap::ConstraintEvaluator *d_eval = nullptr;

    // direct io flag, used in the define mode,the default is false. It should be set to true when direct io is supported.
    // TODO: This is for the temporary memory usage optimization. Once we can support the define() with or without dio for individual array.
    //       This flag is not necessary and should be removed. KY 11/29/23
    bool fdio_flag = false;


public:
    FONcBaseType() = default;
    ~FONcBaseType() override = default;

    libdap::DDS *get_dds() const {return d_dds;}
    void set_dds(libdap::DDS *dds) {d_dds = dds;}

    libdap::ConstraintEvaluator *get_eval() const {return d_eval;}
    void set_eval(libdap::ConstraintEvaluator *eval) {d_eval = eval;}

    // I made this change to see how hard it would be to refactor a virtual
    // method that used parameters with default (prohibited) values. jhrg 10/3/22
    void convert(std::vector<std::string> embed) {
        convert(embed, false, false);
    }
    void convert(std::vector<std::string> embed, bool is_dap4) {
        convert(embed, is_dap4, false);
    }
    virtual void convert(std::vector<std::string> embed, bool is_dap4, bool is_dap4_group);

    virtual void define(int ncid);

    virtual void write(int ncid) = 0;

    virtual std::string name() = 0;

    virtual nc_type type();
    virtual void clear_embedded();
    virtual int varid() const { return d_varid; }

    void dump(std::ostream &strm) const override = 0;

    virtual void setVersion(const std::string &version);
    virtual void setNC4DataModel(const string &nc4_datamodel);
    virtual bool isNetCDF4();
    bool isNetCDF4_ENHANCED();
    virtual void set_is_dap4(bool set_dap4) { d_is_dap4 = set_dap4;}
    virtual libdap::AttrType getAttrType(nc_type t);
    virtual D4AttributeType getD4AttrType(nc_type t);
    virtual void updateD4AttrType(libdap::D4Attributes *d4_attrs, nc_type t);
    virtual void updateAttrType(libdap::AttrTable&  attrs, nc_type t);

    // TODO: This is for the temporary memory usage optimization. Once we can support the define() with or without dio for individual array.
    //       The following methods are  not necessary and should be removed. KY 11/29/23
    bool get_fdio_flag() const {return fdio_flag; }
    void set_fdio_flag(bool dio_flag_value = true) { fdio_flag = dio_flag_value; }

};

#endif // FONcBaseType_h_

