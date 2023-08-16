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

#include "test_config.h"

#include <memory>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>


#include "BESError.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "BESCatalogList.h"
#include "TheBESKeys.h"
#include "BESContextManager.h"
#include "BESForbiddenError.h"
#include "DapUtils.h"
#include "libdap/DMR.h"
#include "libdap/D4BaseTypeFactory.h"
#include "libdap/D4ParserSax2.h"
#include "libdap/D4ConstraintEvaluator.h"
#include "libdap/D4Group.h"

// Maybe the common testing code in modules should be moved up one level? jhrg 11/3/22
#include "modules/common/run_tests_cppunit.h"

using namespace std;
using namespace libdap;

#define prolog std::string("DapUtilsTest::").append(__func__).append("() - ")

namespace http {

class DapUtilsTest : public CppUnit::TestFixture {

public:

    // bool debug=true;

    // Called once before everything gets tested
    DapUtilsTest() = default;

    // Called at the end of the test
    ~DapUtilsTest() = default;

    bool hack_bug = true;

    // Called before each test
    void setUp()  {
        string bes_conf = BESUtil::assemblePath(TEST_BUILD_DIR, "bes.conf");

        if (debug) {
            cerr << endl;
            cerr << "setUp() - BEGIN" << endl;

            cerr << "setUp() - Using BES configuration file: " << bes_conf << endl;
            if (debug2) {
                show_file(bes_conf);
            }

            cerr << "Ingested BESDebug options: " << BESDebug::GetOptionsString() << endl;
        }

        TheBESKeys::ConfigFile = bes_conf;

        if (debug) cerr << "setUp() - END" << endl;
    }

    // Called after each test
    void tearDown()  {
    }



/*##################################################################################################*/
/* TESTS BEGIN */

    void var_too_big_test() {

        D4BaseTypeFactory d_d4f;
        DMR *d_test_dmr;
        d_test_dmr = new DMR(&d_d4f);
        D4ParserSax2 dp;
        stringstream msg;
        uint64_t response_size = 0;
        uint64_t expected_response_size = 1016;

        string file_name=BESUtil::pathConcat(TEST_SRC_DIR,"input-files/test_01.dmr");
        DBG(cerr << prolog << "DMR file to be parsed: " << file_name << endl);

        fstream in(file_name.c_str(), ios::in|ios::binary);
        dp.intern(in, d_test_dmr);
        d_test_dmr->root()->set_send_p(true);

        uint64_t max_size = 200;
        std::unordered_map<std::string,int64_t> too_big;
        response_size =  dap_utils::compute_response_size_and_inv_big_vars( *d_test_dmr, max_size, too_big);
        msg << prolog << "response_size: " << response_size  << " (expected: " << expected_response_size << ")" << endl;
        DBG( cerr << msg.str());

        // We can't test response size because the response contains arrays of strings and the string sizes
        // differ from one system to the next, example OS-X: 24 bytes, centos-8: 32 bytes

        if(!too_big.empty()){
            DBG( cerr << prolog << "Found " << too_big.size() <<  " variables larger than " << max_size << " bytes:" << endl);
            for(auto apair:too_big){
                DBG(cerr << prolog << "  " << apair.first << " (size: " << apair.second << ")" <<  endl);
            }
            CPPUNIT_ASSERT( too_big.size() == 2 );
        }
        else {
            CPPUNIT_FAIL("ERROR: No variables were deemed too big!");
        }
    }

    void dmrpp_var_too_big_test() {

        DMR *d_test_dmr;
        D4BaseTypeFactory d_d4f;
        d_test_dmr = new DMR(&d_d4f);
        D4ParserSax2 dp;
        stringstream msg;
        uint64_t response_size = 0;
        uint64_t expected_response_size = 180356500;

        string file_name=BESUtil::pathConcat(TEST_SRC_DIR,"input-files/tempo_l2.nc.dmrpp");
        DBG(cerr << prolog << "DMR file to be parsed: " << file_name << endl);

        fstream in(file_name.c_str(), ios::in|ios::binary);
        dp.intern(in, d_test_dmr);
        d_test_dmr->root()->set_send_p(true);

        uint64_t max_size = 1000000;
        std::unordered_map<std::string,int64_t> too_big;

        response_size = dap_utils::compute_response_size_and_inv_big_vars( *d_test_dmr, max_size, too_big);
        msg << prolog << "response_size: " << response_size  << " (expected: " << expected_response_size << ")" << endl;
        DBG( cerr << msg.str());

        msg.str(string());
        msg  << prolog << "ERROR: Unexpected response_size. expected: " << expected_response_size << " got response_size: " << response_size << endl;
        CPPUNIT_ASSERT_EQUAL_MESSAGE(msg.str(), response_size, expected_response_size);

        if(!too_big.empty()){
            DBG(cerr << prolog << "Found " << too_big.size() <<  " variables larger than " << max_size << " bytes:" << endl);
            for(auto apair:too_big){
                DBG(cerr << prolog << "  " << apair.first << " (size: " << apair.second << ")" <<  endl);
            }
            CPPUNIT_ASSERT(too_big.size() == 10 );
        }
        else {
            CPPUNIT_FAIL("ERROR: No variables were deemed too big!");
        }
    }

    void dmrpp_constrained_var_too_big_test() {

        DMR *d_test_dmr;
        D4BaseTypeFactory d_d4f;
        d_test_dmr = new DMR(&d_d4f);
        D4ParserSax2 dp;
        stringstream msg;
        uint64_t response_size = 0;
        uint64_t expected_response_size = 140378112;

        string file_name=BESUtil::pathConcat(TEST_SRC_DIR,"input-files/tempo_l2.nc.dmrpp");
        DBG(cerr << prolog << "DMR file to be parsed: " << file_name << endl);

        fstream in(file_name.c_str(), ios::in|ios::binary);
        dp.intern(in, d_test_dmr);
        D4ConstraintEvaluator d4ce(d_test_dmr);

        uint64_t max_size = 1000000;
        std::unordered_map<std::string,int64_t> too_big;

        d4ce.parse("/support_data/gas_profile;/support_data/scattering_weights");

        response_size = dap_utils::compute_response_size_and_inv_big_vars( *d_test_dmr, max_size, too_big);
        msg << prolog << "response_size: " << response_size  << " (expected: " << expected_response_size << ")" << endl;
        DBG( cerr << msg.str());

        msg.str(string());
        msg  << prolog << "ERROR: Unexpected response_size. expected: " << expected_response_size << " got response_size: " << response_size << endl;
        CPPUNIT_ASSERT_EQUAL_MESSAGE(msg.str(), response_size, expected_response_size);


        if(!too_big.empty()){
            DBG(cerr << prolog << "Found " << too_big.size() <<  " variables larger than " << max_size << " bytes:" << endl);
            for(auto apair:too_big){
                DBG(cerr << prolog << "  " << apair.first << "(" << apair.second << " bytes)" <<  endl);
            }
            CPPUNIT_ASSERT( too_big.size() == 2);
        }
        else {
            CPPUNIT_FAIL(prolog + "ERROR: The constraint reduced the size of the variables to less than the maximum "
                                  "when it should not have done so. No variables were deemed too big!");
        }

    }


    void dmrpp_constrained_var_ok_test() {

        DMR *d_test_dmr;
        D4BaseTypeFactory d_d4f;
        d_test_dmr = new DMR(&d_d4f);
        D4ParserSax2 dp;
        stringstream msg;
        uint64_t response_size = 0;
        uint64_t expected_response_size = 1179648;

        string file_name=BESUtil::pathConcat(TEST_SRC_DIR,"input-files/tempo_l2.nc.dmrpp");
        DBG(cerr << prolog << "DMR file to be parsed: " << file_name << endl);

        fstream in(file_name.c_str(), ios::in|ios::binary);
        dp.intern(in, d_test_dmr);
        D4ConstraintEvaluator d4ce(d_test_dmr);

        uint64_t max_size = 1000000;
        std::unordered_map<std::string,int64_t> too_big;

        d4ce.parse("/support_data/gas_profile[1][][];/support_data/scattering_weights[3][][]");

        response_size = dap_utils::compute_response_size_and_inv_big_vars( *d_test_dmr, max_size, too_big);
        msg << prolog << "response_size: " << response_size  << " (expected: " << expected_response_size << ")" << endl;
        DBG( cerr << msg.str());

        msg.str(string());
        msg  << prolog << "ERROR: Unexpected response_size. expected: " << expected_response_size << " got response_size: " << response_size << endl;
        CPPUNIT_ASSERT_EQUAL_MESSAGE(msg.str(), response_size, expected_response_size);

        if(!too_big.empty()){
            DBG( cerr << prolog << "Found " << too_big.size() <<  " variables larger than " << max_size << " bytes:" << endl);
            for(auto apair:too_big){
                DBG( cerr << prolog << "  " << apair.first << "(" << apair.second << " bytes)" <<  endl);
            }
            CPPUNIT_FAIL( prolog + "ERROR The applied constraint expression should have reduced the size of the "
                                   "requested variables so that the are no longer too big. That's not ok." );
        }
        else {
            CPPUNIT_ASSERT( too_big.empty() );
        }


    }

/* TESTS END */
/*##################################################################################################*/

    CPPUNIT_TEST_SUITE(DapUtilsTest);

    CPPUNIT_TEST(var_too_big_test);
    CPPUNIT_TEST(dmrpp_var_too_big_test);
    CPPUNIT_TEST(dmrpp_constrained_var_too_big_test);
    CPPUNIT_TEST(dmrpp_constrained_var_ok_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(DapUtilsTest);

} // namespace http

int main(int argc, char *argv[]) {
    return bes_run_tests<http::DapUtilsTest>(argc, argv, "cerr,bes,dap_utils") ? 0 : 1;
}
