// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2018 OPeNDAP, Inc.
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

#include <memory>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <GetOpt.h>
#include <util.h>
#include <debug.h>

#include "BESContextManager.h"
#include "BESError.h"
#include "BESDebug.h"
#include "TheBESKeys.h"

#include "CredentialsManager.h"

#include "test_config.h"

using namespace libdap;

static bool debug = false;
static bool bes_debug = false;

#undef DBG
#define DBG(x) do { if (debug) x; } while(false)

namespace dmrpp {

class CredentialsManagerTest: public CppUnit::TestFixture {
private:
    string cm_config;
    string weak_config;

public:
    // Called once before everything gets tested
    CredentialsManagerTest()
    {
    }

    // Called at the end of the test
    ~CredentialsManagerTest()
    {
    }

    // Called before each test
    void setUp()
    {
        if(debug) cout << endl ;
        if (bes_debug) BESDebug::SetUp("cerr,dmrpp");

        TheBESKeys::ConfigFile = string(TEST_BUILD_DIR).append("/bes.conf");
        cm_config = string(TEST_BUILD_DIR).append("/input-files/credentials.conf");
        weak_config = string(TEST_BUILD_DIR).append("/input-files/weak.conf");

    }

    // Called after each test
    void tearDown()
    {
    }
    void check_keys() {
        string value;
        bool found;
        TheBESKeys::TheKeys()->set_key(CATALOG_MANAGER_CREDENTIALS, "foo");
        TheBESKeys::TheKeys()->get_value(CATALOG_MANAGER_CREDENTIALS,value,found);

        CPPUNIT_ASSERT(found);
        CPPUNIT_ASSERT("foo" == value );

        if(debug)  cout << "check_keys() - Retrieved credentials file name from TheBESKeys: " << value << endl;

    }

    void bad_config_file_permissions() {
        try {
            TheBESKeys::TheKeys()->set_key(CATALOG_MANAGER_CREDENTIALS, weak_config);
            CredentialsManager::load_credentials();
            CPPUNIT_FAIL("bad_config_file_permissions() The load_credentials() call should have failed but it did not.");
        }
        catch (BESError &e) {
            if(debug) cout << "bad_config_file_permissions() - Unable to load keys, this is expected. Message: ";
            if(debug) cout << e.get_message() << endl;
        }


    }

    void load_credentials() {
        try {
            TheBESKeys::TheKeys()->set_key(CATALOG_MANAGER_CREDENTIALS, cm_config);
            CredentialsManager::load_credentials();
        }
        catch (BESError &e) {
            CPPUNIT_FAIL("bad_file_permissions() The load_credentials() "
                         "has failed unexpectedly. message");
            if(debug) cout << e.get_message() << endl;
        }

    }

    void check_credentials() {
        try {
            if(debug) cout << "load_credentials() - Read " << CredentialsManager::theCM()->size() << " AccessCredentials." << endl;
            CPPUNIT_ASSERT( CredentialsManager::theCM()->size() == 3);

            string cloudydap_dataset_url = "https://s3.amazonaws.com/cloudydap/samples/d_int.h5";
            string cloudyopendap_dataset_url = "https://s3.amazonaws.com/cloudyopendap/samples/d_int.h5";
            string someother_dataset_url = "https://ssotherone.org/opendap/data/fnoc1.nc";
            AccessCredentials *ac;
            string url, id, key, region, bucket;

            /*
            cloudydap=url:https://s3.amazonaws.com/cloudydap/
            cloudydap+=id:foo
            cloudydap+=key:qwecqwedqwed
            cloudydap+=region:us-east-1
            cloudydap+=bucket:cloudydap
            */

            ac = CredentialsManager::theCM()->get(cloudydap_dataset_url);
            CPPUNIT_ASSERT( ac);
            CPPUNIT_ASSERT( ac->get(AccessCredentials::URL) == "https://s3.amazonaws.com/cloudydap/");
            CPPUNIT_ASSERT( ac->get(AccessCredentials::ID) == "foo");
            CPPUNIT_ASSERT( ac->get(AccessCredentials::KEY) == "qwecqwedqwed");
            CPPUNIT_ASSERT( ac->get(AccessCredentials::REGION) == "us-east-1");
            CPPUNIT_ASSERT( ac->get(AccessCredentials::BUCKET) == "cloudydap");

            /*
            cloudyopendap+=url:https://s3.amazonaws.com/cloudyopendap/
            cloudyopendap+=id:bar
            cloudyopendap+=key:qwedjhgvewqwedqwed
            cloudyopendap+=region:nirvana-west-0
            cloudyopendap+=bucket:cloudyopendap
            */
            ac = CredentialsManager::theCM()->get(cloudyopendap_dataset_url);
            CPPUNIT_ASSERT( ac);
            CPPUNIT_ASSERT( ac->get(AccessCredentials::URL) == "https://s3.amazonaws.com/cloudyopendap/");
            CPPUNIT_ASSERT( ac->get(AccessCredentials::ID) == "bar");
            CPPUNIT_ASSERT( ac->get(AccessCredentials::KEY) == "qwedjhgvewqwedqwed");
            CPPUNIT_ASSERT( ac->get(AccessCredentials::REGION) == "nirvana-west-0");
            CPPUNIT_ASSERT( ac->get(AccessCredentials::BUCKET) == "cloudyopendap");

            /*
            cname_02+=url:https://ssotherone.org/opendap/
            cname_02+=id:some_other_id_string
            cname_02+=key:some_other_key_string
            cname_02+=region:oz-7
            cname_02+=bucket:cloudyotherdap
            */
            ac = CredentialsManager::theCM()->get(someother_dataset_url);
            CPPUNIT_ASSERT( ac);
            CPPUNIT_ASSERT( ac->get(AccessCredentials::URL) == "https://ssotherone.org/opendap/");
            CPPUNIT_ASSERT( ac->get(AccessCredentials::ID) == "some_other_id_string");
            CPPUNIT_ASSERT( ac->get(AccessCredentials::KEY) == "some_other_key_string");
            CPPUNIT_ASSERT( ac->get(AccessCredentials::REGION) == "oz-7");
            CPPUNIT_ASSERT( ac->get(AccessCredentials::BUCKET) == "cloudyotherdap");

        }
        catch (BESError &e) {
            CPPUNIT_FAIL("bad_file_permissions() The load_credentials() "
                         "has failed unexpectedly. message");
            if(debug) cout << e.get_message() << endl;
        }

    }

CPPUNIT_TEST_SUITE( CredentialsManagerTest );

        CPPUNIT_TEST(check_keys);
        CPPUNIT_TEST(bad_config_file_permissions);
        CPPUNIT_TEST(load_credentials);
        CPPUNIT_TEST(check_credentials);

   CPPUNIT_TEST_SUITE_END();

};

CPPUNIT_TEST_SUITE_REGISTRATION(CredentialsManagerTest);

} // namespace dmrpp

int main(int argc, char*argv[])
{
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    GetOpt getopt(argc, argv, "dD");
    int option_char;
    while ((option_char = getopt()) != -1)
        switch (option_char) {
        case 'd':
            debug = true;  // debug is a static global
            break;
        case 'b':
            debug = true;  // debug is a static global
            bes_debug = true;  // debug is a static global
            break;
        default:
            break;
        }

    bool wasSuccessful = true;
    int i = getopt.optind;
    if (i == argc) {
        // run them all
        wasSuccessful = runner.run("");
    }
    else {
        while (i < argc) {
            if (debug) cerr << "Running " << argv[i] << endl;
            string test = dmrpp::CredentialsManagerTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
