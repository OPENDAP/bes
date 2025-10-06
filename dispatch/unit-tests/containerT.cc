// containerT.C

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

#include "modules/common/run_tests_cppunit.h"

#include "TheBESKeys.h"
#include "BESContainerStorageList.h"
#include "BESFileContainer.h"
#include "BESContainerStorageFile.h"
#include "BESUncompressCache.h"
#include "BESError.h"
#include "BESUtil.h"
#include "BESDebug.h"
#include "test_config.h"

using namespace std;
using namespace CppUnit;

#if 0
static bool debug = false;
static bool bes_debug = false;
#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);
#endif

static const string CACHE_DIR = BESUtil::assemblePath(TEST_SRC_DIR, "cache");
static const string CACHE_FILE_NAME = BESUtil::assemblePath(CACHE_DIR, "template.txt");
static const string CACHE_PREFIX("container_test");

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

class containerT: public TestFixture {
private:

public:
    containerT() = default;

    ~containerT() override = default;

    void setUp() override
    {
        string bes_conf = (string) TEST_SRC_DIR + "/empty.ini";
        TheBESKeys::ConfigFile = bes_conf;
    }

    void tearDown() override
    {
        clean_dir(CACHE_DIR, CACHE_PREFIX);
    }

    void test_default_cannot_find()
    {
        DBG(cerr << endl << __func__ << "() - BEGIN" << endl);
        try {
            string key = (string) "BES.Container.Persistence.File.TheFile=" +
            TEST_SRC_DIR + "/container01.file";
            TheBESKeys::TheKeys()->set_key(key);
            BESContainerStorageList::TheList()->add_persistence(new BESContainerStorageFile("TheFile"));
        }
        catch (BESError &e) {
            DBG(cerr << __func__ << "() - Caught BESError, message: " << e.get_message() << endl);
            CPPUNIT_ASSERT(!"Failed to add storage persistence");
        }

        DBG(cerr << __func__ << "() - try to find symbolic name that doesn't exist, default" << endl);
        try {
            std::unique_ptr<BESContainer> c(BESContainerStorageList::TheList()->look_for("nosym"));
            if (c) {
                DBG(cerr << __func__ << "() - container is valid, should not be" << endl);
                DBG(cerr << __func__ << "() -  real_name = " << c->get_real_name() << endl);
                DBG(cerr << __func__ << "() -  constraint = " << c->get_constraint() << endl);
                DBG(cerr << __func__ << "() -  sym_name = " << c->get_symbolic_name() << endl);
                DBG(cerr << __func__ << "() -  container type = " << c->get_container_type() << endl);
            }
            CPPUNIT_ASSERT(!"Found nosym, shouldn't have");
        }
        catch (BESError &e) {
            DBG(cerr << __func__ << "() - caught exception, didn't find nosym, That's good!" << endl);
            DBG(cerr << __func__ << "() - Caught BESError, message: " << e.get_message() << endl);
        }
        DBG(cerr << __func__ << "() - END" << endl);
    }

    void test_default_can_find()
    {
        DBG(cerr << endl << __func__ << "() - BEGIN" << endl);
        DBG(cerr << __func__ << "() - try to find symbolic name that does exist, default" << endl);
        try {
            std::unique_ptr<BESContainer> c(BESContainerStorageList::TheList()->look_for("sym1"));
            CPPUNIT_ASSERT(c);
            CPPUNIT_ASSERT(c->get_symbolic_name() == "sym1");
            CPPUNIT_ASSERT(c->get_real_name() == "real1");
            CPPUNIT_ASSERT(c->get_container_type() == "type1");
        }
        catch (BESError &e) {
            DBG(cerr << __func__ << "() - Caught BESError, message: " << e.get_message() << endl);
            CPPUNIT_ASSERT(!"Failed to find container sym1");
        }
        DBG(cerr << __func__ << "() - END" << endl);
    }
    void test_strict_cannot_find()
    {
        DBG(cerr << endl << __func__ << "() - BEGIN" << endl);
        DBG(cerr << __func__ << "() - set to strict" << endl);
        TheBESKeys::TheKeys()->set_key("BES.Container.Persistence=strict");

        DBG(cerr << __func__ << "() - try to find symbolic name that doesn't exist, strict" << endl);
        try {
            BESContainer *c = BESContainerStorageList::TheList()->look_for("nosym");
            if (c) {
                DBG(cerr << __func__ << "() - Found nosym, shouldn't have" << endl);
                DBG(cerr << __func__ << "() -  real_name = " << c->get_real_name() << endl);
                DBG(cerr << __func__ << "() -  constraint = " << c->get_constraint() << endl);
                DBG(cerr << __func__ << "() -  sym_name = " << c->get_symbolic_name() << endl);
                DBG(cerr << __func__ << "() -  container type = " << c->get_container_type() << endl);
                CPPUNIT_ASSERT(!"Found nosym, shouldn't have");
            }
            else {
                CPPUNIT_ASSERT(!"look_for returned null, should have thrown");
            }
        }
        catch (BESError &e) {
            DBG(cerr << __func__ << "() - caught exception, didn't find nosym, good" << endl);
            DBG(cerr << __func__ << "() - Caught BESError, message: " << e.get_message() << endl);
        }
        DBG(cerr << __func__ << "() - END" << endl);
    }

    void test_strict_can_find()
    {
        DBG(cerr << endl << __func__ << "() - BEGIN" << endl);
        DBG(cerr << __func__ << "() - try to find symbolic name that does exist, strict" << endl);
        try {
            std::unique_ptr<BESContainer> c(BESContainerStorageList::TheList()->look_for("sym1"));
            CPPUNIT_ASSERT(c);
            CPPUNIT_ASSERT(c->get_symbolic_name() == "sym1");
            CPPUNIT_ASSERT(c->get_real_name() == "real1");
            CPPUNIT_ASSERT(c->get_container_type() == "type1");
        }
        catch (BESError &e) {
            DBG(cerr << __func__ << "() - Caught BESError, message: " << e.get_message() << endl);
            CPPUNIT_ASSERT(!"Failed to find container sym1");
        }
        DBG(cerr << __func__ << "() - END" << endl);
    }

    void test_nice_cannot_find()
    {
        DBG(cerr << endl << __func__ << "() - BEGIN" << endl);
        DBG(cerr << __func__ << "() - set to nice" << endl);
        TheBESKeys::TheKeys()->set_key("BES.Container.Persistence=nice");

        DBG(cerr << __func__ << "() - try to find symbolic name that doesn't exist, nice" << endl);
        try {
            std::unique_ptr<BESContainer> c(BESContainerStorageList::TheList()->look_for("nosym"));
            if (c) {
                DBG(cerr << __func__ << "() -  real_name = " << c->get_real_name() << endl);
                DBG(cerr << __func__ << "() -  constraint = " << c->get_constraint() << endl);
                DBG(cerr << __func__ << "() -  sym_name = " << c->get_symbolic_name() << endl);
                DBG(cerr << __func__ << "() -  container type = " << c->get_container_type() << endl);
                CPPUNIT_ASSERT(!"Found nosym, shouldn't have");
            }
        }
        catch (BESError &e) {
            DBG(cerr << __func__ << "() - Caught BESError, message: " << e.get_message() << endl);
            CPPUNIT_ASSERT(!"Exception thrown in nice mode");
        }
        DBG(cerr << __func__ << "() - END" << endl);
    }
    void test_nice_can_find()
    {
        DBG(cerr << endl << __func__ << "() - BEGIN" << endl);
        DBG(cerr << __func__ << "() - set to nice" << endl);
        TheBESKeys::TheKeys()->set_key("BES.Container.Persistence=nice");

        DBG(cerr << __func__ << "() - try to find symbolic name that does exist, nice" << endl);
        try {
            std::unique_ptr<BESContainer> c(BESContainerStorageList::TheList()->look_for("sym1"));
            CPPUNIT_ASSERT(c);
            CPPUNIT_ASSERT(c->get_symbolic_name() == "sym1");
            CPPUNIT_ASSERT(c->get_real_name() == "real1");
            CPPUNIT_ASSERT(c->get_container_type() == "type1");
        }
        catch (BESError &e) {
            DBG(cerr << __func__ << "() - Caught BESError, message: " << e.get_message() << endl);
            CPPUNIT_ASSERT(!"Failed to find container sym1");
        }
        DBG(cerr << __func__ << "() - END" << endl);
    }

    void test_forbidden_path_components()
    {

    }

    void test_compressed()
    {
        DBG(cerr << endl << __func__ << "() - BEGIN" << endl);

        /* Because of the nature of the build system sometimes the cache
         * directory will contain ../, which is not allowed for a containers
         * real name (for files). So this test will be different when just doing
         * a make check or a make distcheck
         */
        bool isdotdot = false;
        string::size_type dotdot = CACHE_DIR.find("../");
        if (dotdot != string::npos) isdotdot = true;

        string src_file = CACHE_DIR + "/testfile.txt";
        string com_file = CACHE_DIR + "/testfile.txt.gz";

        TheBESKeys::TheKeys()->set_key("BES.UncompressCache.dir", CACHE_DIR);
        TheBESKeys::TheKeys()->set_key("BES.UncompressCache.prefix", CACHE_PREFIX);
        TheBESKeys::TheKeys()->set_key("BES.UncompressCache.size", "1");

        string chmod = (string) "chmod a+w " + CACHE_DIR;
        system(chmod.c_str());

        DBG(cerr << __func__ << "() - access a non compressed file" << endl);
        if (!isdotdot) {
            try {
                BESFileContainer c("sym", src_file, "txt");

                string result = c.access();
                CPPUNIT_ASSERT(result == src_file);
            }
            catch (BESError &e) {
                DBG(cerr << __func__ << "() - Caught BESError, message: " << e.get_message() << endl);
                CPPUNIT_ASSERT(!"Failed to access non compressed file");
            }
        }
        else {
            try {
                BESFileContainer c("sym", src_file, "txt");

                string result = c.access();
                CPPUNIT_ASSERT(result != src_file);
            }
            catch (BESError &e) {
                DBG(cerr << __func__ << "() - Failed to access file with ../ in name. That's Good!" << endl);
                DBG(cerr << __func__ << "() - Caught BESError, message: " << e.get_message() << endl);
            }
        }

        DBG(cerr << __func__ << "() - access a compressed file" << endl);
        if (!isdotdot) {
            try {
                BESUncompressCache *cache = BESUncompressCache::get_instance();

                string cache_file_name = cache->get_cache_file_name(com_file);
                ifstream f(cache_file_name.c_str());
                if (f.good()) {
                    cache->purge_file(cache_file_name);
                }

                BESFileContainer c("sym", com_file, "txt");

                string result = c.access();
                DBG(cerr << __func__ << "() - result file name =         " << result << endl);
                DBG(cerr << __func__ << "() - expected cache_file_name = " << cache_file_name << endl);
                CPPUNIT_ASSERT(result == cache_file_name);

                int fd;

                CPPUNIT_ASSERT(cache->get_read_lock(cache_file_name, fd));
            }
            catch (BESError &e) {
                DBG(cerr << __func__ << "() - Caught BESError, message: " << e.get_message() << endl);
                CPPUNIT_ASSERT(!"Failed to access compressed file");
            }
        }
        else {
            try {
                BESUncompressCache *cache = BESUncompressCache::get_instance();

                string cache_file_name = cache->get_cache_file_name(com_file);
                ifstream f(cache_file_name.c_str());
                if (f.good()) {
                    cache->purge_file(cache_file_name);
                }

                BESFileContainer c("sym", com_file, "txt");

                string result = c.access();
                CPPUNIT_ASSERT(result != cache_file_name);
            }
            catch (BESError &e) {
                DBG(cerr << __func__ << "() - Failed to access file with ../ in name, That's Good!" << endl);
                DBG(cerr << __func__ << "() - Caught BESError, message: " << e.get_message() << endl);
            }
        }

        DBG(cerr << __func__ << "() - END" << endl);
    }

    CPPUNIT_TEST_SUITE( containerT );

    CPPUNIT_TEST(test_default_cannot_find);
    CPPUNIT_TEST(test_default_can_find);
    CPPUNIT_TEST(test_strict_cannot_find);
    CPPUNIT_TEST(test_strict_can_find);
    CPPUNIT_TEST(test_nice_cannot_find);
    CPPUNIT_TEST(test_nice_can_find);
    CPPUNIT_TEST(test_compressed);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(containerT);

int main(const int argc, char*argv[])
{
    const bool success = bes_run_tests<containerT>(argc, argv, "cache,cache2"); // Using empty context string

    // Return 0 for success (true from bes_run_tests), 1 for failure (false).
    return success ? 0 : 1;
}

#if 0
int option_char;
    while ((option_char = getopt(argc, argv, "dbh")) != -1)
        switch (option_char) {
        case 'd':
            debug = true;  // debug is a static global
            break;

        case 'b':
            bes_debug = true;  // bes_debug is a static global
            break;

        case 'h': {     // help - show test names
            cerr << "Usage: containerT has the following tests:" << endl;
            const std::vector<Test*> &tests = containerT::suite()->getTests();
            unsigned int prefix_len = containerT::suite()->getName().append("::").size();
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

    // Do this AFTER we process the command line so debugging in the test constructor
    // (which does a one time construction of the test cache) will work.

    // init_cache(TEST_CACHE_DIR);

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
            test = containerT::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
#endif
