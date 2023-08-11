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

            cerr << "Ingested debug options: " << BESDebug::GetOptionsString() << endl;
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

        DMR *d_test_dmr;
        D4BaseTypeFactory d_d4f;
        d_test_dmr = new DMR(&d_d4f);
        D4ParserSax2 dp;

        string file_name=BESUtil::pathConcat(TEST_SRC_DIR,"input-files/test_01.dmr");
        DBG(cerr << prolog << "DMR file to be parsed: " << file_name << endl);

        fstream in(file_name.c_str(), ios::in|ios::binary);
        dp.intern(in, d_test_dmr);

        uint64_t max_size = 200;

        std::unordered_map<std::string,int64_t> too_big;
        dap_utils::find_too_big_vars( *d_test_dmr, max_size, too_big);
        if(!too_big.empty()){
            cerr << prolog << "Found " << too_big.size() <<  " variables larger than " << max_size << " bytes:" << endl;
            for(auto apair:too_big){
                cerr << prolog << "  " << apair.first << " (size: " << apair.second << ")" <<  endl;
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

        string file_name=BESUtil::pathConcat(TEST_SRC_DIR,"input-files/tempo_l2.nc.dmrpp");
        DBG(cerr << prolog << "DMR file to be parsed: " << file_name << endl);

        fstream in(file_name.c_str(), ios::in|ios::binary);
        dp.intern(in, d_test_dmr);

        uint64_t max_size = 1000000;
        std::unordered_map<std::string,int64_t> too_big;
        dap_utils::find_too_big_vars( *d_test_dmr, max_size, too_big);
        if(!too_big.empty()){
            cerr << prolog << "Found " << too_big.size() <<  " variables larger than " << max_size << " bytes:" << endl;
            for(auto apair:too_big){
                cerr << prolog << "  " << apair.first << " (size: " << apair.second << ")" <<  endl;
            }
            CPPUNIT_ASSERT(too_big.size() == 10 );
        }
        else {
            CPPUNIT_FAIL("ERROR: No variables were deemed too big!");
        }
    }

/* TESTS END */
/*##################################################################################################*/

    CPPUNIT_TEST_SUITE(DapUtilsTest);

    CPPUNIT_TEST(var_too_big_test);
    CPPUNIT_TEST(dmrpp_var_too_big_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(DapUtilsTest);

} // namespace http

int main(int argc, char *argv[]) {
    return bes_run_tests<http::DapUtilsTest>(argc, argv, "bes,dap_utils") ? 0 : 1;
}
