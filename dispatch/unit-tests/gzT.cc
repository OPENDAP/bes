// gzT.C

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

#include <iostream>
#include <fstream>
#include <cstdlib>

using std::cerr ;
using std::cout ;
using std::endl ;
using std::ifstream ;

#include "gzT.h"
#include "BESUncompressGZ.h"
#include "BESCache.h"
#include "BESError.h"
#include <test_config.h>

#define BES_CACHE_CHAR '#' 

int
gzT::run(void)
{
    cout << endl << "*****************************************" << endl;
    cout << "Entered gzT::run" << endl;
    int retVal = 0;

    string cache_dir = (string)TEST_SRC_DIR + "/cache" ;
    string src_file = cache_dir + "/testfile.txt.gz" ;

    // we're not testing the caching mechanism, so just create it, but make
    // sure it gets created.
    string target ;
    try
    {
	BESCache cache( cache_dir, "gz_cache", 1 ) ;
	// get the target name and make sure the target file doesn't exist
	if( cache.is_cached( src_file, target ) )
	{
	    if( remove( target.c_str() ) != 0 )
	    {
		cerr << "Unable to remove target file " << target
		     << " , initializing test" << endl ;
		return 1 ;
	    }
	}

	cout << endl << "*****************************************" << endl;
	cout << "uncompress a test file" << endl;
	try
	{
	    BESUncompressGZ::uncompress( src_file, target ) ;
	    cout << "Uncompression succeeded" << endl ;
	    ifstream strm( target.c_str() ) ;
	    if( !strm )
	    {
		cerr << "Resulting file " << target << " doesn't exist" << endl;
		return 1 ;
	    }
	    char line[80] ;
	    strm.getline( (char *)line, 80 ) ;
	    string sline = line ;
	    if( sline != "This is a test of a compression method." )
	    {
		cerr << "Contents of file not correct" << endl ;
		cerr << "Actual: " << sline << endl ;
		cerr << "Should be: This is a test of a compression method."
		     << endl ;
		return 1 ;
	    }
	    else
	    {
		cout << "Contents of file correct" << endl ;
	    }
	}
	catch( BESError &e )
	{
	    cerr << "Failed to uncompress the file" << endl ;
	    cerr << e.get_message() << endl ;
	    return 1 ;
	}
	catch( ... )
	{
	    cerr << "Failed to uncompress the file" << endl ;
	    cerr << "Unknown exception thrown" << endl ;
	    return 1 ;
	}

	string tmp ;
	if( cache.is_cached( src_file, tmp ) )
	{
	    cout << "File is now cached" << endl ;
	}
	else
	{
	    cerr << "File should be cached" << endl ;
	    return 1 ;
	}

	cout << endl << "*****************************************" << endl;
	cout << "uncompress a test file that is already cached" << endl;
	try
	{
	    BESUncompressGZ::uncompress( src_file, target ) ;
	    cout << "Uncompression succeeded" << endl ;
	    ifstream strm( target.c_str() ) ;
	    if( !strm )
	    {
		cerr << "Resulting file " << target << " doesn't exist" << endl;
		return 1 ;
	    }
	    char line[80] ;
	    strm.getline( (char *)line, 80 ) ;
	    string sline = line ;
	    if( sline != "This is a test of a compression method." )
	    {
		cerr << "Contents of file not correct" << endl ;
		cerr << "Actual: " << sline << endl ;
		cerr << "Should be: This is a test of a compression method."
		     << endl ;
		return 1 ;
	    }
	    else
	    {
		cout << "Contents of file correct" << endl ;
	    }
	}
	catch( BESError &e )
	{
	    cerr << "Failed to uncompress the file" << endl ;
	    cerr << e.get_message() << endl ;
	    return 1 ;
	}
	catch( ... )
	{
	    cerr << "Failed to uncompress the file" << endl ;
	    cerr << "Unknown exception thrown" << endl ;
	    return 1 ;
	}

	if( cache.is_cached( src_file, tmp ) )
	{
	    cout << "File is still cached" << endl ;
	}
	else
	{
	    cerr << "File should be cached" << endl ;
	    return 1 ;
	}

    }
    catch( BESError &e )
    {
	cerr << "Unable to create the cache object" << endl ;
	cerr << e.get_message() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "Unable to create the cache object" << endl ;
	cerr << "Unknown exception thrown" << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Returning from gzT::run" << endl;

    return retVal;
}

int
main(int argC, char **argV) {
    string env_var = (string)"BES_CONF=" + TEST_SRC_DIR + "/bes.conf" ;
    putenv( (char *)env_var.c_str() ) ;
    Application *app = new gzT();
    return app->main(argC, argV);
}

