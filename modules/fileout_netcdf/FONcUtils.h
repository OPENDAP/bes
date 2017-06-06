// FONcUtils.h

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

#ifndef FONcUtils_h_
#define FONcUtils_h_ 1

#include <netcdf.h>

#include <string>
using std::string;

#include <BaseType.h>
using namespace libdap;

class FONcBaseType;

#define FONC_EMBEDDED_SEPARATOR "."
#define FONC_ATTRIBUTE_SEPARATOR "."
#define FONC_ORIGINAL_NAME "fonc_original_name"

/** @brief Utilities used to help in the return of an OPeNDAP DataDDS
 * object as a netcdf file
 *
 * This class includes static functions to help with the conversion of
 * an OPeNDAP DataDDS object into a netcdf file.
 */
class FONcUtils {
public:
    static string name_prefix;
    static void reset();
    static string id2netcdf(string in);
    static nc_type get_nc_type(BaseType *element);
    static string gen_name(const vector<string> &embed, const string &name, string &original);
    static FONcBaseType * convert(BaseType *v);
    static void handle_error(int stax, const string &err, const string &file, int line);
};

#endif // FONcUtils

