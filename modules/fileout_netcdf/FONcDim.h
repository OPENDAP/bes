// FONcDim.h

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

#ifndef FONcDim_h_
#define FONcDim_h_ 1

#include <BESObj.h>

/** @brief A class that represents the dimension of an array.
 *
 * This class represents a dimension of a DAP Array with additional
 * information needed to write it out to a netcdf file. Since this
 * dimension can be shared, it includes reference counting so that it
 * can be pointed to by multiple arrays.
 */
class FONcDim : public BESObj
{
private:
    string			_name ;
    int				_size ;
    int				_dimid ;
    bool			_defined ;
    int				_ref ;
public:
    				FONcDim( const string &name, int size ) ;
    virtual			~FONcDim() {}
    virtual void		incref() { _ref++ ; }
    virtual void		decref() ;

    virtual void		define( int ncid ) ;

    virtual string		name() { return _name ; }
    virtual int			size() { return _size ; }
    virtual void		update_size( int newsize ) { _size = newsize ; }
    virtual int			dimid() { return _dimid ; }
    virtual bool		defined() { return _defined ; }

    virtual void		dump( ostream &strm ) const ;

    static int			DimNameNum ;
} ;

#endif // FONcDim_h_

