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

#include "CurlUtils.h"

#include "test_config.h"

// Maybe the common testing code in modules should be moved up one level? jhrg 11/3/22
#include "modules/common/run_tests_cppunit.h"

using namespace std;

#define prolog std::string("CurlSListTest::").append(__func__).append("() - ")

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
        DBG( cerr << "setUp() - END\n");
    }

    // Called after each test
    void tearDown() override {
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

    curl_slist *load_slist(curl_slist *request_headers) const {
        auto slist = request_headers;
        //hdrs = curl::append_http_header(hdrs, "Dum", "Dummer");
        DBG(cerr << prolog << "slist: " << (void **)slist << "\n");
        slist = curl::append_http_header(slist,"FirstName", "Willy");
        DBG(cerr << prolog << "slist: " << (void **)slist << "\n");
        slist = curl::append_http_header(slist,"LastName", "Wonka");
        DBG(cerr << prolog << "slist: " << (void **)slist << "\n");
        slist = curl::append_http_header(slist,"kjwhebd", "jkhbvkwjqehv ljhcljhbwvcqwx");
        DBG(cerr << prolog << "slist: " << (void **)slist << "\n");
        slist = curl::append_http_header(slist,"lorem", lorem);
        DBG(cerr << prolog << "slist: " << (void **)slist << "\n");
        return slist;
    }

    static bool check_slist(curl_slist *slist, const vector<string> &baselines ){
        DBG( cerr << prolog << "BEGIN\n");
        string baseline;
        bool success = true;

        DBG(cerr << prolog << "           slist: " << (void **)slist << "\n");
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
                DBG(cerr << prolog << " slist_itr->next: " << (void **)slist << "\n");
                if(slist) {
                    slist_value.append(slist->data);
                    DBG(cerr << prolog << "  slist_value[" << i << "]: " << slist_value << "\n");
                    hc++;
                }
            }
            else {
                DBG(cerr << prolog << "The slist_itr is nullptr for pass: " << i << "\n");
            }
            DBG(cerr << prolog << " slist: " << (void **)slist << " i: " << i << "\n");
            if((slist != nullptr) && (i < baselines.size())){
                bool matched = baseline == slist_value;
                DBG(cerr << prolog << "baseline and slist_value " << (matched?"matched.":"did not match.") << "\n");
                success = success && matched;
                // CPPUNIT_ASSERT(matched);
            }
            else {
                DBG(cerr << prolog << "No test performed, one, or both, of baseline and/or slist_value was missing.\n");
            }
            DBG(cerr << "\n");
            slist = slist->next;
            i++;
        }
        success = success && (hc == baselines.size());
        //CPPUNIT_ASSERT_MESSAGE("Header count and baselines should match. "
          //                     "baselines: " +to_string(baselines.size()) +
            //
            //                   " headers: " + to_string(hc), hc == baselines.size());
        DBG(cerr << prolog << (success?"SUCCEEDED :)":"FAILED :O") << "\n");

        return success;
    }

    void new_curl_slist_test() {
        DBG( cerr << prolog << "BEGIN\n");
        string baseline;
        auto test_slist = new curl_slist{};
        DBG(cerr << prolog << "test_slist: " << (void **)test_slist << "\n");
        CPPUNIT_ASSERT_MESSAGE("test_slist should be not null.", test_slist != nullptr);

        try {
            auto slist = load_slist(test_slist);
            DBG(cerr << prolog << " slist: " << (void **)slist << "\n");
            CPPUNIT_ASSERT_MESSAGE("slist should be not null.", slist != nullptr);

            CPPUNIT_ASSERT_MESSAGE("test_slist and slist should be the same object.", test_slist == slist);

            // DBG( cerr << prolog << "First Header(no advance): " << slist->data << "\n");
            slist->next;

            bool check_slist_status = check_slist(slist, slist_baselines);
            DBG(cerr << prolog << "check_slist_status: " << (check_slist_status?"TRUE":"FALSE") << " (" << check_slist_status << ")\n");
            CPPUNIT_ASSERT_MESSAGE( prolog + "The check_slist() function did not return true", check_slist_status );

            DBG(cerr << prolog << "Calling delete test_slist\n");
            delete test_slist;
        }
        catch(...){
            DBG(cerr << prolog << "Caught exception. Calling delete on test_slist:" << (void **) test_slist << "\n");
            delete test_slist;
            throw;
        }
        DBG( cerr << prolog << "END\n");
    }

    void nullptr_curl_slist_test() {
        string baseline;
        curl_slist *test_slist = nullptr;
        curl_slist *slist = nullptr;
        DBG(cerr << prolog << "request_headers: " << (void **)test_slist << "\n");

        try {
            slist = load_slist(test_slist);
            DBG(cerr << prolog << "           hdrs: " << (void **) slist << "\n");
            CPPUNIT_ASSERT_MESSAGE("hdrs should be not null.", slist != nullptr);

            DBG( cerr << prolog << "First Header(no advance): " << slist->data << "\n");

            bool check_slist_status = check_slist(slist, slist_baselines);
            DBG(cerr << prolog << "check_slist_status: " << (check_slist_status?"TRUE":"FALSE") << "(" << check_slist_status << ")\n");
            // CPPUNIT_ASSERT_MESSAGE( prolog + "The check_slist() function did not return true", check_slist_status );
            DBG(cerr << prolog << "Calling curl_slist_free_all(slist)\n");
            curl_slist_free_all(slist);
        }
        catch(...){
            DBG(cerr << prolog << "Caught exception. slist: " << (void **) slist << "\n");
            if(slist){
                DBG(cerr << prolog << "Calling curl_slist_free_all(slist)\n");
                curl_slist_free_all(slist);
            }
            throw;
        }
        DBG( cerr << prolog << "END\n");
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
