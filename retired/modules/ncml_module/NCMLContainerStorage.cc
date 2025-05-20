// NCMLContainerStorage.cc

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

#include "NCMLContainerStorage.h"
#include "NCMLContainer.h"

string NCMLContainerStorage::NCML_TempDir;

/** @brief create an instance of this persistent store with the given name.
 *
 * Creates an instances of NCMLContainerStorage with the given name.
 *
 * @param n name of this persistent store
 * @see NCMLContainer
 */
NCMLContainerStorage::NCMLContainerStorage(const string &n) :
    BESContainerStorageVolatile(n)
{
}

NCMLContainerStorage::~NCMLContainerStorage()
{
}

/** @brief adds a container with the provided information
 *
 * @param s_name symbolic name for the container
 * @param r_name the wcs request url
 * @param type ignored. The type of the target response is determined by the
 * NCML request with the format parameter
 */
void NCMLContainerStorage::add_container(const string &s_name, const string &r_name, const string &/*type*/)
{
    BESContainer *c = new NCMLContainer(s_name, r_name);
    BESContainerStorageVolatile::add_container(c);
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * each of the NCMLcontainers already stored.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void NCMLContainerStorage::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "NCMLContainerStorage::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESContainerStorageVolatile::dump(strm);
    BESIndent::UnIndent();
}

