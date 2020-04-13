
// FONcUByte.h

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
// Note: The code follows FONcUByte.h.


#ifndef FONcUByte_h_
#define FONcUByte_h_ 1

#include <Byte.h>

using namespace libdap ;

#include "FONcBaseType.h"

/** @brief A class representing the DAP Byte class for file out netcdf
 *
 * This class represents a DAP Byte with additional information
 * needed to write it out to a netcdf file. Includes a reference to the
 * actual DAP Byte being converted
 */
class FONcUByte : public FONcBaseType
{
private:
    Byte *			_b ;
public:
    				FONcUByte( BaseType *b ) ;
    virtual			~FONcUByte() ;

    virtual void		define( int ncid ) ;
    virtual void		write( int ncid ) ;

    virtual string 		name() ;
    virtual nc_type		type() ;

    virtual void		dump( ostream &strm ) const ;
} ;

#endif // FONcUByte_h_

