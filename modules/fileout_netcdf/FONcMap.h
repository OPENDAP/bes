// FONcMap.h

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

#ifndef FONcMap_h_
#define FONcMap_h_ 1

#include <BESObj.h>

//#include "FONcArray.h"

class FONcArray;

namespace libdap {
class Array;
}

/** @brief A map of a DAP Grid with file out netcdf information included
 *
 * This class represents a map of a DAP Grid with additional information
 * needed to write it out to a netcdf file. This map can be shared
 * amongst many grids, so it includes reference counting for reference
 * by the different FONcGrid instances.
 */
class FONcMap: public BESObj {
private:
    FONcArray *_arr;
    bool _ingrid;
    std::vector<std::string> _shared_by;
    bool _defined;
    int _ref;
    FONcMap() : _arr(0), _ingrid(false), _defined(false), _ref(1) { }
public:
    FONcMap(FONcArray *a, bool ingrid = false);
    virtual ~FONcMap();

    virtual void incref() { _ref++; }
    virtual void decref();

    virtual bool compare(libdap::Array *arr);
    virtual void add_grid(const std::string &name);
    virtual void clear_embedded();
    virtual void define(int ncid);
    virtual void write(int ncid);

    virtual void dump(std::ostream &strm) const;
};

#endif // FONcMap_h_

