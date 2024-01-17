// FONcArrayStructure.h

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

#ifndef FONcArrayStructure_h_
#define FONcArrayStructure_h_ 1

#include <libdap/Structure.h>
#include "FONcBaseType.h"
#include "FONcArrayStructureField.h"

namespace libdap {
    class Structure;
    class BaseType;
    class Array;
}


/** @brief A DAP Structure with file out netcdf information included
 *
 * This class represents a DAP Structure with additional information
 * needed to write it out to a netcdf file. Includes a reference to the
 * actual DAP Structure being converted. Keeps the list of converted
 * BaseTypes as FONcBaseType instances.
 */
class FONcArrayStructure : public FONcBaseType
{
private:
    libdap::Array *			_as = nullptr;
    vector<FONcArrayStructureField *>	_vars ;
public:
    				FONcArrayStructure( libdap::BaseType *b ) ;
    virtual			~FONcArrayStructure() ;

    void convert(vector<string> embed, bool _dap4=true, bool is_dap4_group=false) override;
    void		define( int ncid ) override;
    void		write( int ncid ) override;

    string 		name() override;

    void		dump( ostream &strm ) const override;
} ;

#endif // FONcArrayStructure_h_

