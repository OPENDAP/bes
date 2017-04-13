// cacheT.C

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

#include <unistd.h>  // for sleep
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>  // for closedir opendir

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <GetOpt.h>

using std::cerr ;
using std::endl ;
using std::ostringstream ;

#include <TheBESKeys.h>
#include <BESError.h>
#include <BESDebug.h>
#include <BESUtil.h>
#include <BESFileLockingCache.h>

#include <test_config.h>


// Not run by default!
// Set from the command-line invocation of the main only
// since we're not sure the OS has room for the test files.
static bool RUN_64_BIT_CACHE_TEST = false;

static const std::string CACHE_PREFIX = string("bes_cache");
static const std::string MATCH_PREFIX = string(CACHE_PREFIX) + string("#");

static const std::string TEST_CACHE_DIR = (string)TEST_SRC_DIR + "/cache" ;


// For the 64 bit  (> 4G tests)
static const std::string CACHE_DIR_TEST_64 = string(TEST_SRC_DIR) + string("/test_cache_64");
static const unsigned long long MAX_CACHE_SIZE_IN_MEGS_TEST_64 = 5000ULL; // in Mb...
// shift left by 20 multiplies by 1Mb
static const unsigned long long MAX_CACHE_SIZE_IN_BYTES_TEST_64 = (MAX_CACHE_SIZE_IN_MEGS_TEST_64 << 20);
static const unsigned long long NUM_FILES_TEST_64 = 6ULL;
static const unsigned long long FILE_SIZE_IN_MEGS_TEST_64 = 1024ULL; // in Mb


static bool debug = false;
static bool bes_debug = false;


#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

/** @brief Set up the cache.
Add to the cache a set of eight test files, with names that are easy to
work with and each with an access time two seconds later than the
preceding one.

@param cache_dir Directory that holds the cached files.
 */
void
init_cache( const string &cache_dir )
{
    DBG( cerr << __func__ << "() - BEGIN " << endl);

    //string chmod = (string)"chmod a+w " + TEST_SRC_DIR + "/cache" ;
    //system( chmod.c_str() ) ;

    string t_file = cache_dir + "/template.txt" ;
    for( int i = 1; i < 9; i++ )
    {
        ostringstream s ;
        s << "cp -f " << t_file << " " << BESUtil::assemblePath(cache_dir,CACHE_PREFIX) << "#usr#local#data#template0" << i << ".txt" ;
        DBG( cerr << __func__ << "() - cmd: " << s.str() << endl );
        system( s.str().c_str() );

        ostringstream m ;
        m << "chmod a+w " << BESUtil::assemblePath(cache_dir,CACHE_PREFIX) << "#usr#local#data#template0" << i << ".txt" ;
        DBG( cerr << __func__ << "() - cmd: " << m.str() << endl );
        system( m.str().c_str() ) ;
    }

    string touchers[8] = { "7", "6", "4", "2", "8", "5", "3", "1" } ;
    for( int i = 0; i < 8; i++ )
    {
        DBG( cerr << __func__ << "() - sleeping for 1 second..." << endl );
        sleep(1);
        string cmd = (string)"cat " + BESUtil::assemblePath(cache_dir,CACHE_PREFIX) +
            + "#usr#local#data#template0"
            + touchers[i]
                       + ".txt > /dev/null" ;
        DBG( cerr << __func__ << "() - cmd: " << cmd << endl );
        system( cmd.c_str() );
    }
    DBG( cerr << __func__ << "() - END " << endl);
}
void
check_cache( const string &cache_dir, map<string,string> &should_be )
{
    DBG( cerr << __func__ << "() - BEGIN " << endl);

    map<string,string> contents ;
    string match_prefix = MATCH_PREFIX;
    DIR *dip = opendir( cache_dir.c_str() ) ;
    CPPUNIT_ASSERT( dip ) ;
    struct dirent *dit;
    while( ( dit = readdir( dip ) ) != NULL )
    {
        string dirEntry = dit->d_name ;
        if( dirEntry.compare( 0, match_prefix.length(), match_prefix ) == 0)
            contents[dirEntry] = dirEntry ;
    }
    closedir( dip ) ;

    CPPUNIT_ASSERT( should_be.size() == contents.size() ) ;
    map<string,string>::const_iterator ci = contents.begin() ;
    map<string,string>::const_iterator ce = contents.end() ;
    map<string,string>::const_iterator si = should_be.begin() ;
    map<string,string>::const_iterator se = should_be.end() ;
    bool good = true ;
    for( ; ci != ce; ci++, si++ )
    {
        if( (*ci).first != (*si).first )
        {
            DBG( cerr << __func__ << "() - contents: " << (*ci).first
                << " - should be: " << (*si).first << endl );
            good = false ;
        }
        else {
            DBG( cerr << __func__ << "() - " << (*ci).first << " matches." << endl );

        }
        CPPUNIT_ASSERT( (*ci).first == (*si).first ) ;
    }
    DBG( cerr << __func__ << "() - END " << endl);
}

void
purge_cache(const string &cache_dir, const string &cache_prefix){
    DBG( cerr << __func__ << "() - BEGIN " << endl);
    ostringstream s ;
    s << "rm -" << (debug?"v":"") << "f " << BESUtil::assemblePath(cache_dir,cache_prefix) << "*";
    DBG( cerr << __func__ << "() - cmd: " << s.str() << endl );
    system( s.str().c_str() );
    DBG( cerr << __func__ << "() - END " << endl);
}

string show_cache(const string cache_dir, const string match_prefix){
    map<string,string> contents ;
    ostringstream oss;
    DIR *dip = opendir( cache_dir.c_str() ) ;
    CPPUNIT_ASSERT( dip ) ;
    struct dirent *dit;
    while( ( dit = readdir( dip ) ) != NULL )
    {
        string dirEntry = dit->d_name ;
        if( dirEntry.compare( 0, match_prefix.length(), match_prefix ) == 0){
            oss << dirEntry << endl;
            contents[dirEntry] = dirEntry ;
        }
    }
    closedir( dip ) ;
    return oss.str();

}

class cacheT: public TestFixture {
private:

public:
    cacheT() {
    }
    ~cacheT() {
    }

    void setUp()
    {
//        DBG( cerr << "-------------------------------------------" << endl);
//        DBG( cerr << __func__ << "() - BEGIN " << endl);

        string bes_conf = (string)TEST_ABS_SRC_DIR + "/cacheT_bes.keys" ;
        TheBESKeys::ConfigFile = bes_conf ;

        if (bes_debug){
            BESDebug::SetUp("DBG( cerr,cache");
            DBG( cerr << "setup() - BESDEBUG Enabled " << endl);
        }

//        DBG( cerr << __func__ << "() - END " << endl);


    } 

    void tearDown()
    {
    }


    // Fill the test directory with files summing > 4Gb.
    // First calls clean in case the thing exists.
    // TODO HACK This really needs to be cleaned up
    // to not use system but instead make the actual
    // C calls to make the files so it can checkl errno
    // and report the problems and make sure the test is
    // valid!
    void init_64_bit_cache(const std::string& cacheDir)
    {
        DBG( cerr << __func__ << "() - BEGIN " << endl);
        DBG( cerr << __func__ << "() - Called with dir = " << cacheDir << endl);
        DBG( cerr << __func__ << "() - Note: this makes large files and might take a while!" << endl);

        // 0) Make sure it's not there
        DBG( cerr << __func__ << "() - Cleaning old test cache..." << endl);
        clean_64_bit_cache(cacheDir);
        DBG( cerr << __func__ << "() -  ... done." << endl);

        BESFileLockingCache cache64(CACHE_DIR_TEST_64,
                                    CACHE_PREFIX,
                                    MAX_CACHE_SIZE_IN_MEGS_TEST_64);


        // 1) make the dir and files.
        string mkdirCmd = string("mkdir -p ") + cacheDir;
        DBG( cerr << __func__ << "() - Shell call: " << mkdirCmd << endl);
        system(mkdirCmd.c_str());

        DBG( cerr << __func__ << "() - Making " << NUM_FILES_TEST_64 << " files..." << endl);
        for (unsigned int i=0; i < NUM_FILES_TEST_64; ++i)
        {
            std::stringstream fss;
            fss << cacheDir << "/" << MATCH_PREFIX <<
                "_file_" << i << ".txt";
            DBG( cerr << __func__ << "() - Creating filename=" << fss.str() <<
                " of size (mb) = " << FILE_SIZE_IN_MEGS_TEST_64 << endl);
            std::stringstream mkfileCmdSS;
            mkfileCmdSS << "mkfile -n " << FILE_SIZE_IN_MEGS_TEST_64 << "m" << " " << fss.str();
            DBG( cerr << __func__ << "() - Shell call: " << mkfileCmdSS.str() << endl);
            system(mkfileCmdSS.str().c_str());
            DBG( cerr << __func__ << "() - " << "Done making file. Updating cache control file..." << endl);
            cache64.update_cache_info(fss.str());
        }
        DBG( cerr << __func__ << "() - " << "END " << endl);
    }

    // Clean out all those giant temp files
    void clean_64_bit_cache(const std::string& cacheDir)
    {
        DBG( cerr << __func__ << "() - BEGIN " << endl);
        std::string rmCmd = string("rm -r") + string(debug?"v":"") + string("f ") + cacheDir;
        DBG( cerr << __func__ << "() - Shell call: " << rmCmd << endl);
        system(rmCmd.c_str());
        DBG( cerr << __func__ << "() - END " << endl);
    }

    // Test function to test the cache on larger than 4G
    // (ie > 32 bit)
    // sum total of files sizes, which was a bug.
    // Since this takes a lot of time and space to do,
    // we'll make it optional somehow...
    void do_test_64_bit_cache()
    {
        DBG( cerr << __func__ << "() - BEGIN " << endl);
        // create a directory with a bunch of large files
        // to init the cache
        init_64_bit_cache(CACHE_DIR_TEST_64);

        // 1) make the cache
        // 2) get the info and make sure size is too big
        // 3) call purge
        // 4) get info again and make sure size is not too big

        // 1) Make the cache
        DBG( cerr <<  __func__ << "() - "
            "Making a BESFileLockingCache with dir=" << CACHE_DIR_TEST_64 <<
            " and prefix=" << CACHE_PREFIX <<
            " and max size (mb)= " << MAX_CACHE_SIZE_IN_MEGS_TEST_64 << endl);


        BESFileLockingCache cache64(CACHE_DIR_TEST_64,
                                    CACHE_PREFIX,
                                    MAX_CACHE_SIZE_IN_MEGS_TEST_64);

        // Make sure we have a valid test dir
        DBG( cerr <<  __func__ << "() - "
            "cache64.get_cache_size: " << cache64.get_cache_size() << endl);

        DBG( cerr <<  __func__ << "() - "
            "cache64.d_max_cache_size_in_bytes: " << cache64.d_max_cache_size_in_bytes << endl);

        DBG( cerr <<  __func__ << "() - "
            "MAX_CACHE_SIZE_IN_BYTES_TEST_64: " << MAX_CACHE_SIZE_IN_BYTES_TEST_64 << endl);

        CPPUNIT_ASSERT(cache64.d_max_cache_size_in_bytes == MAX_CACHE_SIZE_IN_BYTES_TEST_64);

        // Call purge which should delete enough to make it smaller
        DBG( cerr <<  __func__ << "() - Calling purge() on cache..." << endl);
        cache64.update_and_purge("");

        DBG( cerr <<  __func__ << "() - Checking that size is smaller than max..." << endl);
        CPPUNIT_ASSERT(cache64.get_cache_size() <= MAX_CACHE_SIZE_IN_BYTES_TEST_64);

        DBG( cerr << __func__ << "() - Test complete, cleaning test cache dir..." << endl);

        // Delete the massive empty files
        clean_64_bit_cache(CACHE_DIR_TEST_64);
        DBG( cerr << __func__ << "() - END " << endl);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    // CPPUNIT


    void test_empty_cache_dir_name_cache_creation(){
        DBG( cerr << endl << __func__ << "() - BEGIN " << endl);
        try
        {
            // Should return a disabled cache if the dir is "";
            BESFileLockingCache cache( "", CACHE_PREFIX, 0 ) ;
            DBG( cerr << __func__ << "() - Cache is "<< (cache.cache_enabled()?"en":"dis") << "abled." << endl);
            CPPUNIT_ASSERT( !cache.cache_enabled() ) ;

        }
        catch( BESError &e )
        {
            DBG( cerr << __func__ << "Failed to create disabled cache. That's BAD. " <<
                "msg: " << e.get_message() << endl);
        }
        DBG( cerr << __func__ << "() - END " << endl);
    }

    void test_missing_cache_dir_cache_creation(){
        DBG( cerr << endl << __func__ << "() - BEGIN " << endl);
        try
        {
            BESFileLockingCache cache( "/dummy", CACHE_PREFIX, 0 ) ;
            CPPUNIT_ASSERT( !"Created cache with non-existent dir" ) ;
        }
        catch( BESError &e )
        {
            DBG( cerr << __func__ << "() - Unable to create cache in non-existent dir. " <<
                "That's good. msg: " <<  e.get_message() << endl);
        }
        DBG( cerr << __func__ << "() - END " << endl);
    }

    void test_empty_prefix_name_cache_creation(){
        DBG( cerr << endl << __func__ << "() - BEGIN " << endl);
        try
        {
            BESFileLockingCache cache( TEST_CACHE_DIR, "", 1 ) ;
            CPPUNIT_ASSERT( !"Created cache with empty prefix" ) ;
        }
        catch( BESError &e )
        {
            DBG( cerr << __func__ << "() - Unable to create cache with empty prefix. " <<
                "That's good. msg: " <<  e.get_message() << endl);
        }
        DBG( cerr << __func__ << "() - END " << endl);
    }

    void test_size_zero_cache_creation(){
        DBG( cerr << endl << __func__ << "() - BEGIN " << endl);
        try
        {
            BESFileLockingCache cache( TEST_CACHE_DIR, CACHE_PREFIX, 0 ) ;
            CPPUNIT_ASSERT( !"Created cache with 0 size" ) ;
        }
        catch( BESError &e )
        {
            DBG( cerr << __func__ << "() - Unable to create cache with 0 size. " <<
                "That's good. msg: " <<  e.get_message() << endl);
     }
        DBG( cerr << __func__ << "() - END " << endl);
    }

    void test_good_cache_creation(){
        DBG( cerr << endl << __func__ << "() - BEGIN " << endl);
        try
        {
            BESFileLockingCache cache( TEST_CACHE_DIR, CACHE_PREFIX, 1 ) ;
            DBG( cerr << __func__ << "() - Cache is "<< (cache.cache_enabled()?"en":"dis") << "abled." << endl);
            CPPUNIT_ASSERT( cache.cache_enabled() ) ;

        }
        catch( BESError &e )
        {
            DBG( cerr << __func__ << "() - FAILED to create cache! msg: " <<
                e.get_message() << endl );
            CPPUNIT_ASSERT( !"Failed to create cache" ) ;
        }
        DBG( cerr << __func__ << "() - END " << endl);
    }

    void test_check_cache_for_non_existent_compressed_file(){
        DBG( cerr << endl << __func__ << "() - BEGIN " << endl);
        try
        {
            int fd;
            BESFileLockingCache cache( TEST_CACHE_DIR, CACHE_PREFIX, 1 ) ;
            string no_such_file = BESUtil::assemblePath(TEST_CACHE_DIR,"dummy.nc.gz");
            DBG( cerr << __func__ << "() - Attempting to acquire a read lock on "
                "non-existent file." << endl);
            bool success =  cache.get_read_lock(no_such_file,fd);
            DBG( cerr << __func__ << "() - cache.get_read_lock() returned "<<
                (success?"true":"false")<< endl);
            if(success){
                DBG( cerr << __func__ << "() - OUCH! That shouldn't have worked! "
                    "Releasing lock and closing file before we ASSERT... "<<   endl);
                cache.unlock_and_close(no_such_file);
            }
            CPPUNIT_ASSERT( !success ) ;
        }
        catch( BESError &e )
        {
            DBG( cerr << __func__ << "() - FAILED checking if non-exist file cached! msg: " <<
                e.get_message() << endl );
            CPPUNIT_ASSERT( !"Error checking if non-exist file cached" ) ;
        }
        DBG( cerr << __func__ << "() - END " << endl);
    }

    void test_find_exisiting_cached_file(){
        DBG( cerr << endl << __func__ << "() - BEGIN " << endl);
        try
        {
            BESFileLockingCache cache( TEST_CACHE_DIR, CACHE_PREFIX, 1 ) ;
            string file_name ="/usr/local/data/template01.txt";
            string cache_file_name = cache.get_cache_file_name(file_name);
            int fd;

            bool is_it = cache.get_read_lock( cache_file_name, fd ) ;
            DBG( cerr << __func__ << "() - cache.get_read_lock() returned "<<
                (is_it?"true":"false")<< endl);
            CPPUNIT_ASSERT( is_it == true ) ;
        }
        catch( BESError &e )
        {
            DBG( cerr << __func__ << "() - FAILED checking for cached file! msg: " <<
                e.get_message() << endl );
            CPPUNIT_ASSERT( !"Error checking if expected file is cached" ) ;
        }
        DBG( cerr << __func__ << "() - END " << endl);
    }

    void test_cache_purge(){
        DBG( cerr << endl << __func__ << "() - BEGIN " << endl);

        map<string,string> should_be ;
        should_be["bes_cache#usr#local#data#template01.txt"] = "bes_cache#usr#local#data#template01.txt" ;
        should_be["bes_cache#usr#local#data#template03.txt"] = "bes_cache#usr#local#data#template02.txt" ;
        should_be["bes_cache#usr#local#data#template05.txt"] = "bes_cache#usr#local#data#template03.txt" ;
        should_be["bes_cache#usr#local#data#template08.txt"] = "bes_cache#usr#local#data#template04.txt" ;


        string latest_file = "/usr/local/data/template01.txt";

        DBG( cerr<< __func__ << "() - Cache Before update_and_purge():" << endl
            << show_cache(TEST_CACHE_DIR, CACHE_PREFIX) );

        try
        {
            BESFileLockingCache cache( TEST_CACHE_DIR, CACHE_PREFIX, 1 ) ;
            string latest_cache_file =  cache.get_cache_file_name(latest_file);
            cache.update_and_purge(latest_cache_file);
            check_cache(TEST_CACHE_DIR, should_be ) ;
        }
        catch( BESError &e )
        {
            DBG( cerr << __func__ << "() - " << e.get_message() << endl );
            CPPUNIT_ASSERT( !"purge failed" ) ;
        }

        DBG( cerr << __func__ << "() - Test purge (should not remove any files)" << endl);
        try
        {
            BESFileLockingCache cache( TEST_CACHE_DIR, CACHE_PREFIX, 1 ) ;
            string latest_cache_file =  cache.get_cache_file_name(latest_file);
            cache.update_and_purge(latest_cache_file);
            check_cache(TEST_CACHE_DIR, should_be ) ;
        }
        catch( BESError &e )
        {
            DBG( cerr << __func__ << "() - " << e.get_message() << endl) ;
            CPPUNIT_ASSERT( !"purge failed" ) ;
        }
        DBG( cerr << __func__ << "() - END " << endl);
    }


    void test_64_bit_cache_sizes(){
        if (RUN_64_BIT_CACHE_TEST)
        {
            DBG( cerr << endl << __func__ << "() - BEGIN " << endl);
            try
            {
                do_test_64_bit_cache();
            }
            catch (BESError &e)
            {
                DBG( cerr << __func__ << "Caught BESError. msg: " << e.get_message() << endl);
                CPPUNIT_ASSERT( !"64 bit tests failed" );
            }
            DBG( cerr << __func__ << "() - END " << endl);
        }

    }

    CPPUNIT_TEST_SUITE( cacheT ) ;

    CPPUNIT_TEST( test_empty_cache_dir_name_cache_creation ) ;
    CPPUNIT_TEST( test_missing_cache_dir_cache_creation ) ;
    CPPUNIT_TEST( test_size_zero_cache_creation ) ;
    CPPUNIT_TEST( test_good_cache_creation ) ;
    CPPUNIT_TEST( test_check_cache_for_non_existent_compressed_file ) ;
    CPPUNIT_TEST( test_find_exisiting_cached_file ) ;
    CPPUNIT_TEST( test_cache_purge ) ;
    CPPUNIT_TEST( test_64_bit_cache_sizes ) ;

    CPPUNIT_TEST_SUITE_END() ;

#if 0

    void do_test()
    {
        BESKeys *keys = TheBESKeys::TheKeys() ;

        string target ;
        bool is_it = false ;


        DBG( cerr << "*****************************************" << endl);
        DBG( cerr << "creating cache with empty dir key" << endl);
        try
        {
            // BESFileLockingCache cache( "", "", 1 ) ;
            CPPUNIT_ASSERT( !"Created cache with empty dir key" ) ;
        }
        catch( BESError &e )
        {
            DBG( cerr << "Failed to create cache with empty dir key, good" << endl);
            DBG( cerr << e.get_message() << endl);
        }

        DBG( cerr << "*****************************************" << endl);
        DBG( cerr << "creating cache with non-exist dir key" << endl);
        try
        {
            // BESFileLockingCache cache( *keys, "BES.CacheDir", "", "" ) ;
            CPPUNIT_ASSERT( !"Created cache with non-exist dir key" ) ;
        }
        catch( BESError &e )
        {
            DBG( cerr << "Failed to create cache with non-exist dir key, good"
                << endl);
            DBG( cerr << e.get_message() << endl);
        }

        keys->set_key( "BES.CacheDir", "/dummy" ) ;
        DBG( cerr << "*****************************************" << endl);
        DBG( cerr << "creating cache with bad dir in conf" << endl);
        try
        {
            // BESFileLockingCache cache( *keys, "BES.CacheDir", "", "" ) ;
            CPPUNIT_ASSERT( !"Created cache with bad dir in conf" ) ;
        }
        catch( BESError &e )
        {
            DBG( cerr << "Failed to create cache with bad dir in conf, good"
                << endl);
            DBG( cerr << e.get_message() << endl);
        }

        keys->set_key( "BES.CacheDir", cache_dir ) ;
        DBG( cerr << "*****************************************" << endl);
        DBG( cerr << "creating cache with empty prefix key" << endl);
        try
        {
            // BESFileLockingCache cache( *keys, "BES.CacheDir", "", "" ) ;
            CPPUNIT_ASSERT( !"Created cache with empty prefix key" ) ;
        }
        catch( BESError &e )
        {
            DBG( cerr << "Failed to create cache with empty prefix key, good" << endl);
            DBG( cerr << e.get_message() << endl);
        }

        DBG( cerr << "*****************************************" << endl);
        DBG( cerr << "creating cache with non-exist prefix key" << endl);
        try
        {
            // BESFileLockingCache cache( *keys, "BES.CacheDir", "BES.CachePrefix", "" ) ;
            CPPUNIT_ASSERT( !"Created cache with non-exist prefix key" ) ;
        }
        catch( BESError &e )
        {
            DBG( cerr << "Failed to create cache with non-exist prefix key, good"
                << endl);
            DBG( cerr << e.get_message() << endl);
        }

        keys->set_key( "BES.CachePrefix", "" ) ;
        DBG( cerr << "*****************************************" << endl);
        DBG( cerr << "creating cache with empty prefix key in conf" << endl);
        try
        {
            // BESFileLockingCache cache( *keys, "BES.CacheDir", "BES.CachePrefix", "" ) ;
            CPPUNIT_ASSERT( !"Created cache with empty prefix in conf" ) ;
        }
        catch( BESError &e )
        {
            DBG( cerr << "Failed to create cache with empty prefix in conf, good"
                << endl);
            DBG( cerr << e.get_message() << endl);
        }

        keys->set_key( "BES.CachePrefix", "bes_cache" ) ;
        DBG( cerr << "*****************************************" << endl);
        DBG( cerr << "creating cache with empty size key" << endl);
        try
        {
            // BESFileLockingCache cache( *keys, "BES.CacheDir", "BES.CachePrefix", "" ) ;
            CPPUNIT_ASSERT( !"Created cache with empty size key" ) ;
        }
        catch( BESError &e )
        {
            DBG( cerr << "Failed to create cache with empty size key, good"
                << endl);
            DBG( cerr << e.get_message() << endl);
        }

        DBG( cerr << "*****************************************" << endl);
        DBG( cerr << "creating cache with non-exist size key" << endl);
        try
        {
            // BESFileLockingCache cache( *keys, "BES.CacheDir", "BES.CachePrefix", "BES.CacheSize" ) ;
            CPPUNIT_ASSERT( !"Created cache with non-exist size key" ) ;
        }
        catch( BESError &e )
        {
            DBG( cerr << "Failed to create cache with non-exist size key, good"
                << endl);
            DBG( cerr << e.get_message() << endl);
        }

        keys->set_key( "BES.CacheSize", "dummy" ) ;
        DBG( cerr << "*****************************************" << endl);
        DBG( cerr << "creating cache with bad size in conf" << endl);
        try
        {
            // BESFileLockingCache cache( *keys, "BES.CacheDir", "BES.CachePrefix", "BES.CacheSize" ) ;
            CPPUNIT_ASSERT( !"Created cache with bad size in conf" ) ;
        }
        catch( BESError &e )
        {
            DBG( cerr << "Failed to create cache with bad size in conf, good"
                << endl);
            DBG( cerr << e.get_message() << endl);
        }

        keys->set_key( "BES.CacheSize", "0" ) ;
        DBG( cerr << "*****************************************" << endl);
        DBG( cerr << "creating cache with 0 size in conf" << endl);
        try
        {
            // BESFileLockingCache cache( *keys, "BES.CacheDir", "BES.CachePrefix", "BES.CacheSize" ) ;
            CPPUNIT_ASSERT( !"Created cache with 0 size in conf" ) ;
        }
        catch( BESError &e )
        {
            DBG( cerr << "Failed to create cache with 0 size in conf, good"
                << endl);
            DBG( cerr << e.get_message() << endl);
        }

        keys->set_key( "BES.CacheSize", "1" ) ;
        DBG( cerr << "*****************************************" << endl);
        DBG( cerr << "creating good cache from config" << endl);
        try
        {
            // BESFileLockingCache cache( *keys, "BES.CacheDir", "BES.CachePrefix", "BES.CacheSize" ) ;
        }
        catch( BESError &e )
        {
            DBG( cerr << e.get_message() << endl);
            CPPUNIT_ASSERT( !"Failed to create cache with good keys" ) ;
        }
    }

#endif


} ; // test fixture class



CPPUNIT_TEST_SUITE_REGISTRATION( cacheT ) ;


int main(int argc, char*argv[])
{
    GetOpt getopt(argc, argv, "db6");
    int option_char;
    while ((option_char = getopt()) != -1)
        switch (option_char) {
        case 'd':
            debug = true;  // debug is a static global
            break;

        case 'b':
            bes_debug = true;  // bes_debug is a static global
            break;

        case '6':
            RUN_64_BIT_CACHE_TEST = true;  // RUN_64_BIT_CACHE_TEST is a static global
            DBG( cerr << __func__ << "() - 64 Bit Cache Tests Enabled." << endl);
            break;

        default:
            break;
        }

    // Do this AFTER we process the command line so debugging in the test constructor
    // (which does a one time construction of the test cache) will work.

    init_cache(TEST_CACHE_DIR);


    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());


    bool wasSuccessful = true;
    string test = "";
    int i = getopt.optind;
    if (i == argc) {
        // run them all
        wasSuccessful = runner.run("");
    }
    else {
        while (i < argc) {
            test = string("cacheT::") + argv[i++];

            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    purge_cache(TEST_CACHE_DIR, CACHE_PREFIX);

    return wasSuccessful ? 0 : 1;
}

