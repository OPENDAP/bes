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

#include "BESError.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "TheBESKeys.h"
#include "BESXMLInterface.h"

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

/*##################################################################################################*/
/* TESTS BEGIN */

    CPPUNIT_TEST_SUITE(XmlInterfaceTest);

    CPPUNIT_TEST(xml_cmd_test_0);
    CPPUNIT_TEST(olfs_log_url_path_test);
    CPPUNIT_TEST(olfs_log_url_path_last_field_test);
    CPPUNIT_TEST(olfs_log_url_path_short_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(XmlInterfaceTest);

int main(int argc, char *argv[]) {
    return bes_run_tests<XmlInterfaceTest>(argc, argv, "cerr,bes,besxml") ? 0 : 1;
}
