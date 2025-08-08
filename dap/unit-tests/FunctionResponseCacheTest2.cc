// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include "config.h"

#ifdef HAVE_TR1_FUNCTIONAL
#include <tr1/functional>
#endif

#ifdef HAVE_TR1_FUNCTIONAL
#define HASH_OBJ std::tr1::hash
#else
#define HASH_OBJ std::hash
#endif

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>
#include <unistd.h>

#include <libdap/Array.h>
#include <libdap/Byte.h>
#include <libdap/ServerFunctionsList.h>
#include <libdap/ConstraintEvaluator.h>
#include <libdap/DAS.h>
#include <libdap/DDS.h>
#include <libdap/DDXParserSAX2.h>

#include <libdap/util.h>
#include <libdap/debug.h>

#include <test/TestTypeFactory.h>

#include "BESDapFunctionResponseCache.h"
#include "BESRegex.h"
#include "BESError.h"
#include "TheBESKeys.h"
#include "BESDebug.h"

#include "TestFunction.h"
#include "test_utils.h"
#include "test_config.h"

using namespace CppUnit;
using namespace std;

int test_variable_sleep_interval = 0;

static bool debug = false;
static bool bes_debug = false;
static bool clean = true;
static const string c_cache_name = "/response_cache";

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

using namespace libdap;
#if 0
const Type requested_type = dods_byte_c;
const int num_dim = 2;
const int dim_sz = 3;
#endif

// Move this into the class when we goto C++-11
const string d_mds_prefix = "rc"; // used when cleaning the cache, etc.

#if 0
/**
 * Server function used by the ConstraintEvaluator. This is needed because passing
 * the CE a null expression or one that names an non-existent function is an error.
 *
 * The test harness code must load up the ServerFunctionList instance - see also
 * TestFunction.h
 */
void TestFunction::function_dap2_test(int argc, libdap::BaseType *argv[], libdap::DDS &, libdap::BaseType **btpp)
{
    if (argc != 1) {
        throw Error(malformed_expr, "test(name) requires one argument.");
    }

    std::string name = extract_string_argument(argv[0]);

    Array *dest = new Array(name, 0);   // The ctor for Array copies the prototype pointer...
    BaseTypeFactory btf;
    dest->add_var_nocopy(btf.NewVariable(requested_type));  // ... so use add_var_nocopy() to add it instead

    vector<int> dims(num_dim, dim_sz);
    //dims.push_back(3); dims.push_back(3);
    unsigned long elem = 1;
    vector<int>::iterator i = dims.begin();
    while (i != dims.end()) {
        elem *= *i;
        dest->append_dim(*i++);
    }

    // stuff the array with values
    vector<dods_byte> values(elem);
    for (unsigned int i = 0; i < elem; ++i) {
        values[i] = i;
    }

    dest->set_value(values, elem);

    dest->set_send_p(true);
    dest->set_read_p(true);

    // return the array
    *btpp = dest;
}
#endif

class FunctionResponseCacheTest: public TestFixture {
private:
    DDXParser dp;
    TestTypeFactory ttf;
    DDS *test_dds;

    ConstraintEvaluator eval;

    string d_cache;
    BESDapFunctionResponseCache *cache;

public:
    FunctionResponseCacheTest() :
        dp(&ttf), test_dds(0), d_cache(string(TEST_BUILD_DIR) + c_cache_name), cache(0)
    {
        libdap::ServerFunctionsList::TheList()->add_function(new TestFunction());
    }

    ~FunctionResponseCacheTest()
    {
    }

    void setUp()
    {
        DBG(cerr << "setUp() - BEGIN" << endl);
        if (bes_debug) BESDebug::SetUp("cerr,response_cache");

        string cid; // This is an unused value-result parameter. jhrg 5/10/16
        test_dds = new DDS(&ttf);
        dp.intern((string) TEST_SRC_DIR + "/input-files/test.05.ddx", test_dds, cid);

        // for these tests, set the filename to the dataset_name. ...keeps the cache names short
        test_dds->filename(test_dds->get_dataset_name());

        DBG(cerr << "DDS Name: " << test_dds->get_dataset_name() << endl);

        if (clean) clean_cache_dir(d_cache);

        TheBESKeys::ConfigFile = (string) TEST_SRC_DIR + "/input-files/test.keys"; // empty file. jhrg 10/20/15
        TheBESKeys::TheKeys()->set_key(BESDapFunctionResponseCache::PATH_KEY, "");
        TheBESKeys::TheKeys()->set_key(BESDapFunctionResponseCache::PREFIX_KEY, d_mds_prefix);
        TheBESKeys::TheKeys()->set_key(BESDapFunctionResponseCache::SIZE_KEY, "1000");

        DBG(cerr << "setUp() - END" << endl);
    }

    void tearDown()
    {
        DBG(cerr << "tearDown() - BEGIN" << endl);

        delete test_dds;

        if (clean) clean_cache_dir(d_cache);

        DBG(cerr << "tearDown() - END" << endl);
    }

    // The directory 'never' does not exist; the cache won't be initialized,
    // so is_available() should be false
    void ctor_test_1()
    {
        DBG(cerr << "ctor_test_1() - BEGIN" << endl);

        cache = BESDapFunctionResponseCache::get_instance();
        DBG(cerr << "ctor_test_1() - retrieved BESDapFunctionResponseCache instance: " << cache << endl);

        CPPUNIT_ASSERT_MESSAGE("Cache pointer should be null", !cache);

        DBG(cerr << "ctor_test_1() - END" << endl);
    }

CPPUNIT_TEST_SUITE( FunctionResponseCacheTest );

    CPPUNIT_TEST(ctor_test_1);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(FunctionResponseCacheTest);

int main(int argc, char*argv[])
{
    int option_char;
    while ((option_char = getopt(argc, argv, "dbkh")) != -1)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;
        case 'b':
            bes_debug = true;  // bes_debug is a static global
            cerr << "##### BES DEBUG is ON" << endl;
            break;
        case 'k':   // -k turns off cleaning the response_cache dir
            clean = false;
            break;
        case 'h': {     // help - show test names
            cerr << "Usage: FunctionResponseCacheTest has the following tests:" << endl;
            const std::vector<Test*> &tests = FunctionResponseCacheTest::suite()->getTests();
            unsigned int prefix_len = FunctionResponseCacheTest::suite()->getName().append("::").size();
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
            test = FunctionResponseCacheTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
	    ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
