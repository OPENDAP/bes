// FONcStructure.h

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

#ifndef FONcStructure_h_
#define FONcStructure_h_ 1

#include <Structure.h>

using namespace libdap ;

#include "FONcBaseType.h"

/** @brief A DAP Structure with file out netcdf information included
 *
 * This class represents a DAP Structure with additional information
 * needed to write it out to a netcdf file. Includes a reference to the
 * actual DAP Structure being converted. Keeps the list of converted
 * BaseTypes as FONcBaseType instances.
 */
class FONcStructure : public FONcBaseType
{
private:
    Structure *			_s ;
    vector<FONcBaseType *>	_vars ;
public:
    				FONcStructure( BaseType *b ) ;
    virtual			~FONcStructure() ;

    virtual void		convert( vector<string> embed ) ;
    virtual void		define( int ncid ) ;
    virtual void		write( int ncid ) ;

    virtual string 		name() ;

    virtual void		dump( ostream &strm ) const ;
} ;

#endif // FONcStructure_h_

