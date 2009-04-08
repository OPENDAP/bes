// BESUncompressBZ2.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
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
 
// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>


#include "config.h"

#ifdef HAVE_BZLIB_H
#include <bzlib.h>
#endif

#include <cstring>
#include <cerrno>
#include <sstream>

using std::ostringstream ;

#include "BESUncompressBZ2.h"
#include "BESInternalError.h"
#include "BESDebug.h"

#define CHUNK 4096

void
bz_internal_error ( int errcode )
{
    ostringstream strm ;
    strm << "internal error in bz2 library occurred: " << errcode ;
    throw BESInternalError( strm.str(), __FILE__, __LINE__ ) ;
}

/** @brief uncompress a file with the .bz2 file extension
 *
 * @param src_name file that will be uncompressed
 * @param target file to uncompress the src file to
 * @return full path to the uncompressed file
 */
void
BESUncompressBZ2::uncompress( const string &src_name, const string &target )
{
#ifndef HAVE_BZLIB_H
    string err = "Unable to uncompress bz2 files, feature not built. Check config.h in bes directory for HAVE_BZLIB_H flag set to 1" ;
    throw BESInternalError( err, __FILE__, __LINE__ ) ;
#else
    FILE *src = fopen( src_name.c_str(), "rb" ) ;
    if( !src )
    {
	char *serr = strerror( errno ) ;
	string err = "Unable to open the compressed file "
	             + src_name + ": " ;
	if( serr )
	{
	    err.append( serr ) ;
	}
	else
	{
	    err.append( "unknown error occurred" ) ;
	}
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
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
	fclose( src ) ;
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }

    int bzerror = 0 ; // any error flags will be stored here
    int verbosity = 0 ; // 0 is silent up to 4 which is very verbose
    int small = 0 ; // if non zero then memory management is different
    void *unused = NULL ; // any unused bytes would be stored in here
    int nunused = 0 ; // the size of the unused buffer
    char in[CHUNK] ; // input buffer used to read uncompressed data in bzRead

    BZFILE *bsrc = NULL ;

    bsrc = BZ2_bzReadOpen( &bzerror, src, verbosity, small, NULL, 0 ) ;
    if( bsrc == NULL )
    {
	const char *berr = BZ2_bzerror( bsrc, &bzerror ) ;
	string err = "bzReadOpen failed on " + src_name + ": "  ;
	if( berr )
	{
	    err.append( berr ) ;
	}
	else
	{
	    err.append( "Unknown error" ) ;
	}
	fclose( dest ) ;
	fclose( src ) ;

	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }

    bool done = false ;
    while( !done )
    {
	int bytes_read = BZ2_bzRead( &bzerror, bsrc, in, CHUNK ) ;
	if( bzerror != BZ_OK && bzerror != BZ_STREAM_END )
	{
	    const char *berr = BZ2_bzerror( bsrc, &bzerror ) ;
	    string err = "bzRead failed on " + src_name + ": " ;
	    if( berr )
	    {
		err.append( berr ) ;
	    }
	    else
	    {
		err.append( "Unknown error" ) ;
	    }

	    BZ2_bzReadClose( &bzerror, bsrc ) ;
	    fclose( dest ) ;
	    fclose( src ) ;

	    throw BESInternalError( err, __FILE__, __LINE__ ) ;
	}
	//if( bytes_read == 0 || bzerror == BZ_STREAM_END )
	if( bzerror == BZ_STREAM_END )
	{
	    done = true ;
	}
	int bytes_written = fwrite( in, 1, bytes_read, dest) ;
	if( bytes_written < bytes_read )
	{
	    ostringstream strm ;
	    strm << "Error writing uncompressed data "
			 << "to dest file " << target << ": "
			 << "wrote " << bytes_written << " "
			 << "instead of " << bytes_read ;

	    BZ2_bzReadClose( &bzerror, bsrc ) ;
	    fclose( dest ) ;
	    fclose( src ) ;

	    throw BESInternalError( strm.str(), __FILE__, __LINE__ ) ;
	}
    }

    BZ2_bzReadClose( &bzerror, bsrc ) ;
    fclose( dest ) ;
    fclose( src ) ;
#endif
}

