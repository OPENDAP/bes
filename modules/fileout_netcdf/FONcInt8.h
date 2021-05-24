
// FONcInt8.h

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
//
// Authors:
//      kyang     Kent Yang  <myang6@hdfgroup.org>
// Note: The code follows FONcByte.h.


#ifndef FONcInt8_h_
#define FONcInt8_h_ 1

#include <Int8.h>

namespace libdap {
    class BaseType;
}

#include "FONcBaseType.h"

/** @brief A class representing the DAP4 int8 class for file out netcdf
 *
 * This class represents a DAP4 int8 with additional information
 * needed to write it out to a netcdf file. Includes a reference to the
 * actual DAP4 int8 being converted
 */
class FONcInt8 : public FONcBaseType
{
private:
    libdap::Int8 *			_b ;
public:
    				FONcInt8( libdap::BaseType *b ) ;
    virtual			~FONcInt8() ;

    virtual void		define( int ncid ) ;
    virtual void		write( int ncid ) ;

    virtual string 		name() ;
    virtual nc_type		type() ;

    virtual void		dump( ostream &strm ) const ;
} ;

#endif // FONcInt8_h_

