// FONcInt.cc

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

#include <BESInternalError.h>
#include <BESDebug.h>
#include <Int32.h>
#include <UInt32.h>

#include "FONcInt.h"
#include "FONcUtils.h"
#include "FONcAttributes.h"

/** @brief Constructor for FOncInt that takes a DAP Int32 or UInt32
 *
 * This constructor takes a DAP BaseType and makes sure that it is a DAP
 * Int32 or UInt32 instance. If not, it throws an exception
 *
 * @param b A DAP BaseType that should be an int32 or uint32
 * @throws BESInternalError if the BaseType is not an Int32 or UInt32
 */
FONcInt::FONcInt( BaseType *b )
    : FONcBaseType(), _bt( b )
{
    Int32 *i32 = dynamic_cast<Int32 *>(b) ;
    UInt32 *u32 = dynamic_cast<UInt32 *>(b) ;
    if( !i32 && !u32 )
    {
	string s = (string)"File out netcdf, FONcInt was passed a "
		   + "variable that is not a DAP Int32 or UInt32" ;
	throw BESInternalError( s, __FILE__, __LINE__ ) ;
    }
}

/** @brief Destructor that cleans up the instance
 *
 * The DAP Int32 or UInt32 instance does not belong to the FONcByte
 * instance, so it is not deleted.
 */
FONcInt::~FONcInt()
{
}

/** @brief define the DAP Int32 or UInt32 in the netcdf file
 *
 * The definition actually takes place in FONcBaseType. This function
 * adds the attributes for the instance as well as an attribute if
 * the name of the variable had to be modified.
 *
 * @param ncid The id of the NetCDF file
 * @throws BESInternalError if there is a problem defining the
 * Int32 or UInt32
 */
void
FONcInt::define( int ncid )
{
    FONcBaseType::define( ncid ) ;

    if( !_defined )
    {
	FONcAttributes::add_variable_attributes( ncid, _varid, _bt ) ;
	FONcAttributes::add_original_name( ncid, _varid,
					   _varname, _orig_varname ) ;

	_defined = true ;
    }
}

/** @brief Write the int out to the netcdf file
 *
 * Once the int is defined, the value of the int can be written out
 *
 * @param ncid The id of the netcdf file
 * @throws BESInternalError if there is a problem writing the value
 */
void
FONcInt::write( int ncid )
{
    BESDEBUG( "fonc", "FONcInt::write for var " << _varname << endl ) ;
    size_t var_index[] = {0} ;
    int *data = new int ;
    _bt->buf2val( (void**)&data ) ;
    int stax = nc_put_var1_int( ncid, _varid, var_index, data ) ;
    if( stax != NC_NOERR )
    {
	string err = (string)"fileout.netcdf - "
		     + "Failed to write int data for "
		     + _varname ;
	FONcUtils::handle_error( stax, err, __FILE__, __LINE__ ) ;
    }
    delete data ;
    BESDEBUG( "fonc", "FONcInt::done write for var " << _varname << endl ) ;
}

/** @brief returns the name of the DAP Int32 or UInt32
 *
 * @returns The name of the DAP Int32 or UInt32
 */
string
FONcInt::name()
{
    return _bt->name() ;
}

/** @brief returns the netcdf type of the DAP object
 *
 * @returns The nc_type of NC_INT
 */
nc_type
FONcInt::type()
{
    return NC_INT ;
}

/** @brief dumps information about this object for debugging purposes
 *
 * Displays the pointer value of this instance plus instance data
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
FONcInt::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "FONcInt::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "name = " << _bt->name()  << endl ;
    BESIndent::UnIndent() ;
}

