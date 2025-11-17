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
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

#include <cstdlib>
#include <iostream>
#include <unistd.h>

#include "BESInternalError.h"

#include "BESDebug.h"
#include "BESError.h"
#include "BESUncompressCache.h"
#include "BESUncompressManager3.h"
#include "BESUtil.h"
#include "TheBESKeys.h"
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
#define DBG(x)                                                                                                         \
    do {                                                                                                               \
        if (debug)                                                                                                     \
            (x);                                                                                                       \
    } while (false);

class uncompressT : public TestFixture {
private:
public:
    uncompressT() {}
    ~uncompressT() {}

    int clean_dir(const string &cache_dir, const string &cache_prefix) {
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

    void setUp() {
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
        TheBESKeys::TheKeys()->set_key("BES.UncompressCache.dir", "");
        TheBESKeys::TheKeys()->set_key("BES.UncompressCache.prefix", CACHE_PREFIX);
        TheBESKeys::TheKeys()->set_key("BES.UncompressCache.size", "1");
    }

    void tearDown() {}

    void test_disabled_uncompress_cache() {
        DBG(cerr << __func__ << "() - BEGIN" << endl);
        // Setting the cache_dir parameter to the empty string will disable the cache
        // and cause the get_instance method to return NULL>
        BESUncompressCache *cache = BESUncompressCache::get_instance();
        DBG(cerr << __func__ << "() - cache: " << (void *)cache << endl);

        CPPUNIT_ASSERT_MESSAGE("Cache pointer should be null", !cache);
        DBG(cerr << __func__ << "() - END" << endl);
    }

    CPPUNIT_TEST_SUITE(uncompressT);

    CPPUNIT_TEST(test_disabled_uncompress_cache);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(uncompressT);

int main(int argc, char *argv[]) {
    int option_char;
    while ((option_char = getopt(argc, argv, "dbh")) != -1)
        switch (option_char) {
        case 'd':
            debug = 1; // debug is a static global
            break;
        case 'b':
            bes_debug = 1; // debug is a static global
            break;
        case 'h': { // help - show test names
            cerr << "Usage: uncompressT has the following tests:" << endl;
            const std::vector<Test *> &tests = uncompressT::suite()->getTests();
            unsigned int prefix_len = uncompressT::suite()->getName().append("::").size();
            for (std::vector<Test *>::const_iterator i = tests.begin(), e = tests.end(); i != e; ++i) {
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
    } else {
        int i = 0;
        while (i < argc) {
            if (debug)
                cerr << "Running " << argv[i] << endl;
            test = uncompressT::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
