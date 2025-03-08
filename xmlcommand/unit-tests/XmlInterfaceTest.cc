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
#include "BESContextManager.h"
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
    string d_commands_dir = TEST_COMMAND_DIR;

    // Called once before everything gets tested
    XmlInterfaceTest() = default;

    // Called at the end of the test
    ~XmlInterfaceTest() override = default;

    // Called before each test
    void setUp() override {
        debug = true;
        DBG(cerr << "\n");
        DBG(cerr << prolog << "#-----------------------------------------------------------------\n");
        DBG(cerr << prolog << "BEGIN\n");
        string beslog_filename = TEST_BUILD_DIR ;
        beslog_filename.append("/bes.log");

        TheBESKeys::TheKeys()->set_key("BES.LogName",beslog_filename);
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

        const string bescmd_str = BESUtil::file_to_string(command_file_name);
        DBG(cerr << prolog << "BES Command: \n" << bescmd_str << "\n");


        try {
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
/*##################################################################################################*/
/* TESTS BEGIN */

    CPPUNIT_TEST_SUITE(XmlInterfaceTest);

    CPPUNIT_TEST(xml_cmd_test_0);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(XmlInterfaceTest);

int main(int argc, char *argv[]) {
    return bes_run_tests<XmlInterfaceTest>(argc, argv, "cerr,bes,besxml") ? 0 : 1;
}
