// FONcUShort.h

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
// Note: The code follows FONcShort.h.

#ifndef FONcUShort_h_
#define FONcUShort_h_ 1

#include <BaseType.h>

namespace libdap {
    class BaseType;
}

#include "FONcBaseType.h"

/** @brief A DAP UInt16 with file out netcdf information included
 *
 * This class represents a DAP UInt16 with additional information
 * needed to write it out to a netcdf file. Includes a reference to the
 * actual DAP Int16 or UInt16 being converted
 */
class FONcUShort : public FONcBaseType
{
private:
    libdap::BaseType *			_bt ;
public:
    				FONcUShort( libdap::BaseType *b ) ;
    virtual			~FONcUShort() ;

    virtual void		define( int ncid ) ;
    virtual void		write( int ncid ) ;

    virtual string 		name() ;
    virtual nc_type		type() ;

    virtual void		dump( ostream &strm ) const ;
} ;

#endif // FONcUShort_h_

