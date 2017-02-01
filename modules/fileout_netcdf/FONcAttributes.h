// FONcAttributes.h

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

#ifndef FONcAttributes_h_
#define FONcAttributes_h_ 1

#include <netcdf.h>

#include <string>
using std::string ;

#include <BaseType.h>
using namespace libdap ;

class FONcBaseType ;

/** @brief A class that provides static methods to help write out
 * attributes for a given variable
 *
 * Given a BaseType from a DataDDS, these functions can write out
 * attributes for that BaseType as well as all parent classes of that
 * BaseType. Since netcdf is a flattened data structure, any variables
 * within a structure or grid will write out attributes for the
 * structure or grid along with its own attributes.
 */
class FONcAttributes
{
private:
    static void	add_variable_attributes_worker( int ncid, int varid, BaseType *b, string &emb_name ) ;
    static void	add_attributes_worker( int ncid, int varid, const string &var_name, AttrTable &attrs, AttrTable::Attr_iter &attr, const string &prepend_attr ) ;
public:
    static void add_attributes( int ncid, int varid, AttrTable &attrs, const string &var_name, const string &prepend_attr ) ;
    static void add_variable_attributes( int ncid, int varid, BaseType *b ) ;
    static void add_original_name( int ncid, int varid, const string &var_name, const string &orig ) ;
} ;

#endif // FONcAttributes

