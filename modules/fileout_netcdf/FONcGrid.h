// FONcGrid.h

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

#ifndef FONcGrid_h_
#define FONcGrid_h_ 1

#include <Grid.h>

namespace libdap {
    class BaseType;
    class Grid;
    class Array;
}

#include "FONcBaseType.h"
#include "FONcMap.h"
#include "FONcArray.h"

/** @brief A DAP Grid with file out netcdf information included
 *
 * This class represents a DAP Grid with additional information
 * needed to write it out to a netcdf file and includes a reference to the
 * actual DAP Grid being converted, the maps of the grid stored as
 * FONcMap instances, and the array of the grid stored as a FONcArray.
 *
 * NetCDF does not have a grid representation, per se. For this reason, we flatten
 * the DAP grid into different arrays (maps) as well as the grid's actual
 * array.
 *
 * It is possible to share maps among grids, so a global map list is
 * kept as well.
 */
class FONcGrid: public FONcBaseType {
private:
    libdap::Grid * _grid;
    FONcArray * _arr;
    vector<FONcMap *> _maps;

public:
    explicit FONcGrid(libdap::BaseType *b);
    virtual ~FONcGrid();

    virtual void convert(vector<string> embed, bool _dap4, bool is_dap4_group) override;
    virtual void define(int ncid) override;
    virtual void write(int ncid) override;

    virtual string name();

    virtual void dump(ostream &strm) const override;

    // TODO This should be moved to the FONcUtils code. jhrg 6/4/21
    static vector<FONcMap *> Maps;
    static FONcMap * InMaps(libdap::Array *array);
    static bool InGrid;
};

#endif // FONcGrid_h_

