// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2022 OPeNDAP, Inc.
// Author: Samuel Lloyd <slloyd@opendap.org>
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

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include <cppunit/extensions/HelperMacros.h>

#include <libdap/BaseType.h>
#include <libdap/Int8.h>
#include <libdap/Byte.h>
#include <libdap/Array.h>
#include <libdap/Structure.h>

#include "d4_tools.h"

#include "run_tests_cppunit.h"
#include "test_config.h"

using namespace std;
using namespace d4_tools;
using namespace libdap;

#define prolog std::string("D4ToolsTests::").append(__func__).append("() - ")

class D4ToolsTest: public CppUnit::TestFixture {

private:

public:

    // Called once before everything gets tested
    D4ToolsTest() = default;

    // Called at the end of the test
    ~D4ToolsTest() = default;

    // is_dap4_projected(libdap::BaseType *var, vector<BaseType *> &inv)
    void test_is_dap4_projected_int8(){
        //Int8 b8("b8");
        Int8 b8("b8");
        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(&b8, inv));
        CPPUNIT_ASSERT(inv.size() == 1);
        CPPUNIT_ASSERT(inv.at(0)->name() == "b8");
    }

    void test_is_dap4_projected_byte(){
        Byte b8("b8");
        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(&b8, inv) == false);
        CPPUNIT_ASSERT(inv.size() == 0);
    }

    void test_is_dap4_projected_array(){
        Int8 b8("b8");
        Array a4("b8", &b8);
        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(&a4, inv));
        CPPUNIT_ASSERT(inv.size() == 1);
        CPPUNIT_ASSERT(inv.at(0)->name() == "b8");
    }

    void test_is_dap4_projected_struct(){
        Structure s2("s2");
        Array *a4 = new Array("b8", new Int8("b8"));
        s2.add_var_nocopy(a4);
        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(&s2, inv));
        CPPUNIT_ASSERT(inv.size() == 1);
        CPPUNIT_ASSERT(inv.at(0)->name() == "b8");
    }

    void test_is_dap4_projected_dds1(){
        BaseTypeFactory f;
        DDS *dds = new DDS(&f);
        Array *a4 = new Array("b8", new Int8("b8"));
        a4->set_send_p(true);
        dds->add_var_nocopy(a4);
        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(dds, inv));
        CPPUNIT_ASSERT(inv.size() == 1);
        CPPUNIT_ASSERT(inv.at(0)->name() == "b8");
    }

    CPPUNIT_TEST_SUITE( D4ToolsTest );

    CPPUNIT_TEST(test_is_dap4_projected_int8);
    CPPUNIT_TEST(test_is_dap4_projected_byte);
    CPPUNIT_TEST(test_is_dap4_projected_array);
    CPPUNIT_TEST(test_is_dap4_projected_struct);

    CPPUNIT_TEST(test_is_dap4_projected_dds1);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(D4ToolsTest);

int main(int argc, char *argv[])
{
    return bes_run_tests<D4ToolsTest>(argc, argv, "cerr,fonc") ? 0: 1;
}

