// BESUncompressGZ.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <zlib.h>

#include <sstream>

using std::ostringstream ;

#include "BESUncompressGZ.h"
#include "BESContainerStorageException.h"
#include "BESDebug.h"

#define CHUNK 4096

/** @brief uncompress a file with the .gz file extension
 *
 * @param src file that will be uncompressed
 * @param target file to uncompress the src file to
 * @return full path to the uncompressed file
 */
string
BESUncompressGZ::uncompress( const string &src, const string &target )
{
    // buffer to hold the uncompressed data
    char in[CHUNK] ;

    // open the file to be read by gzopen. If the file is not compressed
    // using gzip then all this function will do is trasnfer the data to the
    // destination file.
    gzFile gsrc = gzopen( src.c_str(), "rb" ) ;
    if( gsrc == NULL )
    {
	string err = "Could not open the compressed file " + src ;
	throw BESContainerStorageException( err, __FILE__, __LINE__ ) ;
    }

    FILE *dest = fopen( target.c_str(), "wb" ) ;
    if( !dest )
    {
	char *serr = strerror( errno ) ;
	string err = "Unable to create the uncompressed file "
	             + target + ": " ;
	if( serr )
	{
	    err.append( serr ) ;
	}
	else
	{
	    err.append( "unknown error occurred" ) ;
	}
	gzclose( gsrc ) ;
	throw BESContainerStorageException( err, __FILE__, __LINE__ ) ;
    }

    // gzread will read the data in uncompressed. All we have to do is write
    // it to the destination file.
    bool done = false ;
    while( !done )
    {
	int bytes_read = gzread( gsrc, in, CHUNK ) ;
	if( bytes_read == 0 )
	{
	    done = true ;
	}
	else
	{
	    int bytes_written = fwrite( in, 1, bytes_read, dest) ;
	    if( bytes_written < bytes_read )
	    {
		ostringstream strm ;
		strm << "Error writing uncompressed data "
		             << "to dest file " << target << ": "
		             << "wrote " << bytes_written << " "
			     << "instead of " << bytes_read ;
		gzclose( gsrc ) ;
		fclose( dest ) ;
		throw BESContainerStorageException( strm.str(), __FILE__, __LINE__ ) ;
	    }
	}
    }

    gzclose( gsrc ) ;
    fclose( dest ) ;

    return target ;
}

