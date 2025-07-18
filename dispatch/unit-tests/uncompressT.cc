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

#include "config.h"

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <iostream>
#include <cstdlib>
#include <unistd.h>

#include "BESInternalError.h"

#include "BESUncompressManager3.h"
#include "BESUncompressCache.h"
#include "BESError.h"
#include "TheBESKeys.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "test_config.h"

using namespace CppUnit;

using std::cerr;
using std::endl;
using std::ifstream;
using std::string;

static bool debug = false;
static bool bes_debug = false;

static const string CACHE_DIR = BESUtil::assemblePath(TEST_SRC_DIR, "cache");
static const string CACHE_FILE_NAME = BESUtil::assemblePath(CACHE_DIR, "template.txt");
static const string CACHE_PREFIX("container_test");

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

class uncompressT: public TestFixture {
private:

public:
    uncompressT()
    {
    }
    ~uncompressT()
    {
    }

    int clean_dir(const string &cache_dir, const string &cache_prefix)
    {
        DBG(cerr << __func__ << "() - BEGIN " << endl);
        std::ostringstream s;
        s << "rm -" << (debug ? "v" : "") << "f " << BESUtil::assemblePath(cache_dir, cache_prefix) << "*";
        DBG(cerr << __func__ << "() - cmd: " << s.str() << endl);
        int status = system(s.str().c_str());
        DBG(cerr << __func__ << "() - END " << endl);
        return status;
    }

#if 0 // replaced with shell based version above. concurrency. woot. ndp-05/3017
    int clean_dir(string dirname, string prefix)
    {
        DIR *dp;
        struct dirent *dirp;
        if ((dp = opendir(dirname.c_str())) == NULL) {
            DBG(cerr << "Error(" << errno << ") opening " << dirname << endl);
            return errno;
        }

        while ((dirp = readdir(dp)) != NULL) {
            string name(dirp->d_name);
            if (name.find(prefix) == 0) {
                string abs_name = BESUtil::assemblePath(dirname, name, true);
                DBG(cerr << "Purging file: " << abs_name << endl);
                remove(abs_name.c_str());
            }

        }

        closedir(dp);
        return 0;
    }
#endif


    void setUp()
    {
#if 0
        string bes_conf = (string) TEST_SRC_DIR + "/uncompressT_bes.keys";
        TheBESKeys::ConfigFile = bes_conf;
#endif
        if (bes_debug) {
            BESDebug::SetUp("cerr,cache,uncompress,uncompress2");
            DBG(cerr << "setup() - BESDEBUG Enabled " << endl);
        }
        TheBESKeys::TheKeys()->set_key("BES.Uncompress.Retry", "2");
        TheBESKeys::TheKeys()->set_key("BES.Uncompress.NumTries", "10");
    }

    void tearDown()
    {
    }

    void test_worker(string cache_prefix, string test_file_base, string test_file_suffix)
    {
        DBG(cerr << __func__ << "() - BEGIN" << endl);

        TheBESKeys::TheKeys()->set_key("BES.UncompressCache.dir", CACHE_DIR);
        TheBESKeys::TheKeys()->set_key("BES.UncompressCache.prefix", CACHE_PREFIX);
        TheBESKeys::TheKeys()->set_key("BES.UncompressCache.size", "1");

        DBG(cerr << __func__ << "() - cache_prefix: " << cache_prefix << endl);

        string cache_dir = (string) TEST_SRC_DIR + "/cache";
        DBG(cerr << __func__ << "() - cache_dir: " << cache_dir << endl);

        // Clean it up...
        clean_dir(cache_dir, cache_prefix);

        string src_file_base = cache_dir + test_file_base;
        string src_file = src_file_base + test_file_suffix;

        // we're not testing the caching mechanism, so just create it, but make
        // sure it gets created.
        try {
            BESUncompressCache *cache = BESUncompressCache::get_instance();

            // get the target name and make sure the target file doesn't exist
            string cache_file_name = cache->get_cache_file_name(src_file_base, false);
            if (cache->is_valid(cache_file_name, src_file)) cache->purge_file(cache_file_name);

            DBG(cerr << __func__ << "() - *****************************************" << endl);
            DBG(cerr << __func__ << "() - uncompress a test " << test_file_suffix << " file" << endl);
            try {
                string result;
                bool cached = BESUncompressManager3::TheManager()->uncompress(src_file, result, cache);
                CPPUNIT_ASSERT( cached );
                DBG(cerr << __func__ << "() - Cache file expected: " << cache_file_name << endl);
                DBG(cerr << __func__ << "() -   Cache file result: " << result << endl);
                CPPUNIT_ASSERT( result == cache_file_name );

                ifstream strm(cache_file_name.c_str());
                CPPUNIT_ASSERT( strm );

                char line[80];
                strm.getline((char *) line, 80);
                string sline = line;
                string should_be = "This is a test of a compression method.";
                DBG(cerr << __func__ << "() - expected contents = " << should_be << endl);
                DBG(cerr << __func__ << "() -   result contents = " << sline << endl);
                CPPUNIT_ASSERT( sline == should_be );
            }
            catch (BESError &e) {
                DBG(cerr << __func__ << "() - Caught BESError. msg: " << e.get_message() << endl);
                CPPUNIT_ASSERT( !"Failed to uncompress the gz file" );
            }

            string tmp;
            CPPUNIT_ASSERT( cache->is_valid(cache_file_name,src_file) );

            DBG(cerr << __func__ << "() - Uncompress a test " << test_file_suffix << " file, should be cached" << endl);
            try {
                string result;
                bool cached = BESUncompressManager3::TheManager()->uncompress(src_file, result, cache);
                CPPUNIT_ASSERT( cached );
                CPPUNIT_ASSERT( result == cache_file_name );

                ifstream strm(cache_file_name.c_str());
                CPPUNIT_ASSERT( strm );

                char line[80];
                strm.getline((char *) line, 80);
                string sline = line;
                string should_be = "This is a test of a compression method.";
                DBG(cerr << __func__ << "() - expected contents = " << should_be << endl);
                DBG(cerr << __func__ << "() -   result contents = " << sline << endl);
                CPPUNIT_ASSERT( sline == should_be );
            }
            catch (const BESError &e) {
                DBG(cerr << e.get_message() << endl);
                CPPUNIT_FAIL( "Failed to uncompress the file" );
            }

            CPPUNIT_ASSERT( cache->is_valid(cache_file_name, src_file) );
        }
        catch (const BESError &e) {
            DBG(cerr << __func__ << "() - Caught BESError. Message: " << e.get_message() << endl);
            CPPUNIT_FAIL( "Unable to create the required cache object." );
        }

        clean_dir(cache_dir, cache_prefix);
        DBG(cerr << __func__ << "() - END" << endl);

    }

    void test_disabled_uncompress_cache()
    {
        DBG(cerr << __func__ << "() - BEGIN" << endl);
        // Setting the cache_dir parameter to the empty string will disable the cache
        // and cause the get_instance method to return NULL>
        BESUncompressCache *cache = BESUncompressCache::get_instance();
        DBG(cerr << __func__ << "() - cache: " << (void * )cache << endl);
        CPPUNIT_FAIL( "Should of thrown BESInternalError" );
        DBG(cerr << __func__ << "() - END" << endl);
    }

    void gz_test()
    {
        DBG(cerr << __func__ << "() - BEGIN" << endl);
        string cache_prefix = "zcache";
        string test_file_base = "/testfile.txt";
        string test_file_suffix = ".gz";

        test_worker(cache_prefix, test_file_base, test_file_suffix);
        DBG(cerr << __func__ << "() - END" << endl);
    }

    void Z_test()
    {
        DBG(cerr << __func__ << "() - BEGIN" << endl);
        string cache_prefix = "zcache";
        string test_file_base = "/testfile.txt";
        string test_file_suffix = ".Z";

        test_worker(cache_prefix, test_file_base, test_file_suffix);
        DBG(cerr << __func__ << "() - END" << endl);
    }

    void libz2_test()
    {
#ifdef HAVE_LIBBZ2
        DBG(cerr << __func__ << "() - BEGIN" << endl);
        string cache_prefix = "zcache";
        string test_file_base = "/testfile.txt";
        string test_file_suffix = ".bz2";

        test_worker(cache_prefix, test_file_base, test_file_suffix);
        DBG(cerr << __func__ << "() - END" << endl);
#endif
    }

    CPPUNIT_TEST_SUITE( uncompressT );
    //CPPUNIT_TEST( test_disabled_uncompress_cache );
    CPPUNIT_TEST_EXCEPTION( test_disabled_uncompress_cache, BESInternalError );
    CPPUNIT_TEST( gz_test );
    CPPUNIT_TEST( libz2_test );
    CPPUNIT_TEST( Z_test );

    CPPUNIT_TEST_SUITE_END();

};

CPPUNIT_TEST_SUITE_REGISTRATION( uncompressT );

int main(int argc, char*argv[])
{
    int option_char;
    while ((option_char = getopt(argc, argv, "dbh")) != -1)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;
        case 'b':
            bes_debug = 1;  // debug is a static global
            break;
        case 'h': {     // help - show test names
            cerr << "Usage: uncompressT has the following tests:" << endl;
            const std::vector<Test*> &tests = uncompressT::suite()->getTests();
            unsigned int prefix_len = uncompressT::suite()->getName().append("::").size();
            for (std::vector<Test*>::const_iterator i = tests.begin(), e = tests.end(); i != e; ++i) {
                cerr << (*i)->getName().replace(0, prefix_len, "") << endl;
            }
            break;
        }
        default:
            break;
        }

    argc -= optind;
    argv += optind;

    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    bool wasSuccessful = true;
    string test = "";
    if (0 == argc) {
        // run them all
        wasSuccessful = runner.run("");
    }
    else {
        int i = 0;
        while (i < argc) {
            if (debug) cerr << "Running " << argv[i] << endl;
            test = uncompressT::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}

