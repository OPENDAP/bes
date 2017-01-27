// FONcDouble.cc

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
#include <Float64.h>

#include "FONcDouble.h"
#include "FONcUtils.h"
#include "FONcAttributes.h"

/** @brief Constructor for FOncDouble that takes a DAP Float64
 *
 * This constructor takes a DAP BaseType and makes sure that it is a DAP
 * Float64 instance. If not, it throws an exception
 *
 * @param b A DAP BaseType that should be a float64
 * @throws BESInternalError if the BaseType is not a Float64
 */
FONcDouble::FONcDouble( BaseType *b )
    : FONcBaseType(), _f( 0 )
{
    _f = dynamic_cast<Float64 *>(b) ;
    if( !_f )
    {
	string s = (string)"File out netcdf, FONcDouble was passed a "
		   + "variable that is not a DAP Float64" ;
	throw BESInternalError( s, __FILE__, __LINE__ ) ;
    }
}

/** @brief Destructor that cleans up the instance
 *
 * Since the DAP object is not owned by this instance, it is not deleted
 */
FONcDouble::~FONcDouble()
{
}

/** @brief define the DAP Float64 in the netcdf file
 *
 * The parent class, FONcBaseType, actually defines the variable since
 * it is a simple type. This method adds the attributes for the Float64
 * and an attribute if the name had to be modified in any way.
 *
 * @param ncid The id of the NetCDF file
 * @throws BESInternalError if there is a problem defining the
 * variable
 */
void
FONcDouble::define( int ncid )
{
    FONcBaseType::define( ncid ) ;

    if( !_defined )
    {
	FONcAttributes::add_variable_attributes( ncid, _varid, _f ) ;
	FONcAttributes::add_original_name( ncid, _varid,
					   _varname, _orig_varname ) ;

	_defined = true ;
    }
}

/** @brief Write the float64 out to the netcdf file
 *
 * Once the double is defined, the value can be written out
 *
 * @param ncid The id of the netcdf file
 * @throws BESInternalError if there is a problem writing the value
 */
void
FONcDouble::write( int ncid )
{
    BESDEBUG( "fonc", "FONcDouble::write for var " << _varname << endl ) ;
    size_t var_index[] = {0} ;
    double *data = new double ;
    _f->buf2val( (void**)&data ) ;
    int stax = nc_put_var1_double( ncid, _varid, var_index, data ) ;
    if( stax != NC_NOERR )
    {
	string err = (string)"fileout.netcdf - "
		     + "Failed to write double data for "
		     + _varname ;
	FONcUtils::handle_error( stax, err, __FILE__, __LINE__ ) ;
    }
    delete data ;
    BESDEBUG( "fonc", "FONcDouble::done write for var " << _varname << endl ) ;
}

/** @brief returns the name of the DAP Float64
 *
 * @returns The name of the DAP Float64
 */
string
FONcDouble::name()
{
    return _f->name() ;
}

/** @brief returns the netcdf type of the DAP Float64
 *
 * @returns The nc_type of NC_DOUBLE
 */
nc_type
FONcDouble::type()
{
    return NC_DOUBLE ;
}

/** @brief dumps information about this object for debugging purposes
 *
 * Displays the pointer value of this instance plus instance data
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
FONcDouble::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "FONcDouble::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "name = " << _f->name()  << endl ;
    BESIndent::UnIndent() ;
}

