// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES component of the Hyrax Data Server.

// Copyright (c) 2020 OPeNDAP, Inc.
// Author: Nathan Potter <ndp@opendap.org>
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
#include <cstring>
#include <iostream>

#include <curl/curl.h>
#include <curl/easy.h>

#include "BESError.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "BESStopWatch.h"
#include "BESCatalogList.h"
#include "TheBESKeys.h"
#include "BESContextManager.h"
#include "url_impl.h"
#include "AccessCredentials.h"
#include "CredentialsManager.h"
#include "BESForbiddenError.h"
#include "BESSyntaxUserError.h"
#include "CurlUtils.h"
#include "HttpError.h"

#include "test_config.h"

// Maybe the common testing code in modules should be moved up one level? jhrg 11/3/22
#include "modules/common/run_tests_cppunit.h"

using namespace std;

#define prolog std::string("# CurlSListTest::").append(__func__).append("() - ")

namespace http {

class CurlSListTest : public CppUnit::TestFixture {

public:
    string d_data_dir = TEST_DATA_DIR;

    // Called once before everything gets tested
    CurlSListTest() = default;

    // Called at the end of the test
    ~CurlSListTest() override = default;

    // Called before each test
    void setUp() override {
        DBG( cerr << "\n");
        DBG( cerr << "#-----------------------------------------------------------------\n");
        DBG( cerr << "setUp() - BEGIN\n");
        debug = true;
        string bes_conf = BESUtil::assemblePath(TEST_BUILD_DIR, "bes.conf");
        DBG( cerr << "setUp() - Using BES configuration: " << bes_conf << "\n");
        DBG2( show_file(bes_conf));
        TheBESKeys::ConfigFile = bes_conf;

        DBG( cerr << "setUp() - END\n");
    }

    // Called after each test
    void tearDown() override {
        // These are set in add_edl_auth_headers_test() and not 'unsetting' them
        // causes other odd behavior in subsequent tests (Forbidden exceptions
        // become SyntaxUser ones). Adding the unset operations here ensures they
        // happen even if exceptions are thrown by the add_edl...() test.
        BESContextManager::TheManager()->unset_context(EDL_UID_KEY);
        BESContextManager::TheManager()->unset_context(EDL_AUTH_TOKEN_KEY);
        BESContextManager::TheManager()->unset_context(EDL_ECHO_TOKEN_KEY);

        // We have to remove the cookie file between test invocations.
        // Not doing so can cause the previous test's login success
        // to propagate to the next test. Which is a problem when testing
        // behaviors related to authentication success/failure.
        auto cookie_file = curl::get_cookie_filename();
        ifstream f(cookie_file.c_str());
        if(f.good()) {
            int retval = std::remove(cookie_file.c_str());
            if (retval != 0 && debug) {
                stringstream msg;
                msg << "Failed to delete cookie file: '" << cookie_file << "' ";
                msg << "Message: " << strerror(errno);
                DBG(cerr << prolog << msg.str() << "\n");
            }
        }
        DBG( cerr << "\n");
    }

/*##################################################################################################*/
/* TESTS BEGIN */


    string lorem="Lorem ipsum dolor sit amet, consectetur adipiscing elit. Mauris semper laoreet enim, sed porttitor "
                 "lacus consectetur sit amet. Mauris ultrices aliquet velit, in mattis dolor rutrum et. Curabitur "
                 "dolor purus, aliquet nec erat ut, ullamcorper vestibulum sem. Donec mattis pharetra justo, vel "
                 "egestas mi semper sit amet. Sed scelerisque elementum nunc vitae faucibus. Cras mattis facilisis "
                 "arcu quis aliquet. Morbi rutrum nunc et mattis laoreet. In finibus ante eu ullamcorper tempor. "
                 "Curabitur blandit, nisl ut blandit malesuada, arcu sem faucibus eros, sit amet congue velit sem "
                 "in velit. Etiam ut tellus venenatis, sodales elit vel, finibus augue. Integer consequat purus.";

    vector<string> slist_baselines{
            "FirstName: Willy",
            "LastName: Wonka",
            "kjwhebd: jkhbvkwjqehv ljhcljhbwvcqwx",
            "lorem: "+lorem
    };

    curl_slist *load_slist(curl_slist *request_headers){
        auto hdrs = curl::append_http_header(request_headers,"FirstName", "Willy");
        hdrs = curl::append_http_header(hdrs,"LastName", "Wonka");
        hdrs = curl::append_http_header(hdrs,"kjwhebd", "jkhbvkwjqehv ljhcljhbwvcqwx");
        hdrs = curl::append_http_header(hdrs,"lorem", lorem);
        return hdrs;
    }

    void check_slist(curl_slist *slist, const vector<string> &baselines ){
        string baseline;

        DBG(cerr << prolog << "           slist: " << (void *)slist << "\n");
        DBG(cerr << prolog << "baselines.size(): " << baselines.size() << "\n");
        size_t i = 0;
        size_t hc = 0;
        while(slist != nullptr || i < baselines.size()){
            string slist_value;
            if( i < baselines.size()){
                baseline = baselines[i];
                DBG(cerr << prolog << "    baselines[" << i << "]: " << baseline << "\n");
            }
            else {
                DBG(cerr << prolog << "No baseline for index: " << i << "\n");
            }
            if(slist){
                slist = slist->next;
                DBG(cerr << prolog << " slist_itr->next: " << (void *)slist << "\n");
                if(slist) {
                    slist_value = slist->data;
                    DBG(cerr << prolog << "  slist_value[" << i << "]: " << slist_value << "\n");
                    hc++;
                }
            }
            else {
                DBG(cerr << prolog << "The slist_itr is nullptr for pass: " << i << "\n");
            }
            DBG(cerr << prolog << " slist: " << (void *)slist << " i: " << i << "\n");
            if((slist != nullptr) && (i < baselines.size())){
                CPPUNIT_ASSERT(baseline == slist_value);
                DBG(cerr << prolog << "baseline and slist_value matched.\n");
            }
            else {
                DBG(cerr << prolog << "No test performed, one of baseline or slist_value was missing.\n");
            }
            DBG(cerr << "\n");

            i++;
        }
        CPPUNIT_ASSERT_MESSAGE("Header count and baselines should match. "
                               "baselines: " +to_string(baselines.size()) +
                               " headers: " + to_string(hc), hc == baselines.size());
    }

    void new_curl_slist_test() {
        string baseline;
        auto test_slist = new curl_slist{};
        DBG(cerr << prolog << "test_slist: " << (void *)test_slist << "\n");
        CPPUNIT_ASSERT_MESSAGE("test_slist should be not null.", test_slist != nullptr);

        try {
            auto slist = load_slist(test_slist);
            DBG(cerr << prolog << "         slist: " << (void *)slist << "\n");
            CPPUNIT_ASSERT_MESSAGE("slist should be not null.", slist != nullptr);

            CPPUNIT_ASSERT_MESSAGE("test_slist and slist should be the same object.", test_slist == slist);

            check_slist(slist, slist_baselines);

            delete test_slist;
        }
        catch(...){
            if(test_slist) {
                delete test_slist;
            }

        }
    }

    void nullptr_curl_slist_test() {
        string baseline;
        curl_slist *test_slist = nullptr;
        DBG(cerr << prolog << "request_headers: " << (void *)test_slist << "\n");
        //CPPUNIT_ASSERT_MESSAGE("request_headers should be not null.", test_slist != nullptr);

        try {
            auto hdrs = load_slist(test_slist);
            DBG(cerr << prolog << "           hdrs: " << (void *)hdrs << "\n");
            CPPUNIT_ASSERT_MESSAGE("hdrs should be not null.", hdrs != nullptr);

            check_slist(hdrs, slist_baselines);

            delete test_slist;
        }
        catch(...){
            if(test_slist) {
                delete test_slist;
            }

        }
    }

/* TESTS END */
/*##################################################################################################*/

    CPPUNIT_TEST_SUITE(CurlSListTest);


    CPPUNIT_TEST(new_curl_slist_test);

    CPPUNIT_TEST(nullptr_curl_slist_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(CurlSListTest);

} // namespace http

int main(int argc, char *argv[]) {
    return bes_run_tests<http::CurlSListTest>(argc, argv, "cerr,bes,http,curl,curl:timing") ? 0 : 1;
}
