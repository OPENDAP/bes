// FONcArrayStructureField.cc

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
#include <libdap/Array.h>
#include <BESDebug.h>
#include <BESUtil.h>

#include "FONcArrayStructureField.h"
#include "FONcDim.h"
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
FONcArrayStructureField::FONcArrayStructureField( BaseType *b )
    : FONcBaseType()
{
    d_a = dynamic_cast<Array *>(b);
    if (!d_a) {
        string s = "File out netcdf, FONcArray was passed a variable that is not a DAP Array";
        throw BESInternalError(s, __FILE__, __LINE__);
    }

}

/** @brief Destructor that cleans up the instance
 *
 * The DAP Int32 or UInt32 instance does not belong to the FONcByte
 * instance, so it is not deleted.
 */
FONcArrayStructureField::~FONcArrayStructureField()
{
}

void
FONcArrayStructureField::convert_asf( std::vector<std::string> embed) {


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
FONcArrayStructureField::define( int ncid )
{
}

/** @brief Write the int out to the netcdf file
 *
 * Once the int is defined, the value of the int can be written out
 *
 * @param ncid The id of the netcdf file
 * @throws BESInternalError if there is a problem writing the value
 */
void
FONcArrayStructureField::write( int ncid )
{
    BESDEBUG( "fonc", "FONcArrayStructureField::write for var " << d_varname << endl ) ;
}

/** @brief returns the name of the DAP Int32 or UInt32
 *
 * @returns The name of the DAP Int32 or UInt32
 */
string
FONcArrayStructureField::name()
{
    return var_name ;
}

/** @brief returns the netcdf type of the DAP object
 *
 * @returns The nc_type of NC_INT
 */
nc_type
FONcArrayStructureField::type()
{
    return d_array_type;
}

/** @brief dumps information about this object for debugging purposes
 *
 * Displays the pointer value of this instance plus instance data
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
FONcArrayStructureField::dump( ostream &strm ) const
{
#if 0
    strm << BESIndent::LMarg << "FONcArrayStructureField::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "name = " << _bt->name()  << endl ;
    BESIndent::UnIndent() ;
#endif
}

