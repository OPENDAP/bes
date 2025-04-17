// NCMLContainerStorage.h

// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of wcs_module, A C++ module that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle wcs requests.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
// Author: Patrick West <pwest@ucar.edu>
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

// (c) COPYRIGHT URI/MIT 1994-1999
// Please read the full copyright statement in the file COPYRIGHT_URI.
//
// Authors:
//      pcw       Patrick West <pwest@ucar.edu>

#ifndef NCMLContainerStorage_h_
#define NCMLContainerStorage_h_ 1

#include <list>

#include "BESContainerStorageVolatile.h"

class BESCatalogUtils;

/** @brief implementation of BESContainerStorageVolatile that represents a
 * list of NCML requests
 *
 * Each of the containers stored in the NCMLContainerStorage represents a NCML
 * request. When accessed the container will make the NCML request in order
 * to create the target response.
 *
 * @see BESContainerStorageVolatile
 * @see NCMLContainer
 */
class NCMLContainerStorage: public BESContainerStorageVolatile {
public:
    static string NCML_TempDir;

    NCMLContainerStorage(const string &n);
    virtual ~NCMLContainerStorage();

    virtual void add_container(const string &s_name, const string &r_name, const string &type);

    virtual void dump(ostream &strm) const;
};

#endif // NCMLContainerStorage_h_

