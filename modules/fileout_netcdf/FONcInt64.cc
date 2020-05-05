// FONcInt64.cc

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
//      kyang     Kent Yang  <myang6@hdfgroup.org>
// Note: The code follows FONcUInt.cc.


#include <BESInternalError.h>
#include <BESDebug.h>
#include <Int64.h>

#include "FONcInt64.h"
#include "FONcUtils.h"
#include "FONcAttributes.h"

/** @brief Constructor for FOncInt64 that takes a DAP Int64
 *
 * This constructor takes a DAP BaseType and makes sure that it is a DAP
 * Int64 instance. If not, it throws an exception
 *
 * @param b A DAP BaseType that should be an uint64
 * @throws BESInternalError if the BaseType is not an Int64
 */
FONcInt64::FONcInt64( BaseType *b )
    : FONcBaseType(), _bt( b )
{
    Int64 *u64 = dynamic_cast<Int64 *>(b) ;
    if( !u64 )
    {
	string s = (string)"File out netcdf, FONcUInt was passed a "
		   + "variable that is not a DAP Int64" ;
	throw BESInternalError( s, __FILE__, __LINE__ ) ;
    }
}

/** @brief Destructor that cleans up the instance
 *
 * The DAP Int64 instance does not belong to the FONcByte
 * instance, so it is not deleted.
 */
FONcInt64::~FONcInt64()
{
}

/** @brief define the DAP Int64 in the netcdf file
 *
 * The definition actually takes place in FONcBaseType. This function
 * adds the attributes for the instance as well as an attribute if
 * the name of the variable had to be modified.
 *
 * @param ncid The id of the NetCDF file
 * @throws BESInternalError if there is a problem defining the
 * Int64
 */
void
FONcInt64::define( int ncid )
{
    FONcBaseType::define( ncid ) ;

    if( !_defined )
    {
	FONcAttributes::add_variable_attributes( ncid, _varid, _bt ,isNetCDF4_ENHANCED()) ;
	FONcAttributes::add_original_name( ncid, _varid,
					   _varname, _orig_varname ) ;

	_defined = true ;
    }
}

/** @brief Write the unsigned int out to the netcdf file
 *
 * Once the unsigned int is defined, the value of the unsigned int can be written out
 *
 * @param ncid The id of the netcdf file
 * @throws BESInternalError if there is a problem writing the value
 */
void
FONcInt64::write( int ncid )
{
    BESDEBUG( "fonc", "FONcInt64::write for var " << _varname << endl ) ;
    size_t var_index[] = {0} ;
    //int64_t *data = new int64_t ;
    long long  *data = new long long ;
    _bt->buf2val( (void**)&data ) ;
    //int stax = nc_put_var1_longlong( ncid, _varid, var_index, (const long long*)data ) ;
    int stax = nc_put_var1_longlong( ncid, _varid, var_index, data ) ;
    if( stax != NC_NOERR )
    {
	string err = (string)"fileout.netcdf - "
		     + "Failed to write unsigned int data for "
		     + _varname ;
	FONcUtils::handle_error( stax, err, __FILE__, __LINE__ ) ;
    }
    delete data ;
    BESDEBUG( "fonc", "FONcInt64::done write for var " << _varname << endl ) ;
}

/** @brief returns the name of the DAP Int64
 *
 * @returns The name of the DAP Int64
 */
string
FONcInt64::name()
{
    return _bt->name() ;
}

/** @brief returns the netcdf type of the DAP object
 *
 * @returns The nc_type of NC_UINT
 */
nc_type
FONcInt64::type()
{
    return NC_INT64 ;
}

/** @brief dumps information about this object for debugging purposes
 *
 * Displays the pointer value of this instance plus instance data
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
FONcInt64::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "FONcInt64::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "name = " << _bt->name()  << endl ;
    BESIndent::UnIndent() ;
}

