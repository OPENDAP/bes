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
#include <iostream>
#include <fstream>
#include <sstream>


#include "BESError.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "TheBESKeys.h"
#include "BESContextManager.h"
#include "BESSyntaxUserError.h"
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

namespace dap_utils {

constexpr auto BES_KEYS_MAX_RESPONSE_SIZE_KEY = "BES.MaxResponseSize.bytes";
constexpr auto BES_KEYS_MAX_VAR_SIZE_KEY = "BES.MaxVariableSize.bytes";
constexpr auto BES_CONTEXT_MAX_RESPONSE_SIZE_KEY = "max_response_size";
constexpr auto BES_CONTEXT_MAX_VAR_SIZE_KEY = "max_variable_size";


class DapUtilsTest : public CppUnit::TestFixture {

public:
    const uint64_t fourGB = 4294967296;
    const uint64_t twoGB = 2147483648;

    // bool debug=true;

    // Called once before everything gets tested
    DapUtilsTest() = default;

    // Called at the end of the test
    ~DapUtilsTest() = default;

    bool hack_bug = true;

    // Called before each test
    void setUp()  {
        string bes_conf = BESUtil::assemblePath(TEST_BUILD_DIR, "bes.conf");

        TheBESKeys::TheKeys()->set_key(BES_KEYS_MAX_RESPONSE_SIZE_KEY,"0");
        TheBESKeys::TheKeys()->set_key(BES_KEYS_MAX_VAR_SIZE_KEY,"0");
        BESContextManager::TheManager()->unset_context(BES_CONTEXT_MAX_RESPONSE_SIZE_KEY);
        BESContextManager::TheManager()->unset_context(BES_CONTEXT_MAX_VAR_SIZE_KEY);

        if (debug) {
            cerr << endl;
            cerr << "setUp() - BEGIN" << endl;

            cerr << "setUp() - Using BES configuration file: " << bes_conf << endl;
            if (debug2) {
                show_file(bes_conf);
            }

        }
        if(debug2) {
            cerr << prolog << "Ingested BESDebug options: " << BESDebug::GetOptionsString() << endl;
            bool found = false;
            string bcmrs = BESContextManager::TheManager()->get_context(BES_CONTEXT_MAX_RESPONSE_SIZE_KEY,found);
            if(!found) { bcmrs = "not-found" ;}
            cerr << prolog << BES_CONTEXT_MAX_RESPONSE_SIZE_KEY << ": " << bcmrs << '\n';


            found = false;
            string bcmvs = BESContextManager::TheManager()->get_context(BES_CONTEXT_MAX_VAR_SIZE_KEY,found);
            if(!found) { bcmvs = "not-found" ;}
            cerr << prolog << BES_CONTEXT_MAX_VAR_SIZE_KEY << ": " << bcmvs << '\n';

            found = false;
            string bkmrs;
            TheBESKeys::TheKeys()->get_value(BES_KEYS_MAX_RESPONSE_SIZE_KEY,bkmrs,found);
            if(!found) { bkmrs="not-found"; }
            cerr << prolog << BES_KEYS_MAX_RESPONSE_SIZE_KEY << ": " << bkmrs << '\n';

            found = false;
            string bkmvs;
            TheBESKeys::TheKeys()->get_value(BES_KEYS_MAX_VAR_SIZE_KEY,bkmvs, found);
            if(!found) { bkmrs="not-found"; }
            cerr << prolog << BES_KEYS_MAX_VAR_SIZE_KEY << ": " << bkmvs << '\n';
        }

        TheBESKeys::ConfigFile = bes_conf;

        if (debug) cerr << "setUp() - END" << endl;
    }

    // Called after each test
    void tearDown()  {
    }

    std::unique_ptr<DMR> mk_dmr_from_file(string filename, D4ParserSax2 &dp, D4BaseTypeFactory *d4f){
        auto test_dmr = std::make_unique<DMR>(d4f);

        string file_name = BESUtil::pathConcat(TEST_SRC_DIR, filename);
        DBG(cerr << prolog << "DMR file to be parsed: " << file_name << endl);

        fstream in(file_name.c_str(), ios::in | ios::binary);
        dp.intern(in, test_dmr.get());

        return test_dmr;
    }


/*##################################################################################################*/
/* TESTS BEGIN */

    /**
     * config_vs_cmd_test_1()
     *
     * We set the bes configuration, via TheBESKeys to allow all of the data in the dataset
     * Then we use the BESContext manager, as a client might submit in an XML command,
     * to set the max response size to be smaller than the requested response, and the
     * max variable size to be smaller than the largest variable.
     */
    void config_vs_cmd_test_1() {
        D4BaseTypeFactory d_d4f;
        D4ParserSax2 dp;
        string file_name = "input-files/test_01.dmr";
        auto d_test_dmr = mk_dmr_from_file(file_name, dp, &d_d4f);

        // Apply Constraint to the DMR (Mark all)
        d_test_dmr->root()->set_send_p(true);

        // Setting these in TheBESKeys is like setting it in the bes configuration files.
        TheBESKeys::TheKeys()->set_key(BES_KEYS_MAX_RESPONSE_SIZE_KEY,"2048");
        TheBESKeys::TheKeys()->set_key(BES_KEYS_MAX_VAR_SIZE_KEY,"512");
        try {
            dap_utils::throw_if_too_big(*(d_test_dmr.get()), __FILE__, __LINE__);
            DBG(cerr << prolog << "SUCCESS: The response was deemed acceptable.\n");
        }
        catch (BESSyntaxUserError &bsue) {
            DBG( cerr << prolog << "ERROR: Caught BESSyntaxUserError " + bsue.get_message() + '\n');
            CPPUNIT_FAIL("ERROR: throw_if_too_big() threw BESSyntaxUserError "
                         "and it shouldn't have. msg: "+bsue.get_message());
        }

        // Setting these values in the BESContextManager is equivalent to sending them
        // in an XML command from the OLFS, bescmdln, or besstandalone.
        BESContextManager::TheManager()->set_context(BES_CONTEXT_MAX_RESPONSE_SIZE_KEY,"200");
        BESContextManager::TheManager()->set_context(BES_CONTEXT_MAX_VAR_SIZE_KEY,"100");

        try {
            dap_utils::throw_if_too_big(*(d_test_dmr.get()), __FILE__, __LINE__);
            CPPUNIT_FAIL("ERROR: Failed to throw exception for test dmr '" + file_name + "'");
        }
        catch (BESSyntaxUserError &bsue) {
            DBG(cerr << prolog <<"SUCCESS: Caught BESSyntaxUserError. message: \n" + bsue.get_message() + '\n');
        }

    }



    /**
     * config_vs_cmd_test_2()
     *
     * We set the bes configuration, via TheBESKeys to not allow of the data in the dataset
     * to be requested at once and to deny the largest variables. Then, we use the BESContext
     * manager to set the max response size to be larger what was set in TheBESKeys, and the
     * max variable size to be larger than what  was set in TheBESKeys. The result should be a no-op
     * The TheBESKeys more restrictive setting should prevail over the remote clients request.
     *
     */
    void config_vs_cmd_test_2() {
        D4BaseTypeFactory d_d4f;
        D4ParserSax2 dp;
        string file_name = "input-files/test_01.dmr";
        auto d_test_dmr = mk_dmr_from_file(file_name, dp, &d_d4f);

        // Apply Constraint to the DMR (Mark all)
        d_test_dmr->root()->set_send_p(true);

        // Setting these in TheBESKeys is like setting it in the bes configuration files.
        TheBESKeys::TheKeys()->set_key(BES_KEYS_MAX_RESPONSE_SIZE_KEY,"1015");
        TheBESKeys::TheKeys()->set_key(BES_KEYS_MAX_VAR_SIZE_KEY,"287");

        // Setting these values in the BESContextManager is equivalent to sending them
        // in an XML command from the OLFS, bescmdln, or besstandalone.
        BESContextManager::TheManager()->set_context(BES_CONTEXT_MAX_RESPONSE_SIZE_KEY,"1500`");
        BESContextManager::TheManager()->set_context(BES_CONTEXT_MAX_VAR_SIZE_KEY,"512");

        try {
            dap_utils::throw_if_too_big(*(d_test_dmr.get()), __FILE__, __LINE__);
            CPPUNIT_FAIL("ERROR: Failed to throw exception for test dmr '" + file_name + "'");
        }
        catch (BESSyntaxUserError &bsue) {
            DBG(cerr << prolog <<"SUCCESS: Caught BESSyntaxUserError. message: \n" + bsue.get_message() + '\n');
        }
    }

    void check_dap4_max_sizes_test()
    {
        uint64_t max_response_size_bytes;
        uint64_t max_var_size_bytes;

        // Setting these in TheBESKeys is like setting it in the bes configuration files.
        TheBESKeys::TheKeys()->set_key(BES_KEYS_MAX_RESPONSE_SIZE_KEY,"200");
        TheBESKeys::TheKeys()->set_key(BES_KEYS_MAX_VAR_SIZE_KEY,"100");

        dap_utils::get_max_sizes_bytes(max_response_size_bytes, max_var_size_bytes, true);

        CPPUNIT_ASSERT(max_response_size_bytes == 200);
        CPPUNIT_ASSERT(max_var_size_bytes == 100);
    }

    void check_dap2_max_sizes_test()
    {
        uint64_t max_response_size_bytes;
        uint64_t max_var_size_bytes;
        bool is_dap2 = false;

        dap_utils::get_max_sizes_bytes(max_response_size_bytes, max_var_size_bytes, true);

        CPPUNIT_ASSERT(max_response_size_bytes == 0);
        CPPUNIT_ASSERT(max_var_size_bytes == twoGB);

        // Setting these in TheBESKeys is like setting it in the bes configuration files.
        TheBESKeys::TheKeys()->set_key(BES_KEYS_MAX_RESPONSE_SIZE_KEY, to_string(fourGB));
        TheBESKeys::TheKeys()->set_key(BES_KEYS_MAX_VAR_SIZE_KEY,to_string(fourGB));

        dap_utils::get_max_sizes_bytes(max_response_size_bytes, max_var_size_bytes, true);

        CPPUNIT_ASSERT(max_response_size_bytes == fourGB);
        CPPUNIT_ASSERT(max_var_size_bytes == twoGB);
    }



    void throw_if_dds_response_too_big_test() {
        D4BaseTypeFactory d_d4f;
        D4ParserSax2 dp;
        string file_name = "input-files/test_01.dmr";
        auto d_test_dmr = mk_dmr_from_file(file_name, dp, &d_d4f);

        // Apply Constraint to the DMR (Mark all)
        d_test_dmr->root()->set_send_p(true);

        auto test_dds = d_test_dmr->getDDS();

        // Setting these in TheBESKeys is like setting it in the bes configuration files.
        TheBESKeys::TheKeys()->set_key(BES_KEYS_MAX_RESPONSE_SIZE_KEY,"200");
        TheBESKeys::TheKeys()->set_key(BES_KEYS_MAX_VAR_SIZE_KEY,"100");

        try {
            dap_utils::throw_if_too_big(*test_dds, __FILE__, __LINE__);
            CPPUNIT_FAIL("ERROR: Failed to throw exception for test dmr '" + file_name + "'");
        }
        catch (BESSyntaxUserError &bsue) {
            DBG(cerr << prolog <<"SUCCESS: Caught BESSyntaxUserError. message: \n" + bsue.get_message() + '\n');
        }

        delete test_dds;
    }

    /**
     * Response too big
     * Variable(s) too big.
     */
    void throw_if_dmr_too_big_test_RV() {
        D4BaseTypeFactory d_d4f;
        D4ParserSax2 dp;
        string file_name = "input-files/test_01.dmr";
        auto d_test_dmr = mk_dmr_from_file(file_name, dp, &d_d4f);

        // Apply Constraint to the DMR (Mark all)
        d_test_dmr->root()->set_send_p(true);

        // Setting these in TheBESKeys is like setting it in the bes configuration files.
        TheBESKeys::TheKeys()->set_key(BES_KEYS_MAX_RESPONSE_SIZE_KEY,"200");
        TheBESKeys::TheKeys()->set_key(BES_KEYS_MAX_VAR_SIZE_KEY,"100");

        try {
            dap_utils::throw_if_too_big(*(d_test_dmr.get()), __FILE__, __LINE__);
            CPPUNIT_FAIL("ERROR: Failed to throw exception for test dmr '" + file_name + "'");
        }
        catch (BESSyntaxUserError &bsue) {
            DBG(cerr << prolog <<"SUCCESS: Caught BESSyntaxUserError. message: \n" + bsue.get_message() + '\n');
        }
    }

    /**
     * Response size ok
     * Variable(s) too big.
     */
    void throw_if_dmr_too_big_test_rV() {
        D4BaseTypeFactory d_d4f;
        D4ParserSax2 dp;
        string file_name = "input-files/test_01.dmr";
        auto d_test_dmr = mk_dmr_from_file(file_name, dp, &d_d4f);

        // Apply Constraint to the DMR (Mark all)
        d_test_dmr->root()->set_send_p(true);

        // Setting these in TheBESKeys is like setting it in the bes configuration files.
        TheBESKeys::TheKeys()->set_key(BES_KEYS_MAX_RESPONSE_SIZE_KEY,"1024");
        TheBESKeys::TheKeys()->set_key(BES_KEYS_MAX_VAR_SIZE_KEY,"100");

        try {
            dap_utils::throw_if_too_big(*(d_test_dmr.get()), __FILE__, __LINE__);
            CPPUNIT_FAIL("ERROR: Failed to throw exception for test dmr '" + file_name + "'");
        }
        catch (BESSyntaxUserError &bsue) {
            DBG(cerr << prolog << "SUCCESS: Caught BESSyntaxUserError. message: \n" + bsue.get_message() + '\n');
        }
    }


    /**
     * Response too big
     * Variable(s) size(s) ok.
     */
    void throw_if_dmr_too_big_test_Rv() {
        D4BaseTypeFactory d_d4f;
        D4ParserSax2 dp;
        string file_name = "input-files/test_01.dmr";
        auto d_test_dmr = mk_dmr_from_file(file_name, dp, &d_d4f);

        // Apply Constraint to the DMR (Mark all)
        d_test_dmr->root()->set_send_p(true);

        // Setting these in TheBESKeys is like setting it in the bes configuration files.
        TheBESKeys::TheKeys()->set_key(BES_KEYS_MAX_RESPONSE_SIZE_KEY,"20");
        TheBESKeys::TheKeys()->set_key(BES_KEYS_MAX_VAR_SIZE_KEY,"10000");

        try {
            dap_utils::throw_if_too_big(*(d_test_dmr.get()), __FILE__, __LINE__);
            CPPUNIT_FAIL("ERROR: Failed to throw exception for test dmr '" + file_name + "'");
        }
        catch (BESSyntaxUserError &bsue) {
            DBG(cerr << prolog << "SUCCESS: Caught BESSyntaxUserError. message: \n" + bsue.get_message() + '\n');
        }
    }

    /**
     * Response size ok
     * Variable(s) size(s) ok.
     */
    void throw_if_dmr_too_big_test_rv() {
        D4BaseTypeFactory d_d4f;
        D4ParserSax2 dp;
        string file_name = "input-files/test_01.dmr";
        auto d_test_dmr = mk_dmr_from_file(file_name, dp, &d_d4f);

        // Apply Constraint to the DMR (Mark all)
        d_test_dmr->root()->set_send_p(true);

        // Setting these in TheBESKeys is like setting it in the bes configuration files.
        TheBESKeys::TheKeys()->set_key(BES_KEYS_MAX_RESPONSE_SIZE_KEY,"10000");
        TheBESKeys::TheKeys()->set_key(BES_KEYS_MAX_VAR_SIZE_KEY,"10000");

        try {
            dap_utils::throw_if_too_big(*(d_test_dmr.get()), __FILE__, __LINE__);
            DBG(cerr << prolog << "SUCCESS: The response was deemed acceptable.\n");
        }
        catch (BESSyntaxUserError &bsue) {
            DBG( cerr << prolog << "ERROR: Caught BESSyntaxUserError " + bsue.get_message() + '\n');
            CPPUNIT_FAIL("ERROR: throw_if_too_big() threw BESSyntaxUserError "
                         "and it shouldn't have. msg: "+bsue.get_message());
        }
    }


    void var_too_big_test() {

        D4BaseTypeFactory d_d4f;
        D4ParserSax2 dp;
        string file_name = "input-files/test_01.dmr";
        auto d_test_dmr = mk_dmr_from_file(file_name, dp, &d_d4f);

        // Apply Constraint to the DMR (Mark all)
        d_test_dmr->root()->set_send_p(true);

        stringstream msg;
        uint64_t response_size = 0;
        uint64_t expected_response_size = 1016;
        uint64_t max_var_size = 200;
        std::vector<std::string> too_big;

        response_size =  dap_utils::compute_response_size_and_inv_big_vars( *(d_test_dmr.get()), max_var_size, too_big);
        msg << prolog << "response_size: " << response_size  << " (expected: " << expected_response_size << ")" << endl;
        DBG( cerr << msg.str());

        // We can't test response size because the response contains arrays of strings and the string sizes
        // differ from one system to the next, example OS-X: 24 bytes, centos-8: 32 bytes

        if(!too_big.empty()){
            DBG( cerr << prolog << "Found " << too_big.size() <<  " variables larger than " << max_var_size << " bytes:" << endl);
            for(auto entry:too_big){
                DBG(cerr << prolog << "  " << entry <<  endl);
            }
            CPPUNIT_ASSERT( too_big.size() == 2 );
        }
        else {
            CPPUNIT_FAIL("ERROR: No variables were deemed too big!");
        }
    }


    void dmrpp_var_too_big_test() {

        D4BaseTypeFactory d_d4f;
        D4ParserSax2 dp;
        string file_name = "input-files/tempo_l2.nc.dmrpp";
        auto d_test_dmr = mk_dmr_from_file(file_name, dp, &d_d4f);

        // Apply Constraint to the DMR (Mark all)
        d_test_dmr->root()->set_send_p(true);

        stringstream msg;
        uint64_t response_size = 0;
        uint64_t expected_response_size = 180356500;

        uint64_t max_size = 1000000;
        std::vector<std::string> too_big;

        response_size = dap_utils::compute_response_size_and_inv_big_vars( *(d_test_dmr.get()), max_size, too_big);
        msg << prolog << "response_size: " << response_size  << " (expected: " << expected_response_size << ")" << endl;
        DBG( cerr << msg.str());

        msg.str(string());
        msg  << prolog << "ERROR: Unexpected response_size. expected: " << expected_response_size << " got response_size: " << response_size << endl;
        CPPUNIT_ASSERT_EQUAL_MESSAGE(msg.str(), response_size, expected_response_size);

        if(!too_big.empty()){
            DBG(cerr << prolog << "Found " << too_big.size() <<  " variables larger than " << max_size << " bytes:" << endl);
            for(auto entry:too_big){
                DBG(cerr << prolog << "  " << entry <<  endl);
            }
            CPPUNIT_ASSERT(too_big.size() == 10 );
        }
        else {
            CPPUNIT_FAIL("ERROR: No variables were deemed too big!");
        }
    }

    void dmrpp_constrained_var_too_big_test() {

        D4BaseTypeFactory d_d4f;
        D4ParserSax2 dp;
        string file_name = "input-files/tempo_l2.nc.dmrpp";
        auto d_test_dmr = mk_dmr_from_file(file_name, dp, &d_d4f);

        // Apply Constraint to the DMR
        D4ConstraintEvaluator d4ce(d_test_dmr.get());
        d4ce.parse("/support_data/gas_profile;/support_data/scattering_weights");

        stringstream msg;
        uint64_t response_size = 0;
        uint64_t expected_response_size = 140378112;
        uint64_t max_size = 1000000;
        std::vector<std::string> too_big;


        response_size = dap_utils::compute_response_size_and_inv_big_vars( *(d_test_dmr.get()), max_size, too_big);
        msg << prolog << "response_size: " << response_size  << " (expected: " << expected_response_size << ")" << endl;
        DBG( cerr << msg.str());

        msg.str(string());
        msg  << prolog << "ERROR: Unexpected response_size. expected: " << expected_response_size << " got response_size: " << response_size << endl;
        CPPUNIT_ASSERT_EQUAL_MESSAGE(msg.str(), response_size, expected_response_size);


        if(!too_big.empty()){
            DBG(cerr << prolog << "Found " << too_big.size() <<  " variables larger than " << max_size << " bytes:" << endl);
            for(auto entry:too_big){
                DBG(cerr << prolog << "  " << entry <<  endl);
            }
            CPPUNIT_ASSERT( too_big.size() == 2);
        }
        else {
            CPPUNIT_FAIL(prolog + "ERROR: The constraint reduced the size of the variables to less than the maximum "
                                  "when it should not have done so. No variables were deemed too big!");
        }

    }


    void dmrpp_constrained_var_ok_test() {

        D4BaseTypeFactory d_d4f;
        D4ParserSax2 dp;
        string file_name = "input-files/tempo_l2.nc.dmrpp";
        auto d_test_dmr = mk_dmr_from_file(file_name, dp, &d_d4f);

        // Apply Constraint to the DMR
        D4ConstraintEvaluator d4ce(d_test_dmr.get());
        d4ce.parse("/support_data/gas_profile[1][][];/support_data/scattering_weights[3][][]");

        stringstream msg;
        uint64_t response_size = 0;
        uint64_t expected_response_size = 1179648;
        uint64_t max_size = 1000000;
        std::vector<std::string> too_big;

        response_size = dap_utils::compute_response_size_and_inv_big_vars( *(d_test_dmr.get()), max_size, too_big);
        msg << prolog << "response_size: " << response_size  << " (expected: " << expected_response_size << ")" << endl;
        DBG( cerr << msg.str());

        msg.str(string());
        msg  << prolog << "ERROR: Unexpected response_size. expected: " << expected_response_size << " got response_size: " << response_size << endl;
        CPPUNIT_ASSERT_EQUAL_MESSAGE(msg.str(), response_size, expected_response_size);

        if(!too_big.empty()){
            DBG( cerr << prolog << "Found " << too_big.size() <<  " variables larger than " << max_size << " bytes:" << endl);
            for(auto entry:too_big){
                DBG(cerr << prolog << "  " << entry <<  endl);
            }
            CPPUNIT_FAIL( prolog + "ERROR The applied constraint expression should have reduced the size of the "
                                   "requested variables so that they are no longer too big. That's not ok." );
        }
        else {
            DBG( cerr << prolog << "No variables larger than " << max_size << " bytes were found." << endl);
            CPPUNIT_ASSERT( too_big.empty() );
        }
    }

    void dmrpp_root_group_var_too_big_test() {
        D4BaseTypeFactory d_d4f;
        D4ParserSax2 dp;
        string file_name = "input-files/tempo_l2.nc.dmrpp";
        auto d_test_dmr = mk_dmr_from_file(file_name, dp, &d_d4f);

        // Apply Constraint to the DMR
        D4ConstraintEvaluator d4ce(d_test_dmr.get());
        d4ce.parse("/xtrack;/mirror_step");

        stringstream msg;
        uint64_t response_size = 0;
        uint64_t expected_response_size = 8668;
        uint64_t max_size = 8000;
        std::vector<std::string> too_big;

        response_size = dap_utils::compute_response_size_and_inv_big_vars( *(d_test_dmr.get()), max_size, too_big);
        msg << prolog << "response_size: " << response_size  << " (expected: " << expected_response_size << ")" << endl;
        DBG( cerr << msg.str());

        msg.str(string());
        msg  << prolog << "ERROR: Unexpected response_size. expected: " << expected_response_size << " got response_size: " << response_size << endl;
        CPPUNIT_ASSERT_EQUAL_MESSAGE(msg.str(), response_size, expected_response_size);

        if(!too_big.empty()){
            DBG(cerr << prolog << "Found " << too_big.size() <<  " variables larger than " << max_size << " bytes:" << endl);
            for(auto entry:too_big){
                DBG(cerr << prolog << "  " << entry <<  endl);
            }
            CPPUNIT_ASSERT( too_big.size() == 1);
        }
        else {
            CPPUNIT_FAIL(prolog + "ERROR: The constraint reduced the size of the variables to less than the maximum "
                                  "when it should not have done so. No variables were deemed too big!");
        }

    }


    void dmrpp_constrained_root_group_var_ok_test() {

        D4BaseTypeFactory d_d4f;
        auto d_test_dmr = std::unique_ptr<DMR>(new DMR(&d_d4f));

        D4ParserSax2 dp;
        stringstream msg;
        uint64_t response_size = 0;
        uint64_t expected_response_size = 2524;

        string file_name=BESUtil::pathConcat(TEST_SRC_DIR,"input-files/tempo_l2.nc.dmrpp");
        DBG(cerr << prolog << "DMR file to be parsed: " << file_name << endl);

        fstream in(file_name.c_str(), ios::in|ios::binary);
        dp.intern(in, d_test_dmr.get());
        D4ConstraintEvaluator d4ce(d_test_dmr.get());

        uint64_t max_size = 8000;
        std::vector<std::string> too_big;

        d4ce.parse("/xtrack[1:4:2047];/mirror_step");

        response_size = dap_utils::compute_response_size_and_inv_big_vars( *(d_test_dmr.get()), max_size, too_big);
        msg << prolog << "response_size: " << response_size  << " (expected: " << expected_response_size << ")" << endl;
        DBG( cerr << msg.str());

        msg.str(string());
        msg  << prolog << "ERROR: Unexpected response_size. expected: " << expected_response_size << " got response_size: " << response_size << endl;
        CPPUNIT_ASSERT_EQUAL_MESSAGE(msg.str(), response_size, expected_response_size);

        if(!too_big.empty()){
            DBG( cerr << prolog << "Found " << too_big.size() <<  " variables larger than " << max_size << " bytes:" << endl);
            for(auto apair:too_big){
                for(auto entry:too_big){
                    DBG(cerr << prolog << "  " << entry <<  endl);
                }
            }
            CPPUNIT_FAIL( prolog + "ERROR The applied constraint expression should have reduced the size of the "
                                   "requested variables so that they are no longer too big. That's not ok." );
        }
        else {
            DBG( cerr << prolog << "No variables larger than " << max_size << " bytes were found." << endl);
            CPPUNIT_ASSERT( too_big.empty() );
        }

    }

/* TESTS END */
/*##################################################################################################*/

    CPPUNIT_TEST_SUITE(DapUtilsTest);
    CPPUNIT_TEST(config_vs_cmd_test_1);
    CPPUNIT_TEST(config_vs_cmd_test_2);

    CPPUNIT_TEST(check_dap4_max_sizes_test);
    CPPUNIT_TEST(check_dap2_max_sizes_test);
    CPPUNIT_TEST(throw_if_dds_response_too_big_test);

    CPPUNIT_TEST(var_too_big_test);
    CPPUNIT_TEST(throw_if_dmr_too_big_test_rv);
    CPPUNIT_TEST(throw_if_dmr_too_big_test_Rv);
    CPPUNIT_TEST(throw_if_dmr_too_big_test_rV);
    CPPUNIT_TEST(throw_if_dmr_too_big_test_RV);
    CPPUNIT_TEST(dmrpp_var_too_big_test);
    CPPUNIT_TEST(dmrpp_constrained_var_too_big_test);
    CPPUNIT_TEST(dmrpp_constrained_var_ok_test);
    CPPUNIT_TEST(dmrpp_root_group_var_too_big_test);
    CPPUNIT_TEST(dmrpp_constrained_root_group_var_ok_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(DapUtilsTest);

} // namespace http

int main(int argc, char *argv[]) {
    string bes_debug="cerr,bes,dap_utils,dap_utils_verbose,context";
    return bes_run_tests<dap_utils::DapUtilsTest>(argc, argv, bes_debug) ? 0 : 1;
}

