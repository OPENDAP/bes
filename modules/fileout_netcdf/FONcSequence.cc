// FONcSequence.cc

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

#include "FONcSequence.h"
#include "FONcUtils.h"

/** @brief Constructor for FONcSequence that takes a DAP Sequence
 *
 * This constructor takes a DAP BaseType and makes sure that it is a DAP
 * Sequence instance. If not, it throws an exception
 *
 * @param b A DAP BaseType that should be a Sequence
 * @throws BESInternalError if the BaseType is not a Sequence
 */
FONcSequence::FONcSequence( BaseType *b )
    : FONcBaseType(), _s( 0 )
{
    _s = dynamic_cast<Sequence *>(b) ;
    if( !_s )
    {
	string s = (string)"File out netcdf, FONcSequence was passed a "
		   + "variable that is not a DAP Sequence" ;
	throw BESInternalError( s, __FILE__, __LINE__ ) ;
    }
}

/** @brief Destructor that cleans up the sequence
 *
 * The DAP Sequence instance does not belong to the FONcSequence
 * instance, so it is not delete it.
 */
FONcSequence::~FONcSequence()
{
}

/** @brief convert the Sequence to something that can be stored in a
 * netcdf file
 *
 * Currently Sequences are not supported by FileOut NetCDF
 *
 * @param embed The list of parent names for this sequence
 * @throws BESInternalError if there is a problem converting the
 * Byte
 */
void
FONcSequence::convert( vector<string> embed )
{
    FONcBaseType::convert( embed ) ;
    _varname = FONcUtils::gen_name( embed, _varname, _orig_varname ) ;
}

/** @brief define the DAP Sequence in the netcdf file
 *
 * Currently, Sequences are not supported by FileOut NetCDF. For now, a
 * global attribute is added to the netcdf file stating that the
 * sequence is not written to the file.
 *
 * @param ncid The id of the NetCDF file
 * @throws BESInternalError if there is a problem writing out the
 * attribute
 */
void
FONcSequence::define( int ncid )
{
    // for now we are simply going to add a global variable noting the
    // presence of the sequence, the name of the sequence, and that the
    // sequences has been elided.
    string val = (string)"The sequence " + _varname
		 + " is a member of this dataset and has been elided." ;
    int stax = nc_put_att_text( ncid, NC_GLOBAL, _varname.c_str(),
				val.length(), val.c_str() ) ;
    if( stax != NC_NOERR )
    {
	string err = (string)"File out netcdf, "
		     + "failed to write string attribute for sequence "
		     + _varname ;
	FONcUtils::handle_error( stax, err, __FILE__, __LINE__ ) ;
    }
}

/** @brief Write the sequence data out to the netcdf file
 *
 * Currently, Sequences are not supported by FileOut NetCDF. Nothing is
 * done in this method.
 *
 * @param ncid The id of the netcdf file
 * @throws BESInternalError if there is a problem writing the value out
 * to the netcdf file
 */
void
FONcSequence::write( int /*ncid*/ )
{
}

string
FONcSequence::name()
{
    return _s->name() ;
}

/** @brief dumps information about this object for debugging purposes
 *
 * Displays the pointer value of this instance plus instance data
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
FONcSequence::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "FONcSequence::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "name = " << _s->name()  << endl ;
    BESIndent::UnIndent() ;
}

