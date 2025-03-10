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

#include "config.h"

#include <memory>
#include <string>

#include <cstdlib>
#include <unistd.h>

#include "BESContextManager.h"
#include "BESInternalError.h"
#include "BESDebug.h"
#include "TheBESKeys.h"

#include "url_impl.h"
#include "AccessCredentials.h"
#include "CredentialsManager.h"

// Maybe the common testing code in modules should be moved up one level? jhrg 11/3/22
#include "modules/common/run_tests_cppunit.h"

#include "test_config.h"

using namespace std;

namespace http {

class CredentialsManagerTest: public CppUnit::TestFixture {
    string cm_config;
    string weak_config;

public:
    // Called once before everything gets tested
    CredentialsManagerTest() = default;

    // Called at the end of the test
    ~CredentialsManagerTest() override = default;

    // Called before each test
    void setUp() override  {
        TheBESKeys::ConfigFile = string(TEST_BUILD_DIR).append("/bes.conf");
        cm_config = string(TEST_BUILD_DIR).append("/credentials.conf");
        weak_config = string(TEST_SRC_DIR).append("/weak.conf");
    }

    void check_keys() {
        string value;
        bool found;
        TheBESKeys::TheKeys()->set_key(CATALOG_MANAGER_CREDENTIALS, "foo");
        TheBESKeys::TheKeys()->get_value(CATALOG_MANAGER_CREDENTIALS, value, found);

        CPPUNIT_ASSERT(found);
        CPPUNIT_ASSERT("foo" == value);

        DBG(cerr << "check_keys() - Retrieved credentials file name from TheBESKeys: " << value << '\n');
    }


    void bad_config_file_permissions() {
        try {
            TheBESKeys::TheKeys()->set_key(CATALOG_MANAGER_CREDENTIALS, weak_config);
            auto cm = CredentialsManager::theCM();
            DBG(cerr << "bad_config_file_permissions() - After theCM()\n");
            cm->load_credentials();
            DBG(cerr << "bad_config_file_permissions() - After load_credentials()\n");
            CPPUNIT_FAIL("The load_credentials() call should have failed but it did not.");
        }
        catch (const BESInternalError &e) {
            DBG(cerr << "bad_config_file_permissions() - Caught expected exception: " << e.get_message() << '\n');
            CPPUNIT_ASSERT(true);
        }
    }

    void load_credentials() {
        DBG(cerr  << "load_credentials() - Loading AccessCredentials." << '\n');
        try {
            TheBESKeys::TheKeys()->set_key(CATALOG_MANAGER_CREDENTIALS, cm_config);
            auto ac = CredentialsManager::theCM();
            ac->load_credentials();
            DBG(cerr << "load_credentials() - AccessCredentials loaded.\n");
            CPPUNIT_ASSERT(true);
        }
        catch (const BESError &e) {
            CPPUNIT_FAIL("load_credentials() has failed unexpectedly. Message: "+ e.get_message());
        }
    }

    void check_credentials() {
        try {
            TheBESKeys::TheKeys()->set_key(CATALOG_MANAGER_CREDENTIALS, cm_config);
#if 0
          unsigned int expected = CredentialsManager::theCM()->hasNgapS3Credentials()?4:3;
            DBG(cerr << "check_credentials() - Found " << CredentialsManager::theCM()->size() <<
                " AccessCredentials. Expected: "<< expected << '\n');
            CPPUNIT_ASSERT( CredentialsManager::theCM()->size() == expected);

#endif

            auto cloudydap_dataset_url = make_shared<http::url>("https://s3.amazonaws.com/cloudydap/samples/d_int.h5");
            auto cloudyopendap_dataset_url= make_shared<http::url>("https://s3.amazonaws.com/cloudyopendap/samples/d_int.h5");
            auto someother_dataset_url = make_shared<http::url>("https://ssotherone.org/opendap/data/fnoc1.nc");
            AccessCredentials *ac;
            string url, id, key, region, bucket;

            ac = CredentialsManager::theCM()->get(cloudydap_dataset_url);
            CPPUNIT_ASSERT( ac);
            CPPUNIT_ASSERT( ac->get(AccessCredentials::URL_KEY) == "https://s3.amazonaws.com/cloudydap/");
            CPPUNIT_ASSERT( ac->get(AccessCredentials::ID_KEY) == "foo");
            CPPUNIT_ASSERT( ac->get(AccessCredentials::KEY_KEY) == "qwecqwedqwed");
            CPPUNIT_ASSERT( ac->get(AccessCredentials::REGION_KEY) == "us-east-1");

            ac = CredentialsManager::theCM()->get(cloudyopendap_dataset_url);
            CPPUNIT_ASSERT( ac);
            CPPUNIT_ASSERT( ac->get(AccessCredentials::URL_KEY) == "https://s3.amazonaws.com/cloudyopendap/");
            CPPUNIT_ASSERT( ac->get(AccessCredentials::ID_KEY) == "bar");
            CPPUNIT_ASSERT( ac->get(AccessCredentials::KEY_KEY) == "qwedjhgvewqwedqwed");
            CPPUNIT_ASSERT( ac->get(AccessCredentials::REGION_KEY) == "nirvana-west-0");

            ac = CredentialsManager::theCM()->get(someother_dataset_url);
            CPPUNIT_ASSERT( ac);
            CPPUNIT_ASSERT( ac->get(AccessCredentials::URL_KEY) == "https://ssotherone.org/opendap/");
            CPPUNIT_ASSERT( ac->get(AccessCredentials::ID_KEY) == "some_other_id_string");
            CPPUNIT_ASSERT( ac->get(AccessCredentials::KEY_KEY) == "some_other_key_string");
            CPPUNIT_ASSERT( ac->get(AccessCredentials::REGION_KEY) == "oz-7");
        }
        catch (BESError &e) {
            CPPUNIT_FAIL("check_credentials() has failed unexpectedly. message");
            DBG(cerr  << e.get_message() << endl);
        }
        DBG(cerr  << "check_credentials() - Credentials matched expected." << endl);
    }

    /**
     * Clears the environment variable used by the CredentialsManager for credentials injection.
     */
    void clear_cm_env(){
        unsetenv(CredentialsManager::ENV_ID_KEY);
        unsetenv(CredentialsManager::ENV_ACCESS_KEY);
        unsetenv(CredentialsManager::ENV_REGION_KEY);
    }

    void check_incomplete_env_credentials() {
#if 0
          unsigned int expected = CredentialsManager::theCM()->hasNgapS3Credentials()?4:3;
        DBG(cerr << "check_incomplete_env_credentials() - Found " << CredentialsManager::theCM()->size() <<
            " existing AccessCredentials. Expected: "<< expected << '\n');
        CPPUNIT_ASSERT( CredentialsManager::theCM()->size() == expected);
#endif

        CredentialsManager::theCM()->clear();
        DBG(cerr  << "check_incomplete_env_credentials() - CredentialsManager has been cleared, contains " << CredentialsManager::theCM()->size() << " AccessCredentials." << endl);
        CPPUNIT_ASSERT( CredentialsManager::theCM()->size() == 0);

        clear_cm_env();
        TheBESKeys::TheKeys()->set_key(CATALOG_MANAGER_CREDENTIALS, CredentialsManager::USE_ENV_CREDS_KEY_VALUE);

        DBG(cerr << "check_incomplete_env_credentials() - Setting incomplete env injected credentials. "
                          "They should be ignored."<< '\n');

        string id("Frank Morgan");
        string region("oz-east-1");
        string bucket("emerald_city");
        string url("https://s3.amazonaws.com/emerald_city");
        string some_dataset_url(url+"data/fnoc1.nc");

        setenv(CredentialsManager::ENV_ID_KEY, id.c_str(), true);
        setenv(CredentialsManager::ENV_REGION_KEY, region.c_str(), true);
        setenv(CredentialsManager::ENV_URL_KEY, url.c_str(), true);

        CredentialsManager::theCM();    // force a load of the credentials

        DBG(cerr  << "check_incomplete_env_credentials() - Read from ENV, found " << CredentialsManager::theCM()->size() << " AccessCredentials." << endl);
        CPPUNIT_ASSERT( CredentialsManager::theCM()->size() == 0);

        DBG(cerr  << "check_incomplete_env_credentials() - Incomplete ENV credentials ignored as expected." << endl);
    }

    void check_env_credentials() {
        CredentialsManager::theCM()->clear();
        DBG(cerr  << "check_env_credentials() - CredentialsManager has been cleared, contains " << CredentialsManager::theCM()->size() << " AccessCredentials." << endl);
        CPPUNIT_ASSERT(CredentialsManager::theCM()->size() == 0);

        clear_cm_env();
        TheBESKeys::TheKeys()->set_key(CATALOG_MANAGER_CREDENTIALS, CredentialsManager::USE_ENV_CREDS_KEY_VALUE, false);

        string id("Frank Morgan");
        string key("Ihadasecretthingforthewickedwitchofthewest");
        string region("oz-east-1");
        string bucket("emerald_city");
        string base_url("https://s3.amazonaws.com/emerald_city/");
        shared_ptr<http::url> some_dataset_url(new http::url(base_url+"data/fnoc1.nc"));

        setenv(CredentialsManager::ENV_ID_KEY,     id.c_str(), true);
        setenv(CredentialsManager::ENV_ACCESS_KEY, key.c_str(), true);
        setenv(CredentialsManager::ENV_REGION_KEY, region.c_str(), true);
        //setenv(CMAC_ENV_BUCKET_KEY, bucket.c_str(),true);
        setenv(CredentialsManager::ENV_URL_KEY,    base_url.c_str(), true);
        DBG(cerr  << "check_env_credentials() - Environment conditioned, calling CredentialsManager::load_credentials()" << endl);
        auto cm = CredentialsManager::theCM();
        cm->load_credentials(); // force a load of the credentials

        unsigned int expected = 1;
        DBG(cerr << "check_env_credentials() - Read from ENV, found " << CredentialsManager::theCM()->size() <<
            " AccessCredentials. Expected: " << expected << '\n');
        CPPUNIT_ASSERT( CredentialsManager::theCM()->size() == expected);

        AccessCredentials *ac = CredentialsManager::theCM()->get(some_dataset_url);
        CPPUNIT_ASSERT( ac );
        CPPUNIT_ASSERT( ac->get(AccessCredentials::URL_KEY) == base_url);
        CPPUNIT_ASSERT( ac->get(AccessCredentials::ID_KEY) == id);
        CPPUNIT_ASSERT( ac->get(AccessCredentials::KEY_KEY) == key);
        CPPUNIT_ASSERT( ac->get(AccessCredentials::REGION_KEY) == region);

        DBG(cerr  << "check_env_credentials() - Credentials matched expected." << endl);
    }

#if 0
     void check_ngap_s3_credentials() {
        clear_cm_env();
        CredentialsManager::theCM()->clear();
        TheBESKeys::TheKeys()->set_key(CATALOG_MANAGER_CREDENTIALS, "");
        DBG(cerr  << "check_no_credentials() - CredentialsManager has been cleared, contains " << CredentialsManager::theCM()->size() << " AccessCredentials." << endl);
        CPPUNIT_ASSERT( CredentialsManager::theCM()->size() == 0);

        CredentialsManager::theCM();   // force a load of the credentials

        if(CredentialsManager::theCM()->hasNgapS3Credentials()){
            unsigned int expected = 1;
            DBG(cerr << "check_no_credentials() - Called CredentialsManager::load_credentials(), found "
                     << CredentialsManager::theCM()->size() << " AccessCredentials. Expected:" << expected << '\n');
            CPPUNIT_ASSERT( CredentialsManager::theCM()->size() == expected);

            shared_ptr<http::url> target_url(new http::url("https://s3.us-west-2.amazonaws.com"));
            AccessCredentials *ac = CredentialsManager::theCM()->get(target_url);
            CPPUNIT_ASSERT( ac );

            if(debug) {
                try {
                    cout << "                   ac->name(): " << ac->name() << endl;
                    cout << "   AccessCredentials::URL_KEY: " << ac->get(AccessCredentials::URL_KEY) << endl;
                    cout << "    AccessCredentials::ID_KEY: " << ac->get(AccessCredentials::ID_KEY) << endl;
                    cout << "   AccessCredentials::KEY_KEY: " << ac->get(AccessCredentials::KEY_KEY) << endl;
                    cout << "AccessCredentials::REGION_KEY: " << ac->get(AccessCredentials::REGION_KEY) << endl;
                }
                catch (const BESError &besError){
                    cerr << endl << endl;
                    cerr << "Caught BESError. Message: " << besError.get_message() << "  ";
                    cerr << "[" << besError.get_file() << ":" << besError.get_line() << "]" << endl << endl;
                    CPPUNIT_ASSERT(false);
                }
            }
        }
    }

#endif

    CPPUNIT_TEST_SUITE( CredentialsManagerTest );

    CPPUNIT_TEST(check_keys);
    // Change this test once we decide on how to handle missing and/or bad config files. jhrg 3/7/25
    CPPUNIT_TEST_FAIL(bad_config_file_permissions);
    CPPUNIT_TEST(load_credentials);
    CPPUNIT_TEST(check_credentials);
    CPPUNIT_TEST(check_incomplete_env_credentials);
    CPPUNIT_TEST(check_env_credentials);
#if 0
      CPPUNIT_TEST(check_ngap_s3_credentials);
#endif


    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(CredentialsManagerTest);

} // namespace http

int main(int argc, char *argv[]) {
    return bes_run_tests<http::CredentialsManagerTest>(argc, argv, "cerr,dmrpp,curl,ngap") ? 0 : 1;
}
