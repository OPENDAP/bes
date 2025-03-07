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
#include "modules/common/run_tests_cppunit.h"

using namespace std;

#define prolog std::string("# XmlInterfaceTest::").append(__func__).append("() - ")

namespace http {

class XmlInterfaceTest : public CppUnit::TestFixture {

public:
    string d_commands_dir = TEST_COMMAND_DIR;

    // Called once before everything gets tested
    XmlInterfaceTest() = default;

    // Called at the end of the test
    ~XmlInterfaceTest() override = default;

    // Called before each test
    void setUp() override {
        DBG(cerr << "\n");
        DBG(cerr << prolog << "#-----------------------------------------------------------------\n");
        DBG(cerr << prolog << "BEGIN\n");


        DBG(cerr << prolog << "END\n");
    }

    // Called after each test
    void tearDown() override {
        DBG(cerr << prolog << "BEGIN\n");


        DBG(cerr << prolog << "END\n");
    }

    void xml_cmd_test_0(){
        DBG(cerr << prolog << "BEGIN\n");

        const string xml_doc_str = BESUtil::file_to_string(d_commands_dir + "/bes.cmd");

        ostringstream oss;

        BESXMLInterface bxi(xml_doc_str, &oss);

        // bxi.build_data_request_plan();

        DBG(cerr << prolog << "END\n");
    }
/*##################################################################################################*/
/* TESTS BEGIN */

    CPPUNIT_TEST_SUITE(XmlInterfaceTest);

    CPPUNIT_TEST(xml_cmd_test_0);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(XmlInterfaceTest);

} // namespace http

int main(int argc, char *argv[]) {
    return bes_run_tests<http::XmlInterfaceTest>(argc, argv, "cerr,bes,besxml") ? 0 : 1;
}
