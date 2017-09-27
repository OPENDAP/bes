// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
// Author: Nathan Potter <npotter@opendap.org>
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
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#ifndef GRID_UTILS_H_
#define GRID_UTILS_H_

namespace libdap {
class DDS;
class BaseType;
class Grid;
}

namespace functions {

class GSEClause;
struct gse_arg;         // in gse_parser.h

void get_grids(libdap::BaseType *bt, std::vector<libdap::Grid *> *grids);
void get_grids(libdap::DDS &dds, std::vector<libdap::Grid *> *grids);
bool is_geo_grid(libdap::Grid *d_grid);

void parse_gse_expression(gse_arg *arg, libdap::BaseType * expr);
void apply_grid_selection_expressions(libdap::Grid * grid, std::vector <GSEClause *>clauses);

}

#endif /* GRID_UTILS_H_ */
