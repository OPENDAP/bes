// uncompressT.C

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

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

using namespace CppUnit ;

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <dirent.h>

using std::cerr ;
using std::cout ;
using std::endl ;
using std::ifstream ;

#include "config.h"
#include "BESUncompressManager3.h"
#include "BESUncompressCache.h"
#include "BESError.h"
#include "TheBESKeys.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include <test_config.h>

#define BES_CACHE_CHAR '#' 

class uncompressT: public TestFixture {
private:

public:
    uncompressT() {}
    ~uncompressT() {}

    int clean_dir(string dirname, string prefix){
        DIR *dp;
        struct dirent *dirp;
        if((dp  = opendir(dirname.c_str())) == NULL) {
            cout << "Error(" << errno << ") opening " << dirname << endl;
            return errno;
        }

        while ((dirp = readdir(dp)) != NULL) {
            string name(dirp->d_name);
            if (name.find(prefix) == 0){
                string abs_name = BESUtil::assemblePath(dirname, name, true);
                cerr << "Purging file: " << abs_name << endl;
                remove(abs_name.c_str());
            }

        }
        closedir(dp);
        return 0;
    }


    void setUp()
    {
        string bes_conf = (string)TEST_ABS_SRC_DIR + "/bes.conf" ;
        TheBESKeys::ConfigFile = bes_conf ;

        cerr << "-------------------------------------------" << endl;
        cerr << "setup() - BEGIN " << endl;

        BESDebug::SetUp("cerr,cache,uncompress,uncompress2");
        cerr << "setup() - BESDEBUG Enabled " << endl;
   }

    void tearDown()
    {
    }


    void test_worker(string cache_prefix, string test_file_base, string test_file_suffix)
    {

        cout << "cache_prefix: " << cache_prefix << endl;

        string cache_dir = (string)TEST_SRC_DIR + "/cache" ;
        cout << "cache_dir: " << cache_dir << endl;
        // Clean it up...

        clean_dir(cache_dir, cache_prefix);

        string src_file_base = cache_dir + test_file_base ;
        string src_file = src_file_base + test_file_suffix ;

        // we're not testing the caching mechanism, so just create it, but make
        // sure it gets created.
        try
        {
            BESUncompressCache *cache =  BESUncompressCache::get_instance(cache_dir, cache_dir, cache_prefix, 1) ;
            // get the target name and make sure the target file doesn't exist
            string cache_file_name = cache->get_cache_file_name(src_file_base,false);
            if( cache->is_valid(cache_file_name,src_file) )
                cache->purge_file(cache_file_name) ;

            cout << "*****************************************" << endl;
            cout << "uncompress a test " << test_file_suffix << " file" << endl;
            try
            {
                string result ;
                bool cached = BESUncompressManager3::TheManager()->uncompress( src_file, result, cache ) ;
                CPPUNIT_ASSERT( cached ) ;
                cerr << "expected: " << cache_file_name << endl;
                cerr << "result:   " << result << endl;
                CPPUNIT_ASSERT( result == cache_file_name ) ;

                ifstream strm( cache_file_name.c_str() ) ;
                CPPUNIT_ASSERT( strm ) ;

                char line[80] ;
                strm.getline( (char *)line, 80 ) ;
                string sline = line ;
                string should_be = "This is a test of a compression method." ;
                cout << "    contents = " << sline << endl ;
                cout << "    expected = " << should_be << endl ;
                CPPUNIT_ASSERT( sline == should_be ) ;
            }
            catch( BESError &e )
            {
                cerr << e.get_message() << endl ;
                CPPUNIT_ASSERT( !"Failed to uncompress the gz file" ) ;
            }

            string tmp ;
            CPPUNIT_ASSERT( cache->is_valid(cache_file_name,src_file) ) ;

            cout << "*****************************************" << endl;
            cout << "uncompress a test "<< test_file_suffix << " file, should be cached" << endl;
            try
            {
                string result ;
                bool cached = BESUncompressManager3::TheManager()->uncompress( src_file, result, cache ) ;
                CPPUNIT_ASSERT( cached ) ;
                CPPUNIT_ASSERT( result == cache_file_name ) ;

                ifstream strm( cache_file_name.c_str() ) ;
                CPPUNIT_ASSERT( strm ) ;

                char line[80] ;
                strm.getline( (char *)line, 80 ) ;
                string sline = line ;
                string should_be = "This is a test of a compression method." ;
                cout << "    contents = " << sline << endl ;
                cout << "    expected = " << should_be << endl ;
                CPPUNIT_ASSERT( sline == should_be ) ;
            }
            catch( BESError &e )
            {
                cerr << e.get_message() << endl ;
                CPPUNIT_ASSERT( !"Failed to uncompress the file" ) ;
            }

            CPPUNIT_ASSERT( cache->is_valid(cache_file_name,src_file) ) ;
        }
        catch( BESError &e )
        {
            cerr << "Caught BESError. Message: " << e.get_message() << endl ;
            CPPUNIT_ASSERT( !"Unable to create the required cache object." ) ;
        }


    }


    void do_gz_test(){
        cout << "*****************************************" << endl;
        cout << "uncompressT::"<< __func__ << "() - BEGIN"  << endl;
        string cache_prefix="gzcache";
        string test_file_base = "/testfile.txt" ;
        string test_file_suffix = ".gz" ;

        test_worker(cache_prefix, test_file_base, test_file_suffix);
    }
    void do_Z_test(){
        cout << "*****************************************" << endl;
        cout << "uncompressT::"<< __func__ << "() - BEGIN"  << endl;
        string cache_prefix="Zcache";
        string test_file_base = "/testfile.txt" ;
        string test_file_suffix = ".Z" ;

        test_worker(cache_prefix, test_file_base, test_file_suffix);
    }

    void do_libz2_test(){
#ifdef HAVE_LIBBZ2
        cout << "*****************************************" << endl;
        cout << "uncompressT::"<< __func__ << "() - BEGIN"  << endl;
        string cache_prefix="bz2cache";
        string test_file_base = "/testfile.txt" ;
        string test_file_suffix = ".bz2" ;

        test_worker(cache_prefix, test_file_base, test_file_suffix);
#endif
    }




    CPPUNIT_TEST_SUITE( uncompressT ) ;

    CPPUNIT_TEST( do_gz_test ) ;
    CPPUNIT_TEST( do_libz2_test ) ;
    CPPUNIT_TEST( do_Z_test ) ;

    CPPUNIT_TEST_SUITE_END() ;


} ;

CPPUNIT_TEST_SUITE_REGISTRATION( uncompressT ) ;

int 
main( int, char** )
{
    CppUnit::TextTestRunner runner ;
    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() ) ;

    bool wasSuccessful = runner.run( "", false )  ;

    return wasSuccessful ? 0 : 1 ;
}

