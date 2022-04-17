// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2006 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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

#define YYDEBUG 1
#undef YYERROR_VERBOSE
#define YY_NO_UNPUT 1

#define ID_MAX 256

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

namespace libdap {
class Grid;
}

namespace functions
{

class GSEClause;

/** Argument to the GSE parser. Assumes all errors will be signaled by
    throws; Error objects are not returned. */
struct gse_arg
{
    GSEClause *_gsec;           // The gse parsed.
    libdap::Grid *_grid;                // The Grid being constrained.
    int _status;                // The parser's status.

    gse_arg(): _gsec(0), _grid(0), _status(1)
    {}
    gse_arg(libdap::Grid *g): _gsec(0), _grid(g), _status(1)
    {}
    virtual ~gse_arg()
    {}

    void set_gsec(GSEClause *gsec)
    {
        _gsec = gsec;
    }
    GSEClause *get_gsec()
    {
        return _gsec;
    }
    void set_grid(libdap::Grid *g)
    {
        _grid = g;
    }
    libdap::Grid *get_grid()
    {
        return _grid;
    }
    void set_status(int stat)
    {
        _status = stat;
    }
    int get_status()
    {
        return _status;
    }
};

} // namespace libdap
