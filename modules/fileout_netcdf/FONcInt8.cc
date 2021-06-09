// FONcInt8.cc

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
// Note: The code follows FONcByte.cc.



#include <BESInternalError.h>
#include <BESDebug.h>

#include "FONcInt8.h"
#include "FONcUtils.h"
#include "FONcAttributes.h"

/** @brief Constructor for FONcInt8 that takes a DAP4 int8
 *
 * This constructor takes a DAP BaseType and makes sure that it is a DAP4
 * int8 instance. If not, it throws an exception
 *
 * @param b A DAP BaseType that should be a DAP4 int8
 * @throws BESInternalError if the BaseType is not a DAP4 int8
 */
FONcInt8::FONcInt8( BaseType *b )
    : FONcBaseType(), _b( 0 )
{
    _b = dynamic_cast<Int8 *>(b) ;
    if( !_b )
    {
	string s = (string)"File out netcdf, FONcInt8 was passed a "
		   + "variable that is not a DAP4 int8" ;
	throw BESInternalError( s, __FILE__, __LINE__ ) ;
    }
}

/** @brief Destructor that cleans up the DAP4 int8
 *
 * The DAP4 int8 instance does not belong to the FONcInt8 instance, so it
 * is not deleted.
 */
FONcInt8::~FONcInt8()
{
}

/** @brief define the DAP4 int8 in the netcdf file
 *
 * The definition actually takes place in FONcBaseType. This function
 * adds the attributes for the int8 instance as well as an attribute if
 * the name of the int8 had to be modified.
 *
 * @param ncid The id of the NetCDF file
 * @throws BESInternalError if there is a problem defining the
 * int8
 */
void
FONcInt8::define( int ncid )
{
    FONcBaseType::define( ncid ) ;

    if( !_defined )
    {

        if(is_dap4) {                                                                                       
            D4Attributes *d4_attrs = _b->attributes();                                                     
            updateD4AttrType(d4_attrs,NC_BYTE);   
        }
        else {
            AttrTable &attrs = _b->get_attr_table();  
            updateAttrType(attrs,NC_BYTE); 
        }

	FONcAttributes::add_variable_attributes( ncid, _varid, _b,isNetCDF4_ENHANCED(),is_dap4 ) ;
	FONcAttributes::add_original_name( ncid, _varid,
					   _varname, _orig_varname ) ;

	_defined = true ;
    }
}

/** @brief Write the DAP4 int8 out to the netcdf file
 *
 * Once the DAP4 int8 is defined, the value of the DAP4 int8 can be written out
 * as well.
 *
 * @param ncid The id of the netcdf file
 * @throws BESInternalError if there is a problem writing the value out
 * to the netcdf file
 */
void
FONcInt8::write( int ncid )
{
    BESDEBUG( "fonc", "FOncInt8::write for var " << _varname << endl ) ;
    size_t var_index[] = {0} ;
    //char *data = new char ;
    signed char data_value[1];

    if (is_dap4)
        _b->intern_data();
    else
        _b->intern_data(*get_eval(), *get_dds());

    data_value[0] = _b->value();
    
    //_b->buf2val( (void**)&data ) ;
    int stax = nc_put_var1_schar(ncid, _varid, var_index, data_value ) ;
    if( stax != NC_NOERR )
    {
	string err = (string)"fileout.netcdf - "
		     + "Failed to write byte data for "
		     + _varname ;
	FONcUtils::handle_error( stax, err, __FILE__, __LINE__ ) ;
    }
    //delete data ;
}

/** @brief returns the name of the DAP4 int8
 *
 * @returns The name of the DAP4 int8
 */
string
FONcInt8::name()
{
    return _b->name() ;
}

/** @brief returns the netcdf type of the DAP4 int8
 *
 * @returns The nc_type of NC_BYTE
 */
nc_type
FONcInt8::type()
{
    return NC_BYTE ;
}

/** @brief dumps information about this object for debugging purposes
 *
 * Displays the pointer value of this instance plus instance data
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
FONcInt8::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "FONcInt8::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "name = " << _b->name()  << endl ;
    BESIndent::UnIndent() ;
}

