// BESContainer.cc

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

#include <stdio.h>
#include <errno.h>
#include <fstream>
#include <sstream>

using std::ifstream ;
using std::ofstream ;
using std::ios_base ;
using std::ostringstream ;

#include "BESContainer.h"
#include "TheBESKeys.h"
#include "BESContainerStorageException.h"
#include "GNURegex.h"

string BESContainer::_cacheDir ;
string BESContainer::_compressedExt ;
string BESContainer::_script ;
string BESContainer::_cacheSize ;

BESContainer::BESContainer(const string &s)
    : _valid( false ),
      _real_name( "" ),
      _constraint( "" ),
      _symbolic_name( s ),
      _container_type( "" ),
      _attributes( "" )
{
}

BESContainer::BESContainer( const BESContainer &copy_from )
    : _valid( copy_from._valid ),
      _real_name( copy_from._real_name ),
      _constraint( copy_from._constraint ),
      _symbolic_name( copy_from._symbolic_name ),
      _container_type( copy_from._container_type ),
      _attributes( copy_from._attributes )
{
}

string
BESContainer::access()
{
    // Determine if this file is compressed. If it isn't, then just return
    // the real name
    if( BESContainer::_compressedExt.empty() )
    {
	bool found = false ;
	string key = "BES.Compressed.Extensions" ;
	BESContainer::_compressedExt =
	    TheBESKeys::TheKeys()->get_key( key, found ) ;
	if( !found || BESContainer::_compressedExt.empty() )
	{
	    BESContainer::_compressedExt = ".*(\\.gz|\\.Z|\\.bz2)$" ;
	}
    }

    Regex iscompressed( BESContainer::_compressedExt.c_str() ) ;
    if( iscompressed.match( _real_name.c_str(), _real_name.length() ) == -1)
    {
	return _real_name ;
    }

    // Determine the cache directory where uncompressed files will be
    // stored. If it's already been set then we assume that it exists and is
    // writable
    if( BESContainer::_cacheDir.empty() )
    {
	bool found = false ;
	BESContainer::_cacheDir = TheBESKeys::TheKeys()->get_key( "BES.CacheDir", found ) ;
	if( !found || BESContainer::_cacheDir.empty() )
	{
	    BESContainer::_cacheDir = "/tmp" ;
	}

        string dummy = BESContainer::_cacheDir + "/dummy" ;
	ofstream teststrm( dummy.c_str(), ios_base::out|ios_base::trunc ) ;
        if( !teststrm )
        {
	    BESContainer::_cacheDir = "";
	    string err = "Could not create a file in the cache directory ("
			 + BESContainer::_cacheDir + ")" ;
	    throw BESContainerStorageException( err, __FILE__, __LINE__ ) ;
        }
	teststrm.close() ;
    }

    // Determine the script to call
    if( BESContainer::_script.empty() )
    {
	bool found = false ;
	BESContainer::_script = TheBESKeys::TheKeys()->get_key( "BES.Compressed.Script", found ) ;
	if( !found || BESContainer::_script.empty() )
	{
	    string err = (string)"Script used to uncompress compressed files "
	                 + "is not set. Please set 'BES.Compressed.Script' "
			 + "in the BES configuration file" ;
	    throw BESContainerStorageException( err, __FILE__, __LINE__ ) ;
	}
    }

    // Determine the cache size
    if( BESContainer::_cacheSize.empty() )
    {
	bool found = false ;
	string key = "BES.CacheDir.MaxSize" ;
	BESContainer::_cacheSize =
	    TheBESKeys::TheKeys()->get_key( key, found ) ;
	if( !found || BESContainer::_cacheSize.empty() )
	{
	    BESContainer::_cacheSize = "500" ;
	}
    }

    // Build the command
    string cmd = BESContainer::_script + " "
                 + _real_name + " "
		 + BESContainer::_cacheDir + " "
		 + BESContainer::_cacheSize ;

    // Call the script that will uncompress the file. The script should exit
    // with 0 if there are no problems uncompressing the file and echo to
    // stdout the name of the cached file. If there is an error, return a
    // non-zero value and echo out to stdout the problem encountered.
    ostringstream output ;
    FILE *f = popen( cmd.c_str(), "r" ) ;
    if( f )
    {
	char buf[4096] ;
	while( fgets( buf, 4096, f ) != NULL )
	    output << buf ;
	int stat = pclose( f ) ;
	if( stat != 0 )
	{
	    string err = "Problem uncompressing the data file: \n"
	                 + output.str() ;
	    throw BESContainerStorageException( err, __FILE__, __LINE__ ) ;
	}
    }

    return output.str() ;
}

/* The following code calls the uncompression programs directly after making
 * several checks. The issue with this is that in addition to calling these
 * programs, like gzip, we have to open up a file and read the output from
 * the program and dump it into that file. If the compressed files are
 * rather large than this could take some time. Whereas by calling a script
 * that calls these programs we could save some time.
string
BESContainer::access()
{
    // Determine if this file is compressed. If it isn't, then just return
    // the real name
    if( BESContainer::_compressedExt.empty() )
    {
	bool found = false ;
	string key = "BES.CompressedExtensions" ;
	BESContainer::_compressedExt =
	    TheBESKeys::TheKeys()->get_key( key, found ) ;
	if( !found || BESContainer::_compressedExt.empty() )
	{
	    BESContainer::_compressedExt = ".*(\\.gz|\\.Z|\\.bz2)$" ;
	}
    }
    Regex iscompressed( BESContainer::_compressedExt.c_str() ) ;
    if( iscompressed.match( _real_name.c_str(), _real_name.length() ) == -1)
    {
	return _real_name ;
    }

    // determine if the compressed file actually exists
    ifstream real_exists( _real_name.c_str(), ios_base::in ) ;
    if( !real_exists )
    {
	string err = "The compressed file does not exist or cannot be opened ("
	             + _real_name + ")" ;
	throw BESContainerStorageException( err, __FILE__, __LINE__ ) ;
    }
    real_exists.close() ;

    // make sure the file name doesn't contain any shell meta characters
    string reg = ".*[\s%&()*?<>;]+.*@" ;
    Regex hasmeta( reg.c_str() ) ;
    if( hasmeta.match( _real_name.c_str(), _real_name.length() ) != -1 )
    {
	string err = "File name contains shell meta characters ("
	             + _real_name + ")" ;
	throw BESContainerStorageException( err, __FILE__, __LINE__ ) ;
    }

    // Determine the cache directory where uncompressed files will be
    // stored. If it's already been set then we assume that it exists and is
    // writable
    if( BESContainer::_cacheDir.empty() )
    {
	bool found = false ;
	BESContainer::_cacheDir = TheBESKeys::TheKeys()->get_key( "BES.CacheDir", found ) ;
	if( !found || BESContainer::_cacheDir.empty() )
	{
	    BESContainer::_cacheDir = "/tmp" ;
	}

        string dummy = BESContainer::_cacheDir + "/dummy" ;
	ofstream teststrm( dummy.c_str(), ios_base::out|ios_base::trunc ) ;
        if( !teststrm )
        {
	    BESContainer::_cacheDir = "";
	    string err = "Could not create a file in the cache directory ("
			 + BESContainer::_cacheDir + ")" ;
	    throw BESContainerStorageException( err, __FILE__, __LINE__ ) ;
        }
	teststrm.close() ;
    }

    // Determine the name of the uncompressed file
    string cache_entity = _real_name ;
    //remove a leading slash
    if( cache_entity[0] == '/' )
    {
	cache_entity = cache_entity.substr( 1, cache_entity.length() - 1 ) ;
    }

    //replace all slashes with pound sign
    string::size_type slashes = cache_entity.find( '/' ) ;
    while( slashes != string::npos )
    {
	cache_entity.replace( slashes, 1, "#" ) ;
	slashes = cache_entity.find( '/', slashes+1 ) ;
    }

    //remove the ending file extension (.gz, .Z, .bz2)
    string::size_type lastdot = cache_entity.find_last_of( '.' ) ;
    cache_entity = cache_entity.substr( 0, lastdot ) ;

    string cache_file = BESContainer::_cacheDir + "/bes_cache#" + cache_entity ;

    // Determine if the file has already been uncompressed
    ifstream strm( cache_file.c_str(), ios_base::in ) ;
    if( strm.good() )
    {
	strm.close() ;
	return cache_file ;
    }

    // Uncompress the file
    string cmd = "gzip -c -d " + _real_name ;

    ofstream output( cache_file.c_str(), ios_base::out|ios_base::trunc ) ;
    if( !output )
    {
	string err = "Unable to create the uncompressed file ("
	             + cache_file + ")" ;
	throw BESContainerStorageException( err, __FILE__, __LINE__ ) ;
    }

    FILE *f = popen( cmd.c_str(), "r" ) ;
    if( f )
    {
	char buf[4096] ;
	while( fgets( buf, 4096, f ) != NULL )
	    output << buf ;
	int stat = pclose( f ) ;
	if( stat != 0 )
	{
	    string err = "Problem uncompressing the file ("
			 + _real_name + ") to (" + cache_file + ")" ;
	    char *errnostr = strerror( errno ) ;
	    if( errnostr )
		err += (string)"\n" + errnostr ;
	    throw BESContainerStorageException( err, __FILE__, __LINE__ ) ;
	}
    }
    output.flush() ;
    output.close() ;

    return cache_file ;
}
*/

