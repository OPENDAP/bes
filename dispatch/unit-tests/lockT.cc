// lockT.C

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

using namespace CppUnit;

#include <cerrno>
#include <iostream>
#include <cstdlib>
#include <dirent.h>
#include <GetOpt.h>

using std::cerr;
using std::cerr;
using std::endl;

#include "BESFileLockingCache.h"
#include "BESError.h"
#include "BESDebug.h"
#include "TheBESKeys.h"
#include "BESUtil.h"
#include <test_config.h>

static bool debug = false;
static bool bes_debug = false;
#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

static const string CACHE_DIR = BESUtil::assemblePath(TEST_SRC_DIR, "cache");
static const string CACHE_FILE_NAME = BESUtil::assemblePath(CACHE_DIR, "template.txt");
static const string CACHE_PREFIX("lock_test");

int clean_dir(string dirname, string prefix)
{
    DBG(cerr << endl << __func__ << "() - BEGIN" << endl);
    DBG(cerr << __func__ << "() - dirname: " << dirname << "; prefix: " << prefix << endl);

    DIR *dp;
    struct dirent *dirp;
    if ((dp = opendir(dirname.c_str())) == NULL) {
        DBG(cerr << __func__ << "() - Error(" << errno << ") opening " << dirname << endl);
        return errno;
    }

    while ((dirp = readdir(dp)) != NULL) {
        string name(dirp->d_name);
        if (name.find(prefix) == 0) {
            string abs_name = BESUtil::assemblePath(dirname, name, true);
            DBG(cerr << __func__ << "() - Purging file: " << abs_name << endl);
            remove(abs_name.c_str());
        }

    }

    closedir(dp);

    DBG(cerr << __func__ << "() - END" << endl);
    return 0;
}

class lockT: public TestFixture {
private:

public:
    lockT()
    {
    }
    ~lockT()
    {
    }

    void setUp()
    {
        string bes_conf = (string) TEST_SRC_DIR + "/bes.conf";
        TheBESKeys::ConfigFile = bes_conf;

        if (bes_debug) {
            BESDebug::SetUp("cerr,cache,cache2");
            DBG(cerr << __func__ << "() - setup() - BESDEBUG Enabled " << endl);
        }
    }

    void tearDown()
    {
        clean_dir(CACHE_DIR, CACHE_PREFIX);
    }

    void test_get_2_exlocks()
    {
        DBG(cerr << endl << __func__ << "() - BEGIN" << endl);
        DBG(cerr << __func__ << "() - ExLock a file, then try to exLock it again" << endl);

        try {
            int fd1, fd2;
            BESFileLockingCache cache(CACHE_DIR, CACHE_PREFIX, 1);
            CPPUNIT_ASSERT(cache.cache_enabled());

            DBG(cerr << __func__ << "() - Created cache." << endl);

            try {
                CPPUNIT_ASSERT( cache.getExclusiveLock(CACHE_FILE_NAME, fd1) );
                DBG(cerr << __func__ << "() - Got first lock: fd1: " << fd1 << endl);
            }
            catch (BESError &e) {
                DBG(cerr << __func__ << "() - Caught BESError message: " << e.get_message() << endl);
                cache.unlock_and_close(CACHE_FILE_NAME);
                CPPUNIT_FAIL( "Locking test failed" );
            }

            try {
                CPPUNIT_ASSERT( cache.getExclusiveLock(CACHE_FILE_NAME, fd2) );
                DBG(cerr << __func__ << "() - Got second lock. fd2: " << fd2 << endl);

            }
            catch (BESError &e) {
                DBG(cerr << __func__ << "() - Failed to get lock! Caught BESError message: " << e.get_message() << endl);
                CPPUNIT_FAIL( "Locking test failed" );
            }

            cache.unlock_and_close(CACHE_FILE_NAME);
            DBG(cerr << __func__ << "() - Unlocked file." << endl);
        }
        catch (BESError &e) {
            DBG(cerr << __func__ << "() - Caught BESError message: " << e.get_message() << endl);
            CPPUNIT_FAIL( "Failed to use the cache" );
        }

        DBG(cerr << __func__ << "() - END" << endl);
    }

    void test_two_cache_objects()
    {
        DBG(cerr << endl << __func__ << "() - BEGIN" << endl);

        try {
            int fd_0;
            BESFileLockingCache cache_1(CACHE_DIR, CACHE_PREFIX, 1);
            CPPUNIT_ASSERT(cache_1.cache_enabled());
            DBG(cerr << __func__ << "() - Created cache_1: " << (void * )&cache_1 << endl);

            try {
                CPPUNIT_ASSERT( cache_1.getExclusiveLock(CACHE_FILE_NAME,fd_0) );
                DBG(cerr << __func__ << "() - Got first lock" << endl);
            }
            catch (BESError &e) {
                DBG(cerr << __func__ << "() - Caught BESError message: " << e.get_message() << endl);
                cache_1.unlock_and_close(CACHE_FILE_NAME);
                CPPUNIT_ASSERT( !"Locking test failed" );
            }

            BESFileLockingCache cache_2(CACHE_DIR, CACHE_PREFIX, 1);
            CPPUNIT_ASSERT(cache_2.cache_enabled());
            DBG(cerr << __func__ << "() - Created cache_2: " << (void * )&cache_2 << endl);
            try {
                CPPUNIT_ASSERT( cache_2.getExclusiveLock(CACHE_FILE_NAME,fd_0) );
                DBG(cerr << __func__ << "() - Locked 2nd cache" << endl);
            }
            catch (BESError &e) {
                DBG(cerr << e.get_message() << endl);
                cache_2.unlock_and_close(CACHE_FILE_NAME);
                CPPUNIT_ASSERT( !"cache 2 locking failed" );
            }

            DBG(cerr << __func__ << "() - "
                "Unlocking the first cache" << endl);
            cache_1.unlock_and_close(CACHE_FILE_NAME);

            DBG(cerr << __func__ << "() - "
                "Locking the second cache" << endl);
            try {
                CPPUNIT_ASSERT( cache_2.getExclusiveLock(CACHE_FILE_NAME,fd_0) );
                DBG(cerr << __func__ << "() - Locked second cache." << endl);
            }
            catch (BESError &e) {
                DBG(cerr << __func__ << "() - Caught BESError message: " << e.get_message() << endl);
                cache_2.unlock_and_close(CACHE_FILE_NAME);
                CPPUNIT_ASSERT( !"locking second cache failed" );
            }

            DBG(cerr << __func__ << "() - "
                "Unlock the second cache" << endl);
            cache_2.unlock_and_close(CACHE_FILE_NAME);
        }
        catch (BESError &e) {
            DBG(cerr << __func__ << "() - Caught BESError message: " << e.get_message() << endl);
            CPPUNIT_ASSERT( !"Failed to use the cache" );
        }

        DBG(cerr << __func__ << "() - END" << endl);
    }

CPPUNIT_TEST_SUITE( lockT );

    CPPUNIT_TEST( test_get_2_exlocks );
    CPPUNIT_TEST( test_two_cache_objects );

    CPPUNIT_TEST_SUITE_END()
    ;

};

CPPUNIT_TEST_SUITE_REGISTRATION( lockT );

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

        default:
            break;
        }

    // Do this AFTER we process the command line so debugging in the test constructor
    // (which does a one time construction of the test cache) will work.

    // init_cache(TEST_CACHE_DIR);

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
            test = string("lockT::") + argv[i++];

            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
