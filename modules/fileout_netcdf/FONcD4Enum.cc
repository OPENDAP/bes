// FONcD4Enum.cc

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
#include <libdap/D4Enum.h>

#include "FONcD4Enum.h"
#include "FONcUtils.h"
#include "FONcAttributes.h"

/** @brief Constructor for FONcD4Enum that takes a DAP D4Enum32
 *
 * This constructor takes a DAP BaseType and makes sure that it is a DAP
 * D4Enum32 instance. If not, it throws an exception
 *
 * @param b A DAP BaseType that should be a float32
 * @throws BESInternalError if the BaseType is not a D4Enum32
 */
FONcD4Enum::FONcD4Enum( BaseType *b )
    : FONcBaseType(), _f( 0 )
{
    _f = dynamic_cast<D4Enum *>(b) ;
    if( !_f )
    {
	string s = (string)"File out netcdf, FONcD4Enum was passed a "
		   + "variable that is not a DAP D4Enum32" ;
	throw BESInternalError( s, __FILE__, __LINE__ ) ;
    }
}

/** @brief Destructor that cleans up this instance
 *
 * The DAP D4Enum32 instance does not belong to the FONcD4Enum instance, so it
 * is not deleted up.
 */
FONcD4Enum::~FONcD4Enum()
{
}

/** @brief define the DAP D4Enum32 in the netcdf file
 *
 * The definition actually takes place in FONcBaseType. This function
 * adds the attributes for the D4Enum32 instance as well as an attribute if
 * the name of the D4Enum32 had to be modified.
 *
 * @param ncid The id of the NetCDF file
 * @throws BESInternalError if there is a problem defining the
 * D4Enum32
 */
void
FONcD4Enum::define( int ncid )
{
    FONcBaseType::define( ncid ) ;

#if 0
    if( !d_defined )
    {
        if(d_is_dap4) {
            D4Attributes *d4_attrs = _f->attributes();                                                     
            updateD4AttrType(d4_attrs,NC_FLOAT);   
        }
        else {
            AttrTable &attrs = _f->get_attr_table();  
            updateAttrType(attrs,NC_FLOAT); 
        }


	FONcAttributes::add_variable_attributes(ncid, d_varid, _f, isNetCDF4_ENHANCED() , d_is_dap4) ;
	FONcAttributes::add_original_name(ncid, d_varid,
                                      d_varname, d_orig_varname ) ;

        d_defined = true ;
    }
#endif
}

/** @brief Write the float out to the netcdf file
 *
 * Once the float is defined, the value of the float can be written out
 * as well.
 *
 * @param ncid The id of the netcdf file
 * @throws BESInternalError if there is a problem writing the value out
 * to the netcdf file
 */
void
FONcD4Enum::write( int ncid )
{

#if 0
    BESDEBUG( "fonc", "FONcD4Enum::write for var " << d_varname << endl ) ;
    size_t var_index[] = {0} ;
    float *data = new float ;

    if (d_is_dap4)
        _f->intern_data();
    else
        _f->intern_data(*get_eval(), *get_dds());

    _f->buf2val( (void**)&data ) ;
    int stax = nc_put_var1_float(ncid, d_varid, var_index, data ) ;
    ncopts = NC_VERBOSE ;
    if( stax != NC_NOERR )
    {
	string err = (string)"fileout.netcdf - "
                 + "Failed to write float data for "
                 + d_varname ;
	FONcUtils::handle_error( stax, err, __FILE__, __LINE__ ) ;
    }
    delete data ;
    BESDEBUG( "fonc", "FONcD4Enum::done write for var " << d_varname << endl ) ;
#endif
}

/** @brief returns the name of the DAP D4Enum32
 *
 * @returns The name of the DAP D4Enum32
 */
string
FONcD4Enum::name()
{
    return _f->name() ;
}

#if 0
/** @brief returns the netcdf type of the DAP D4Enum32
 *
 * @returns The nc_type of NC_FLOAT
 */
nc_type
FONcD4Enum::type()
{
    return NC_FLOAT ;
}
#endif

#if 0
/** @brief dumps information about this object for debugging purposes
 *
 * Displays the pointer value of this instance plus instance data
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
FONcD4Enum::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "FONcD4Enum::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "name = " << _f->name()  << endl ;
    BESIndent::UnIndent() ;
}
#endif

