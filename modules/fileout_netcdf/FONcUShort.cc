// FONcUShort.cc

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
// Note: The code follows FONcShort.cc.


#include <BESInternalError.h>
#include <BESDebug.h>
#include <UInt16.h>

#include "FONcUShort.h"
#include "FONcUtils.h"
#include "FONcAttributes.h"

/** @brief Constructor for FOncShort that takes a DAP  UInt16
 *
 * This constructor takes a DAP BaseType and makes sure that it is a DAP
 * UInt16 instance. If not, it throws an exception
 *
 * @param b A DAP BaseType that should be a byte
 * @throws BESInternalError if the BaseType is not a UInt16
 */
FONcUShort::FONcUShort( BaseType *b )
    : FONcBaseType(), _bt( b )
{
    UInt16 *u16 = dynamic_cast<UInt16 *>(b) ;
    if(  !u16 )
    {
	string s = (string)"File out netcdf, FONcUShort was passed a "
		   + "variable that is not a DAP  UInt16" ;
	throw BESInternalError( s, __FILE__, __LINE__ ) ;
    }
}

/** @brief Destructor that cleans up the short
 *
 * The DAP Int16 or UInt16 instance does not belong to the FONcByte
 * instance, so it is not deleted.
 */
FONcUShort::~FONcUShort()
{
}

/** @brief define the DAP Int16 or UInt16 in the netcdf file
 *
 * The definition actually takes place in FONcBaseType. This function
 * adds the attributes for the variable instance as well as an attribute if
 * the name had to be modified.
 *
 * @param ncid The id of the NetCDF file
 * @throws BESInternalError if there is a problem defining the
 * Byte
 */
void
FONcUShort::define( int ncid )
{
    FONcBaseType::define( ncid ) ;

    if( !_defined )
    {
	FONcAttributes::add_variable_attributes( ncid, _varid, _bt,isNetCDF4_ENHANCED() ) ;
	FONcAttributes::add_original_name( ncid, _varid,
					   _varname, _orig_varname ) ;

	_defined = true ;
    }
}

/** @brief Write the ushort out to the netcdf file
 *
 * Once the ushort is defined, the value of the ushort can be written out
 * as well using nc_put_var1_ushort
 *
 * @param ncid The id of the netcdf file
 * @throws BESInternalError if there is a problem writing the value out
 * to the netcdf file
 */
void
FONcUShort::write( int ncid )
{
    BESDEBUG( "fonc", "FONcUShort::write for var " << _varname << endl ) ;
    size_t var_index[] = {0} ;
    unsigned short *data = new unsigned short ;
    _bt->buf2val( (void**)&data ) ;
    int stax = nc_put_var1_ushort( ncid, _varid, var_index, data ) ;
    if( stax != NC_NOERR )
    {
	string err = (string)"fileout.netcdf - "
		     + "Failed to write short data for "
		     + _varname ;
	FONcUtils::handle_error( stax, err, __FILE__, __LINE__ ) ;
    }
    delete data ;
    BESDEBUG( "fonc", "FONcUShort::done write for var " << _varname << endl ) ;
}

/** @brief returns the name of the DAP Int16 or UInt16
 *
 * @returns The name of the DAP Int16 or UInt16
 */
string
FONcUShort::name()
{
    return _bt->name() ;
}

/** @brief returns the netcdf type of the DAP object
 *
 * @returns The nc_type of NC_SHORT
 */
nc_type
FONcUShort::type()
{
    return NC_USHORT ;
}

/** @brief dumps information about this object for debugging purposes
 *
 * Displays the pointer value of this instance plus instance data
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
FONcUShort::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "FONcUShort::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "name = " << _bt->name()  << endl ;
    BESIndent::UnIndent() ;
}

