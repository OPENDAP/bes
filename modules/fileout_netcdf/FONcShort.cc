// FONcShort.cc

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
#include <Int16.h>
#include <UInt16.h>

#include "FONcShort.h"
#include "FONcUtils.h"
#include "FONcAttributes.h"

/** @brief Constructor for FOncShort that takes a DAP Int16 or UInt16
 *
 * This constructor takes a DAP BaseType and makes sure that it is a DAP
 * Int16 or UInt16 instance. If not, it throws an exception
 *
 * @param b A DAP BaseType that should be a byte
 * @throws BESInternalError if the BaseType is not a Int16 or UInt16
 */
FONcShort::FONcShort( BaseType *b )
    : FONcBaseType(), _bt( b )
{
    Int16 *i16 = dynamic_cast<Int16 *>(b) ;
    UInt16 *u16 = dynamic_cast<UInt16 *>(b) ;
    if( !i16 && !u16 )
    {
	string s = (string)"File out netcdf, FONcShort was passed a "
		   + "variable that is not a DAP Int16 or UInt16" ;
	throw BESInternalError( s, __FILE__, __LINE__ ) ;
    }
}

/** @brief Destructor that cleans up the short
 *
 * The DAP Int16 or UInt16 instance does not belong to the FONcByte
 * instance, so it is not deleted.
 */
FONcShort::~FONcShort()
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
FONcShort::define( int ncid )
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

/** @brief Write the short out to the netcdf file
 *
 * Once the short is defined, the value of the short can be written out
 * as well using nc_put_var1_short
 *
 * @param ncid The id of the netcdf file
 * @throws BESInternalError if there is a problem writing the value out
 * to the netcdf file
 */
void
FONcShort::write( int ncid )
{
    BESDEBUG( "fonc", "FONcShort::write for var " << _varname << endl ) ;
    size_t var_index[] = {0} ;
    short *data = new short ;
    _bt->buf2val( (void**)&data ) ;
    int stax = nc_put_var1_short( ncid, _varid, var_index, data ) ;
    if( stax != NC_NOERR )
    {
	string err = (string)"fileout.netcdf - "
		     + "Failed to write short data for "
		     + _varname ;
	FONcUtils::handle_error( stax, err, __FILE__, __LINE__ ) ;
    }
    delete data ;
    BESDEBUG( "fonc", "FONcShort::done write for var " << _varname << endl ) ;
}

/** @brief returns the name of the DAP Int16 or UInt16
 *
 * @returns The name of the DAP Int16 or UInt16
 */
string
FONcShort::name()
{
    return _bt->name() ;
}

/** @brief returns the netcdf type of the DAP object
 *
 * @returns The nc_type of NC_SHORT
 */
nc_type
FONcShort::type()
{
    return NC_SHORT ;
}

/** @brief dumps information about this object for debugging purposes
 *
 * Displays the pointer value of this instance plus instance data
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
FONcShort::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "FONcShort::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "name = " << _bt->name()  << endl ;
    BESIndent::UnIndent() ;
}

