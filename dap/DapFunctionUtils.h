// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES, a component 
// of the Hyrax Data Server

// Copyright (c) 2016 OPeNDAP, Inc.
// Authors: Nathan Potter <ndp@opendap.org>
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


#ifndef DapFunctionUtils_h_
#define DapFunctionUtils_h_ 1

#include <string>
using std::string;

#include <DDS.h>

/** @brief Utilities used to help in the return of an OPeNDAP DataDDS
 * object as a netcdf file
 *
 * This class includes static functions to help with the conversion of
 * an OPeNDAP DataDDS object into a netcdf file.
 */
libdap::DDS *promote_function_output_structures(libdap::DDS *fdds);

#endif // DapFunctionUtils_h_

