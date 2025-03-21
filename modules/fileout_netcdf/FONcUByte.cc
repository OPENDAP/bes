// FONcUByte.cc

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
// Note: The code follows FONcUByte.cc.



#include <BESInternalError.h>
#include <BESDebug.h>

#include "FONcUByte.h"
#include "FONcUtils.h"
#include "FONcAttributes.h"

/** @brief Constructor for FONcUByte that takes a DAP Byte
 *
 * This constructor takes a DAP BaseType and makes sure that it is a DAP
 * Byte instance. If not, it throws an exception
 *
 * @param b A DAP BaseType that should be a byte
 * @throws BESInternalError if the BaseType is not a Byte
 */
FONcUByte::FONcUByte( BaseType *b )
    : FONcBaseType(), _b( 0 )
{
    _b = dynamic_cast<Byte *>(b) ;
    if( !_b )
    {
	string s = (string)"File out netcdf, FONcUByte was passed a "
		   + "variable that is not a DAP Byte" ;
	throw BESInternalError( s, __FILE__, __LINE__ ) ;
    }
}

/** @brief Destructor that cleans up the byte
 *
 * The DAP Byte instance does not belong to the FONcUByte instance, so it
 * is not deleted.
 */
FONcUByte::~FONcUByte()
{
}

/** @brief define the DAP Byte in the netcdf file
 *
 * The definition actually takes place in FONcBaseType. This function
 * adds the attributes for the Byte instance as well as an attribute if
 * the name of the Byte had to be modified.
 *
 * @param ncid The id of the NetCDF file
 * @throws BESInternalError if there is a problem defining the
 * Byte
 */
void
FONcUByte::define( int ncid )
{
    FONcBaseType::define( ncid ) ;

    if( !d_defined )
    {
        if(d_is_dap4) {
            D4Attributes *d4_attrs = _b->attributes();                                                     
            updateD4AttrType(d4_attrs,NC_UBYTE);   
        }
        else {
            AttrTable &attrs = _b->get_attr_table();  
            updateAttrType(attrs,NC_UBYTE); 
        }

	FONcAttributes::add_variable_attributes(ncid, d_varid, _b, isNetCDF4_ENHANCED(), d_is_dap4 ) ;
	FONcAttributes::add_original_name(ncid, d_varid,
                                      d_varname, d_orig_varname ) ;

        d_defined = true ;
    }
}

/** @brief Write the byte out to the netcdf file
 *
 * Once the byte is defined, the value of the byte can be written out
 * as well.
 *
 * @param ncid The id of the netcdf file
 * @throws BESInternalError if there is a problem writing the value out
 * to the netcdf file
 */
void
FONcUByte::write( int ncid )
{
    BESDEBUG( "fonc", "FOncUByte::write for var " << d_varname << endl ) ;
    size_t var_index[] = {0} ;
    unsigned char *data = new unsigned char ;

    if (d_is_dap4)
        _b->intern_data();
    else
        _b->intern_data(*get_eval(), *get_dds());

    _b->buf2val( (void**)&data ) ;
    int stax = nc_put_var1_uchar(ncid, d_varid, var_index, data ) ;
    if( stax != NC_NOERR )
    {
	string err = (string)"fileout.netcdf - "
                 + "Failed to write byte data for "
                 + d_varname ;
	FONcUtils::handle_error( stax, err, __FILE__, __LINE__ ) ;
    }
    delete data ;
}

/** @brief returns the name of the DAP Byte
 *
 * @returns The name of the DAP Byte
 */
string
FONcUByte::name()
{
    return _b->name() ;
}

/** @brief returns the netcdf type of the DAP Byte
 *
 * @returns The nc_type of NC_BYTE
 */
nc_type
FONcUByte::type()
{
    return NC_UBYTE ;
}

/** @brief dumps information about this object for debugging purposes
 *
 * Displays the pointer value of this instance plus instance data
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
FONcUByte::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "FONcUByte::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "name = " << _b->name()  << endl ;
    BESIndent::UnIndent() ;
}

