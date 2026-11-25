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

#include "BESContextManager.h"
#include "BESDataHandlerInterface.h"
#include "BESDebug.h"
#include "BESError.h"
#include "BESFileContainer.h"
#include "BESUtil.h"
#include "BESXMLInterface.h"
#include "TheBESKeys.h"

#include "test_config.h"

// Maybe the common testing code in modules should be moved up one level? jhrg 11/3/22
#include <BESLog.h>
#include <Error.h>

#include "../modules/common/run_tests_cppunit.h"

using namespace std;

#define prolog std::string("# XmlInterfaceTest::").append(__func__).append("() - ")

class XmlInterfaceTest : public CppUnit::TestFixture {

public:
    string d_commands_dir = BESUtil::assemblePath(TEST_SRC_DIR, "cmd");

    // Called once before everything gets tested
    XmlInterfaceTest() = default;

    // Called at the end of the test
    ~XmlInterfaceTest() override = default;

    // Called before each test
    void setUp() override {
        DBG(cerr << "\n");
        DBG(cerr << prolog << "-----------------------------------------------------------------\n");
        DBG(cerr << prolog << "BEGIN\n");

        string beslog_filename = BESUtil::assemblePath(TEST_BUILD_DIR ,"bes.log");
        DBG(cerr << prolog << "beslog_filename: " << beslog_filename << "\n");
        TheBESKeys::TheKeys()->set_key("BES.LogName",beslog_filename);

        DBG(cerr << prolog << "d_commands_dir: " << d_commands_dir << "\n");

        DBG(cerr << prolog << "END\n");
    }

    // Called after each test
    void tearDown() override {
        DBG(cerr << "\n" << prolog << "BEGIN\n");


        DBG(cerr << prolog << "END\n");
    }

    void xml_cmd_test_0(){

        DBG(cerr << prolog << "BEGIN\n");
        bool disabled = true;

        string command_file_name = BESUtil::assemblePath(d_commands_dir , "bes.cmd");
        DBG(cerr << prolog << "command_file_name: " << command_file_name << "\n");

        try {
            const string bescmd_str = BESUtil::file_to_string(command_file_name);
            DBG(cerr << prolog << "BES Command: \n" << bescmd_str << "\n");

            ostringstream oss;
            BESXMLInterface bxi(bescmd_str, &oss);
            if (disabled) {
                cerr << "\n" << prolog << "This XmlInterfaceTest is currently DISABLED\n";
                CPPUNIT_ASSERT(true);
            }
            else {
                DBG(cerr << prolog << "Calling: bxi.build_data_request_plan()\n");
                bxi.build_data_request_plan();
            }
        }
        catch (BESError &bes_error) { // Catch the libdap::Error and throw BESInternalError
            CPPUNIT_FAIL(bes_error.get_verbose_message());
        }
        catch (libdap::Error &libdap_error) { // Catch the libdap::Error and throw BESInternalError
            CPPUNIT_FAIL(libdap_error.get_error_message());
        }
        catch(std::exception &std_error) {
            CPPUNIT_FAIL(std_error.what());
        }

        DBG(cerr << prolog << "END\n");
    }
    // TASK-005: url-path is the 8th field of the olfsLog context value.
    void olfs_log_url_path_test() {
        const string olfs_log = "0:0:0:0:0:0:0:1|&|curl/7.64.1|&|-|&|-|&|1624627728945|&|10|&|HTTP-GET"
                                "|&|/opendap/hyrax/hj/coads_climatology.nc.dap.nc4|&|dap4.ce=SST";
        CPPUNIT_ASSERT_EQUAL(string("/opendap/hyrax/hj/coads_climatology.nc.dap.nc4"),
                             BESXMLInterface::olfs_log_url_path(olfs_log));
    }

    void olfs_log_url_path_last_field_test() {
        const string olfs_log = "ip|&|ua|&|-|&|-|&|1|&|10|&|HTTP-GET|&|/opendap/x.nc.dmr";
        CPPUNIT_ASSERT_EQUAL(string("/opendap/x.nc.dmr"), BESXMLInterface::olfs_log_url_path(olfs_log));
    }

    void olfs_log_url_path_short_test() {
        CPPUNIT_ASSERT_EQUAL(string(""), BESXMLInterface::olfs_log_url_path("ip|&|ua|&|-|&|-|&|1|&|10|&|HTTP-GET"));
        CPPUNIT_ASSERT_EQUAL(string(""), BESXMLInterface::olfs_log_url_path("not an olfs log line"));
        CPPUNIT_ASSERT_EQUAL(string(""), BESXMLInterface::olfs_log_url_path(""));
    }

    // log_the_command() caches the request's URL path and CE in BESLog for 'error'
    // records. These helpers run it for a get command with one container and return what
    // was cached. The test owns the container; the DHI does not delete its containers.
    static void run_log_the_command(const string &action, const string &dap2_ce, const string &dap4_ce,
                                    const string &dap4_function) {
        ostringstream oss;
        BESXMLInterface bxi("", &oss);
        unique_ptr<BESFileContainer> c(new BESFileContainer("c", "/data/fnoc1.nc", "nc"));
        c->set_constraint(dap2_ce);
        c->set_dap4_constraint(dap4_ce);
        c->set_dap4_function(dap4_function);

        bxi.d_dhi_ptr->action = action;
        bxi.d_dhi_ptr->containers.push_back(c.get());

        BESLog::TheLog()->reset_request_url_and_ce();
        bxi.log_the_command();
        bxi.d_dhi_ptr->containers.clear();
    }

    static string logged_ce(const string &dap2_ce, const string &dap4_ce = "", const string &dap4_function = "") {
        run_log_the_command("get.dods", dap2_ce, dap4_ce, dap4_function);
        return BESLog::TheLog()->get_request_ce();
    }

    static string logged_url() {
        run_log_the_command("get.dods", "", "", "");
        return BESLog::TheLog()->get_request_url();
    }

    static void clear_url_contexts() {
        BESContextManager::TheManager()->unset_context("olfsLog");
        BESContextManager::TheManager()->unset_context("xml:base");
    }

    void log_ce_dap2_projection_test() { CPPUNIT_ASSERT_EQUAL(string("u[0:1],v"), logged_ce("u[0:1],v")); }

    void log_ce_dap2_selection_test() {
        CPPUNIT_ASSERT_EQUAL(string("u,v&u>3&v<\"abc\""), logged_ce("u,v&u>3&v<\"abc\""));
    }

    void log_ce_dap2_function_test() {
        CPPUNIT_ASSERT_EQUAL(string("geogrid(SST,62,206,56,210)"), logged_ce("geogrid(SST,62,206,56,210)"));
    }

    // When a container has both, the DAP2 CE is logged, matching the 'request' record.
    void log_ce_dap2_preferred_over_dap4_test() {
        CPPUNIT_ASSERT_EQUAL(string("u[0:1]"), logged_ce("u[0:1]", "/u[0:1]", "f(u)"));
    }

    void log_ce_dap4_test() {
        CPPUNIT_ASSERT_EQUAL(string("/SST[0:1][2:3];/lat"), logged_ce("", "/SST[0:1][2:3];/lat"));
    }

    void log_ce_dap4_with_function_test() {
        CPPUNIT_ASSERT_EQUAL(string("/SST dap4.function=linear_scale(SST)"),
                             logged_ce("", "/SST", "linear_scale(SST)"));
    }

    void log_ce_dap4_function_only_test() {
        CPPUNIT_ASSERT_EQUAL(string("dap4.function=version()"), logged_ce("", "", "version()"));
    }

    void log_ce_none_test() { CPPUNIT_ASSERT_EQUAL(string("-"), logged_ce("")); }

    // Only get commands set the URL and CE (LOG_ONLY_GET_COMMANDS).
    void log_ce_not_get_command_test() {
        run_log_the_command("show.catalog", "u[0:1]", "", "");
        CPPUNIT_ASSERT_EQUAL(string("-"), BESLog::TheLog()->get_request_ce());
    }

    void log_url_from_olfs_log_test() {
        clear_url_contexts();
        BESContextManager::TheManager()->set_context(
            "olfsLog", "ip|&|ua|&|-|&|-|&|1|&|10|&|HTTP-GET|&|/opendap/data/nc/fnoc1.nc.dods|&|u[0:1]");
        BESContextManager::TheManager()->set_context("xml:base", "http://localhost:8080/opendap/data/nc/fnoc1.nc");
        const string url = logged_url();
        clear_url_contexts();
        CPPUNIT_ASSERT_EQUAL(string("/opendap/data/nc/fnoc1.nc.dods"), url);
    }

    void log_url_from_xml_base_test() {
        clear_url_contexts();
        BESContextManager::TheManager()->set_context("xml:base", "http://localhost:8080/opendap/data/nc/fnoc1.nc");
        const string url = logged_url();
        clear_url_contexts();
        CPPUNIT_ASSERT_EQUAL(string("http://localhost:8080/opendap/data/nc/fnoc1.nc"), url);
    }

    void log_url_none_test() {
        clear_url_contexts();
        CPPUNIT_ASSERT_EQUAL(string("-"), logged_url());
    }

/*##################################################################################################*/
/* TESTS BEGIN */

    CPPUNIT_TEST_SUITE(XmlInterfaceTest);

    CPPUNIT_TEST(xml_cmd_test_0);
    CPPUNIT_TEST(olfs_log_url_path_test);
    CPPUNIT_TEST(olfs_log_url_path_last_field_test);
    CPPUNIT_TEST(olfs_log_url_path_short_test);
    CPPUNIT_TEST(log_ce_dap2_projection_test);
    CPPUNIT_TEST(log_ce_dap2_selection_test);
    CPPUNIT_TEST(log_ce_dap2_function_test);
    CPPUNIT_TEST(log_ce_dap2_preferred_over_dap4_test);
    CPPUNIT_TEST(log_ce_dap4_test);
    CPPUNIT_TEST(log_ce_dap4_with_function_test);
    CPPUNIT_TEST(log_ce_dap4_function_only_test);
    CPPUNIT_TEST(log_ce_none_test);
    CPPUNIT_TEST(log_ce_not_get_command_test);
    CPPUNIT_TEST(log_url_from_olfs_log_test);
    CPPUNIT_TEST(log_url_from_xml_base_test);
    CPPUNIT_TEST(log_url_none_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(XmlInterfaceTest);

int main(int argc, char *argv[]) {
    return bes_run_tests<XmlInterfaceTest>(argc, argv, "cerr,bes,besxml") ? 0 : 1;
}
