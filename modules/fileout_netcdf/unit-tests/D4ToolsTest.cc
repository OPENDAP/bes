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
#include <libdap/Int64.h>
#include <libdap/Byte.h>
#include <libdap/Array.h>
#include <libdap/Structure.h>
#include <libdap/D4Attributes.h>

#include <libdap/D4BaseTypeFactory.h>
#include <libdap/DMR.h>

#include "d4_tools.h"
#include "DapObj.h"
#include "D4Group.h"

#include "run_tests_cppunit.h"

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

    /////////////////////////////////////////////////////////////////
    /// DAP2 AND DDS TESTS
    /////////////////////////////////////////////

    /**
     * basic variable test - true
     * tests if dap4 variables returns true
     */
    void test_is_dap4_projected_int8(){
        Int8 int8("int8");

        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(&int8, inv));
        CPPUNIT_ASSERT(inv.size() == 1);
        CPPUNIT_ASSERT(inv.at(0)->name() == "int8");
    }

    /**
     * basic variable test - false
     * tests if non-dap4 variables returns false
     */
    void test_is_dap4_projected_byte(){
        Byte byte("byte");

        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(&byte, inv) == false);
        CPPUNIT_ASSERT(inv.size() == 0);
    }

    /**
     * array variable test - true
     * tests if array containing dap4 vars returns true
     */
    void test_is_dap4_projected_array_int8(){
        Int8 int8("int8");
        Array array("array", &int8);

        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(&array, inv));
        CPPUNIT_ASSERT(inv.size() == 1);
        CPPUNIT_ASSERT(inv.at(0)->name() == "int8");
    }

    /**
     * array variable test - false
     * test if array containing no dap4 vars returns false
     */
     void test_is_dap4_projected_array_byte(){
         Byte byte("byte");
         Array array("array", &byte);

         vector<BaseType *> inv;
         CPPUNIT_ASSERT(is_dap4_projected(&array, inv) == false);
         CPPUNIT_ASSERT(inv.size() == 0);
     }

    /**
     * struct variable test - true
     * test if struct containing dap4 vars returns true
     */
    void test_is_dap4_projected_struct_int8(){
        Structure structure("struct");
        Array *array = new Array("array", new Int8("int8"));
        structure.add_var_nocopy(array);

        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(&structure, inv));
        CPPUNIT_ASSERT(inv.size() == 1);
        CPPUNIT_ASSERT(inv.at(0)->name() == "int8");
    }

    /**
     * struct variable test - false
     * test if struct containing no dap4 vars returns false
     */
    void test_is_dap4_projected_struct_byte(){
        Structure structure("struct");
        Array *array = new Array("array", new Byte("byte"));
        structure.add_var_nocopy(array);

        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(&structure, inv) == false);
        CPPUNIT_ASSERT(inv.size() == 0);
    }

    /**
     * DDS variable test - true
     * test if DDS containing dap4 vars returns true
     */
    void test_is_dap4_projected_dds_int8(){
        Array *array = new Array("array", new Int8("int8"));
        array->set_send_p(true);

        BaseTypeFactory f;
        DDS *dds = new DDS(&f);
        dds->add_var_nocopy(array);

        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(dds, inv));
        CPPUNIT_ASSERT(inv.size() == 1);
        CPPUNIT_ASSERT(inv.at(0)->name() == "int8");
    }

    /**
     * DDS variable test - false
     * test if DDS containing no dap4 vars returns false
     */
    void test_is_dap4_projected_dds_byte(){
        Array *array = new Array("array", new Byte("byte"));
        array->set_send_p(true);

        BaseTypeFactory f;
        DDS *dds = new DDS(&f);
        dds->add_var_nocopy(array);

        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(dds, inv) == false);
        CPPUNIT_ASSERT(inv.size() == 0);
    }

    /**
     * basic variable attribute test - true
     * test if a basic var with dap4 attribute returns true
     */
    void test_is_dap4_projected_attr_true(){
        Byte byte("byte");
        D4Attribute* d4a = new D4Attribute("d4a", attr_int8_c);
        byte.attributes()->add_attribute(d4a);

        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(&byte, inv));
        CPPUNIT_ASSERT(inv.size() == 1);
        CPPUNIT_ASSERT(inv.at(0)->name() == "byte");
    }

    /**
    * basic variable attribute test - false
    * test if a basic var with no dap4 attribute returns false
    */
    void test_is_dap4_projected_attr_false(){
        Byte byte("byte");
        D4Attribute* d4a = new D4Attribute("d4a", attr_byte_c);
        byte.attributes()->add_attribute(d4a);

        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(&byte, inv) == false);
        CPPUNIT_ASSERT(inv.size() == 0);
    }

    /**
    * array attribute test - true
    * test if an array with dap4 attribute returns true
    */
    void test_is_dap4_projected_attr_array_true(){
        Byte byte("byte");
        Array array("array", &byte);
        D4Attribute* d4a = new D4Attribute("d4a", attr_int8_c);
        array.attributes()->add_attribute(d4a);

        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(&array, inv));
        CPPUNIT_ASSERT(inv.size() == 1);
        CPPUNIT_ASSERT(inv.at(0)->name() == "byte");
    }

    /**
    * array attribute test - false
    * test if an array with no dap4 attribute returns false
    */
    void test_is_dap4_projected_attr_array_false(){
        Byte byte("byte");
        Array array("array", &byte);
        D4Attribute* d4a = new D4Attribute("d4a", attr_byte_c);
        array.attributes()->add_attribute(d4a);

        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(&array, inv) == false);
        CPPUNIT_ASSERT(inv.size() == 0);
    }

    /**
    * struct attribute test - true
    * test if a struct with dap4 attribute returns true
    */
    void test_is_dap4_projected_attr_struct_true(){
        Structure structure("struct");
        Array *array = new Array("array", new Byte("byte"));
        D4Attribute* d4a = new D4Attribute("d4a", attr_int8_c);
        array->attributes()->add_attribute(d4a);
        structure.add_var_nocopy(array);

        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(&structure, inv));
        CPPUNIT_ASSERT(inv.size() == 1);
        CPPUNIT_ASSERT(inv.at(0)->name() == "byte");
    }

    /**
    * struct attribute test - false
    * test if a struct with no dap4 attribute returns false
    */
    void test_is_dap4_projected_attr_struct_false(){
        Structure structure("struct");
        Array *array = new Array("array", new Byte("byte"));
        D4Attribute* d4a = new D4Attribute("d4a", attr_byte_c);
        array->attributes()->add_attribute(d4a);
        structure.add_var_nocopy(array);

        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(&structure, inv) == false);
        CPPUNIT_ASSERT(inv.size() == 0);
    }

    /**
    * DDS attribute test - true
    * test if a DDS with dap4 attribute returns true
    */
    void test_is_dap4_projected_attr_dds_true(){
        Array *array = new Array("array", new Byte("byte"));
        array->set_send_p(true);
        D4Attribute* d4a = new D4Attribute("d4a", attr_int8_c);
        array->attributes()->add_attribute(d4a);

        BaseTypeFactory f;
        DDS *dds = new DDS(&f);
        dds->add_var_nocopy(array);

        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(dds, inv));
        CPPUNIT_ASSERT(inv.size() == 1);
        CPPUNIT_ASSERT(inv.at(0)->name() == "byte");
    }

    /**
    * DDS attribute test - false
    * test if a DDS with no dap4 attribute returns false
    */
    void test_is_dap4_projected_attr_dds_false(){
        Array *array = new Array("array", new Byte("byte"));
        array->set_send_p(true);
        D4Attribute* d4a = new D4Attribute("d4a", attr_byte_c);
        array->attributes()->add_attribute(d4a);

        BaseTypeFactory f;
        DDS *dds = new DDS(&f);
        dds->add_var_nocopy(array);

        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(dds, inv) == false);
        CPPUNIT_ASSERT(inv.size() == 0);
    }

    /////////////////////////////////////////////////////////////////
    /// DAP4 AND DMR TESTS
    /////////////////////////////////////////////

    /**
     * DMR Int8 variable test - true
     * test if DMR containing dap4 var [Int8] returns true
     */
    void test_is_dap4_projected_dmr_ddsint8(){
        Int8 *int8 = new Int8("int8");
        int8->set_send_p(true);

        BaseTypeFactory f;
        DDS *dds = new DDS(&f);
        dds->add_var_nocopy(int8);

        D4BaseTypeFactory f4;
        DMR *dmr = new DMR(&f4, *dds);

        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(dmr, inv));
        CPPUNIT_ASSERT(inv.size() == 1);
        CPPUNIT_ASSERT(inv.at(0)->name() == "int8");
    }

    /**
     * DMR Int8 variable test - true
     * test if DMR containing dap4 var [Int8] returns true
     */
    void test_is_dap4_projected_dmr_ddsbyte(){
        Byte *byte = new Byte("byte");
        byte->set_send_p(true);

        BaseTypeFactory f;
        DDS *dds = new DDS(&f);
        dds->add_var_nocopy(byte);

        D4BaseTypeFactory f4;
        DMR *dmr = new DMR(&f4, *dds);

        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(dmr, inv) == false);
        CPPUNIT_ASSERT(inv.size() == 0);
    }

#if 0
    /**
     * DMR variable test - true
     * test if DMR containing dap4 vars returns true
     */
    void test_is_dap4_projected_dmr_ddsarray(){
        BaseTypeFactory f;
        DDS *dds = new DDS(&f);

        Array *a4 = new Array("b8", new Int8("b8"));
        a4->set_send_p(true);
        dds->add_var_nocopy(a4);

        D4BaseTypeFactory f4;
        DMR *dmr = new DMR(&f4, *dds);

        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(dmr, inv));
        CPPUNIT_ASSERT(inv.size() == 1);
        CPPUNIT_ASSERT(inv.at(0)->name() == "b8");
    }
#endif

    /**
     * DMR variable test - true
     * test if DMR containing a dap4 int8 var returns true
     */
    void test_is_dap4_projected_dmr_int8(){
        Int8 *int8 = new Int8("int8");
        int8->set_send_p(true);

        D4BaseTypeFactory f4;
        DMR *dmr = new DMR(&f4, "test");

        D4Group *d4g = dmr->root();
        d4g->add_var_nocopy(int8);

        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(dmr, inv));
        CPPUNIT_ASSERT(inv.size() == 1);
        CPPUNIT_ASSERT(inv.at(0)->name() == "int8");
    }

    /**
     * DMR variable test - true
     * test if DMR containing a dap4 int64 var returns true
     */
    void test_is_dap4_projected_dmr_int64(){
        Int64 *int64 = new Int64("int64");
        int64->set_send_p(true);

        D4BaseTypeFactory f4;
        DMR *dmr = new DMR(&f4, "test");

        D4Group *d4g = dmr->root();
        d4g->add_var_nocopy(int64);

        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(dmr, inv));
        CPPUNIT_ASSERT(inv.size() == 1);
        CPPUNIT_ASSERT(inv.at(0)->name() == "int64");
    }

    /**
     * DMR variable test - false
     * test if DMR not containing a dap4 var returns false
     */
    void test_is_dap4_projected_dmr_byte(){

        Byte *byte = new Byte("byte");
        byte->set_send_p(true);

        D4BaseTypeFactory f4;
        DMR *dmr = new DMR(&f4, "test");

        D4Group *d4g = dmr->root();
        d4g->add_var_nocopy(byte);

        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(dmr, inv) == false);
        CPPUNIT_ASSERT(inv.size() == 0);
    }

    /**
     * dmr array variable test - true
     * tests if a dmr with an array containing a dap4 vars returns true
     */
    void test_is_dap4_projected_dmr_array_int8(){
        Array *array = new Array("array", new Int8("int8"));
        array->set_send_p(true);

        D4BaseTypeFactory f4;
        DMR *dmr = new DMR(&f4, "test");

        D4Group *d4g = dmr->root();
        d4g->add_var_nocopy(array);

        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(dmr, inv));
        CPPUNIT_ASSERT(inv.size() == 1);
        CPPUNIT_ASSERT(inv.at(0)->name() == "int8");
    }

    /**
     * dmr array variable test - false
     * tests if a dmr with an array not containing a dap4 var returns false
     */
    void test_is_dap4_projected_dmr_array_byte(){
        Array *array = new Array("array", new Byte("byte"));
        array->set_send_p(true);

        D4BaseTypeFactory f4;
        DMR *dmr = new DMR(&f4, "test");

        D4Group *d4g = dmr->root();
        d4g->add_var_nocopy(array);

        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(dmr, inv) == false);
        CPPUNIT_ASSERT(inv.size() == 0);
    }

    /**
     * dmr struct variable test - true
     * test if a dmr with a struct containing dap4 vars returns true
     */
    void test_is_dap4_projected_dmr_struct_int8(){
        Structure structure("struct");
        Array *array = new Array("array", new Int8("int8"));
        structure.set_send_p(true);
        structure.add_var_nocopy(array);

        D4BaseTypeFactory f4;
        DMR *dmr = new DMR(&f4, "test");

        D4Group *d4g = dmr->root();
        d4g->add_var_nocopy(&structure);

        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(dmr, inv));
        CPPUNIT_ASSERT(inv.size() == 1);
        CPPUNIT_ASSERT(inv.at(0)->name() == "int8");
    }

    /**
     * dmr struct variable test - false
     * test if a dmr with a struct not containing dap4 vars returns false
     */
    void test_is_dap4_projected_dmr_struct_byte(){
        Structure structure("struct");
        Array *array = new Array("array", new Byte("byte"));
        structure.set_send_p(true);
        structure.add_var_nocopy(array);

        D4BaseTypeFactory f4;
        DMR *dmr = new DMR(&f4, "test");

        D4Group *d4g = dmr->root();
        d4g->add_var_nocopy(array);

        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(dmr, inv) == false);
        CPPUNIT_ASSERT(inv.size() == 0);
    }

    /**
     * dmr basic variable attribute test - true
     * test if a dmr holding a basic var with dap4 attribute returns true
     */
    void test_is_dap4_projected_dmr_attr_true(){
        Byte byte("byte");
        D4Attribute* d4a = new D4Attribute("d4a", attr_int8_c);
        byte.attributes()->add_attribute(d4a);
        byte.set_send_p(true);

        D4BaseTypeFactory f4;
        DMR *dmr = new DMR(&f4, "test");

        D4Group *d4g = dmr->root();
        d4g->add_var_nocopy(&byte);

        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(dmr, inv));
        CPPUNIT_ASSERT(inv.size() == 1);
        CPPUNIT_ASSERT(inv.at(0)->name() == "byte");
    }

    /**
    * dmr basic variable attribute test - false
    * test if a dmr holding a basic var without dap4 attribute returns false
    */
    void test_is_dap4_projected_dmr_attr_false(){
        Byte byte("byte");
        D4Attribute* d4a = new D4Attribute("d4a", attr_byte_c);
        byte.attributes()->add_attribute(d4a);
        byte.set_send_p(true);

        D4BaseTypeFactory f4;
        DMR *dmr = new DMR(&f4, "test");

        D4Group *d4g = dmr->root();
        d4g->add_var_nocopy(&byte);

        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(dmr, inv) == false);
        CPPUNIT_ASSERT(inv.size() == 0);
    }

    /**
    * dmr array attribute test - true
    * test if a dmr holding an array with dap4 attribute returns true
    */
    void test_is_dap4_projected_dmr_attr_array_true(){
        Byte byte("byte");
        Array array("array", &byte);
        D4Attribute* d4a = new D4Attribute("d4a", attr_int8_c);
        array.attributes()->add_attribute(d4a);
        array.set_send_p(true);

        D4BaseTypeFactory f4;
        DMR *dmr = new DMR(&f4, "test");

        D4Group *d4g = dmr->root();
        d4g->add_var_nocopy(&array);

        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(dmr, inv));
        CPPUNIT_ASSERT(inv.size() == 1);
        CPPUNIT_ASSERT(inv.at(0)->name() == "byte");
    }

    /**
    * dmr array attribute test - false
    * test if a dmr holding an array without dap4 attribute returns false
    */
    void test_is_dap4_projected_dmr_attr_array_false(){
        Byte byte("byte");
        Array array("array", &byte);
        D4Attribute* d4a = new D4Attribute("d4a", attr_byte_c);
        array.attributes()->add_attribute(d4a);
        array.set_send_p(true);

        D4BaseTypeFactory f4;
        DMR *dmr = new DMR(&f4, "test");

        D4Group *d4g = dmr->root();
        d4g->add_var_nocopy(&array);

        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(dmr, inv) == false);
        CPPUNIT_ASSERT(inv.size() == 0);
    }

    /**
    * dmr struct attribute test - true
    * test if a dmr holding a struct with dap4 attribute returns true
    */
    void test_is_dap4_projected_dmr_attr_struct_true(){
        Structure structure("struct");
        Array *array = new Array("array", new Byte("byte"));
        D4Attribute* d4a = new D4Attribute("d4a", attr_int8_c);
        array->attributes()->add_attribute(d4a);
        structure.add_var_nocopy(array);
        structure.set_send_p(true);

        D4BaseTypeFactory f4;
        DMR *dmr = new DMR(&f4, "test");

        D4Group *d4g = dmr->root();
        d4g->add_var_nocopy(&structure);

        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(dmr, inv));
        CPPUNIT_ASSERT(inv.size() == 1);
        CPPUNIT_ASSERT(inv.at(0)->name() == "byte");
    }

    /**
    * dmr struct attribute test - false
    * test if a dmr holding a struct without dap4 attribute returns false
    */
    void test_is_dap4_projected_dmr_attr_struct_false(){
        Structure structure("struct");
        Array *array = new Array("array", new Byte("byte"));
        D4Attribute* d4a = new D4Attribute("d4a", attr_byte_c);
        array->attributes()->add_attribute(d4a);
        structure.add_var_nocopy(array);
        structure.set_send_p(true);

        D4BaseTypeFactory f4;
        DMR *dmr = new DMR(&f4, "test");

        D4Group *d4g = dmr->root();
        d4g->add_var_nocopy(&structure);

        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(dmr, inv) == false);
        CPPUNIT_ASSERT(inv.size() == 0);
    }

    /**
    * dmr subgroup variable test - true
    * test if a dmr holding a subgroup with dap4 vars returns true
    */
    void test_is_dap4_projected_dmr_subgroup_true(){
        Int8 int8("int8");
        int8.set_send_p(true);

        D4BaseTypeFactory f4;
        DMR *dmr = new DMR(&f4, "test");

        D4Group *subd4g = new D4Group("subgroup");
        subd4g->add_var_nocopy(&int8);
        subd4g->set_send_p(true);

        D4Group *d4g = dmr->root();
        d4g->add_group_nocopy(subd4g);
        //d4g->add_var_nocopy(subd4g);

        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(dmr, inv));
        CPPUNIT_ASSERT(inv.size() == 1);
        CPPUNIT_ASSERT(inv.at(0)->name() == "int8");
    }

    /**
    * dmr subgroup variable test - false
    * test if a dmr holding a subgroup with no dap4 vars returns false
    */
    void test_is_dap4_projected_dmr_subgroup_false(){
        Byte byte("byte");
        byte.set_send_p(true);

        D4BaseTypeFactory f4;
        DMR *dmr = new DMR(&f4, "test");

        D4Group *subd4g = new D4Group("subgroup");
        subd4g->add_var_nocopy(&byte);
        subd4g->set_send_p(true);

        D4Group *d4g = dmr->root();
        d4g->add_group_nocopy(subd4g);
        //d4g->add_var_nocopy(subd4g);

        vector<BaseType *> inv;
        CPPUNIT_ASSERT(is_dap4_projected(dmr, inv) == false);
        CPPUNIT_ASSERT(inv.size() == 0);
    }

    ///////////////////////////////////////////////////////
    /// DDS/DAP2 Tests

    CPPUNIT_TEST_SUITE( D4ToolsTest );

    CPPUNIT_TEST(test_is_dap4_projected_int8);
    CPPUNIT_TEST(test_is_dap4_projected_byte);

    CPPUNIT_TEST(test_is_dap4_projected_array_int8);
    CPPUNIT_TEST(test_is_dap4_projected_array_byte);

    CPPUNIT_TEST(test_is_dap4_projected_struct_int8);
    CPPUNIT_TEST(test_is_dap4_projected_struct_byte);

    CPPUNIT_TEST(test_is_dap4_projected_dds_int8);
    CPPUNIT_TEST(test_is_dap4_projected_dds_byte);

    CPPUNIT_TEST(test_is_dap4_projected_attr_true);
    CPPUNIT_TEST(test_is_dap4_projected_attr_false);

    CPPUNIT_TEST(test_is_dap4_projected_attr_array_true);
    CPPUNIT_TEST(test_is_dap4_projected_attr_array_false);

    CPPUNIT_TEST(test_is_dap4_projected_attr_struct_true);
    CPPUNIT_TEST(test_is_dap4_projected_attr_struct_false);

    CPPUNIT_TEST(test_is_dap4_projected_attr_dds_true);
    CPPUNIT_TEST(test_is_dap4_projected_attr_dds_false);

    ///////////////////////////////////////////////////////
    /// DMR/DAP4 Tests

    CPPUNIT_TEST(test_is_dap4_projected_dmr_ddsint8);
    CPPUNIT_TEST(test_is_dap4_projected_dmr_ddsbyte);

    //CPPUNIT_TEST(test_is_dap4_projected_dmr_ddsarray);

    CPPUNIT_TEST(test_is_dap4_projected_dmr_int8);
    CPPUNIT_TEST(test_is_dap4_projected_dmr_int64);
    CPPUNIT_TEST(test_is_dap4_projected_dmr_byte);

    CPPUNIT_TEST(test_is_dap4_projected_dmr_array_int8);
    CPPUNIT_TEST(test_is_dap4_projected_dmr_array_byte);

    CPPUNIT_TEST(test_is_dap4_projected_dmr_struct_int8);
    CPPUNIT_TEST(test_is_dap4_projected_dmr_struct_byte);

    CPPUNIT_TEST(test_is_dap4_projected_dmr_attr_true);
    CPPUNIT_TEST(test_is_dap4_projected_dmr_attr_false);

    CPPUNIT_TEST(test_is_dap4_projected_dmr_attr_array_true);
    CPPUNIT_TEST(test_is_dap4_projected_dmr_attr_array_false);

    CPPUNIT_TEST(test_is_dap4_projected_dmr_attr_struct_true);
    CPPUNIT_TEST(test_is_dap4_projected_dmr_attr_struct_false);

    CPPUNIT_TEST(test_is_dap4_projected_dmr_subgroup_true);
    CPPUNIT_TEST(test_is_dap4_projected_dmr_subgroup_false);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(D4ToolsTest);

int main(int argc, char *argv[])
{
    return bes_run_tests<D4ToolsTest>(argc, argv, "cerr,fonc") ? 0: 1;
}

