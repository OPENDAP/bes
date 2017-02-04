// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of asciival, software which can return an ASCII
// representation of the data read from a DAP server.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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

// Tests for the DataDDS class.

#include <iostream>
#include <vector>
#include <algorithm>
#include <iterator>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

//#define DODS_DEBUG2
//#define DODS_DEBUG2

#include <DDS.h>
#include <debug.h>

#include "XDInt32.h"
#include "XDStructure.h"
#include "XDArray.h"
#include "XDOutputFactory.h"

#include "test_config.h"

bool translate = false;

using namespace CppUnit;
using namespace std;
using namespace libdap;

static int str_to_file_cmp(const string &s, const string &f)
{
    ifstream ifs;
    ifs.open(f.c_str());
    if (!ifs)
        throw InternalErr(__FILE__, __LINE__, "Could not open file");
    string line, doc;
    while (!ifs.eof()) {
        getline(ifs, line);
        doc.append(line);
        doc.append("\n");
    }

    return doc.compare(s);
}

class XDArrayTest: public TestFixture {
private:
    DDS *dds1;
    XDArray *a, *b, *c, *d, *e;
    XDOutputFactory *aof;

public:
    XDArrayTest()
    {
    }
    ~XDArrayTest()
    {
    }

    void setUp()
    {
        aof = new XDOutputFactory;
        dds1 = new DDS(aof, "xml_data_array_test");
        try {
            string parsefile = (string) TEST_SRC_DIR + "/testsuite/XDArrayTest1.dds";
            dds1->parse(parsefile);
            DDS::Vars_iter p = dds1->var_begin();

            a = dynamic_cast<XDArray*>(*p++);
            vector<dods_int32> i32a;
            for (dods_int32 i = 0; i < 10; i++) {
                i32a.push_back(i * (-512));
            }
            a->set_value(i32a, i32a.size());

            b = dynamic_cast<XDArray*>(*p++);
            //b->set_redirect(b);
            vector<dods_int32> i32b;
            for (dods_int32 i = 0; i < 10; i++)
                for (dods_int32 j = 0; j < 10; j++)
                    i32b.push_back(i * j * (2));
            b->set_value(i32b, i32b.size());

            c = dynamic_cast<XDArray*>(*p++);
            //c->set_redirect(c);
            vector<dods_int32> i32c;
            for (dods_int32 i = 0; i < 5; i++)
                for (dods_int32 j = 0; j < 5; j++)
                    for (dods_int32 k = 0; k < 5; k++)
                        i32c.push_back(i * j * k * (2));

            c->set_value(i32c, i32c.size());

            d = dynamic_cast<XDArray*>(*p++);
            //d->set_redirect(d);
            vector<dods_int32> i32d;
            for (dods_int32 i = 0; i < 3; i++)
                for (dods_int32 j = 0; j < 4; j++)
                    for (dods_int32 k = 0; k < 5; k++)
                        for (dods_int32 l = 0; l < 6; l++)
                            i32d.push_back(i * j * k * l * (2));
            d->set_value(i32d, i32d.size());

            // Get the Structure array
            e = dynamic_cast<XDArray*>(*p++);
            //e->set_redirect(e);

            // Build a structure that contains arrays
            Structure *elem = new Structure("elem");
            Int32 *e_a = new Int32("e_a");
            e_a->set_value(17);
            e_a->set_send_p(true);
            elem->add_var(e_a);

            Int32 *e_z = new Int32("e_z");
            e_z->set_value(42);
            e_z->set_send_p(true);
            elem->add_var(e_z);

            Array *e_b = new Array("e_b", new Int32("e_b"));
            e_b->append_dim(3, "e_b_values");
            vector<dods_int32> e_b_int32;
            for (dods_int32 i = 0; i < 3; i++)
                e_b_int32.push_back(i * (-512));
            e_b->set_value(e_b_int32, e_b_int32.size());
            elem->add_var(e_b);

            Array *e_c = new Array("e_c", new Int32("e_c"));
            e_c->append_dim(2, "e_c_1_values");
            e_c->append_dim(3, "e_c_2_values");
            vector<dods_int32> e_c_int32;
            for (dods_int32 i = 0; i < 2; i++)
                for (dods_int32 j = 0; j < 3; j++)
                    e_c_int32.push_back(i * j * (2));
            e_c->set_value(e_c_int32, e_c_int32.size());
            elem->add_var(e_c);
            elem->set_send_p(true);

            // Load the same pointer into the array 4 times; set_vec copies
            e->set_vec(0, elem);
            e->set_vec(1, elem);
            e->set_vec(2, elem);
            e->set_vec(3, elem);

            delete elem;
        }
        catch (Error &e) {
            cerr << "Caught Error in setUp: " << e.get_error_message() << endl;
        }
        catch (exception &e) {
            cerr << "Caught std::exception in setUp: " << e.what() << endl;
        }
    }

    void tearDown()
    {
        delete aof;
        aof = 0;
        delete dds1;
        dds1 = 0;
    }

CPPUNIT_TEST_SUITE( XDArrayTest );
#if 0
        CPPUNIT_TEST(test_get_nth_dim_size);
        CPPUNIT_TEST(test_get_shape_vector);
        CPPUNIT_TEST(test_get_index);
#endif

    CPPUNIT_TEST(test_print_xml_data_a);
#if 1
    CPPUNIT_TEST(test_print_xml_data_b);
    CPPUNIT_TEST(test_print_xml_data_c);
    CPPUNIT_TEST(test_print_xml_data_d);
    CPPUNIT_TEST(test_print_xml_data_e);
#endif

    CPPUNIT_TEST_SUITE_END();

    void test_get_nth_dim_size()
    {
        CPPUNIT_ASSERT(a->get_nth_dim_size(0) == 10);

        CPPUNIT_ASSERT(b->get_nth_dim_size(0) == 10);
        CPPUNIT_ASSERT(b->get_nth_dim_size(1) == 10);

        CPPUNIT_ASSERT(c->get_nth_dim_size(0) == 5);
        CPPUNIT_ASSERT(c->get_nth_dim_size(1) == 5);
        CPPUNIT_ASSERT(c->get_nth_dim_size(2) == 5);

        try {
            a->get_nth_dim_size((unsigned long) -1);
            CPPUNIT_ASSERT(false);
        }
        catch (InternalErr &ie) {
            CPPUNIT_ASSERT(true);
        }
        try {
            a->get_nth_dim_size(1);
            CPPUNIT_ASSERT(false);
        }
        catch (InternalErr &ie) {
            CPPUNIT_ASSERT(true);
        }

        try {
            c->get_nth_dim_size(3);
            CPPUNIT_ASSERT(false);
        }
        catch (InternalErr &ie) {
            CPPUNIT_ASSERT(true);
        }
    }

    void test_get_shape_vector()
    {
        try {
            vector<int> a_shape(1, 10);

            CPPUNIT_ASSERT(a->get_shape_vector(1) == a_shape);

            vector<int> b_shape(2, 10);
            CPPUNIT_ASSERT(b->get_shape_vector(2) == b_shape);

            vector<int> c_shape(3, 5);
            CPPUNIT_ASSERT(c->get_shape_vector(3) == c_shape);

            vector<int> d_shape(3);
            d_shape[0] = 3;
            d_shape[1] = 4;
            d_shape[2] = 5;

            CPPUNIT_ASSERT(d->get_shape_vector(3) == d_shape);

            try {
                a->get_shape_vector(0);
                CPPUNIT_ASSERT(false);
            }
            catch (InternalErr &ie) {
                CPPUNIT_ASSERT(true);
            }
            try {
                a->get_shape_vector(2);
                CPPUNIT_ASSERT(false);
            }
            catch (InternalErr &ie) {
                CPPUNIT_ASSERT(true);
            }

            try {
                d->get_shape_vector(5);
                CPPUNIT_ASSERT(false);
            }
            catch (InternalErr &ie) {
                CPPUNIT_ASSERT(true);
            }
        }
        catch (Error &e) {
            cerr << "Error: " << e.get_error_message() << endl;
            CPPUNIT_ASSERT(false);
        }
    }

    void test_get_index()
    {
        try {
            vector<int> a_state(1);
            a_state[0] = 0;
            CPPUNIT_ASSERT(a->m_get_index(a_state) == 0);
            a_state[0] = 9;
            CPPUNIT_ASSERT(a->m_get_index(a_state) == 9);

            vector<int> b_state(2, 0);
            CPPUNIT_ASSERT(b->m_get_index(b_state) == 0);
            b_state[0] = 0;
            b_state[1] = 5;
            CPPUNIT_ASSERT(b->m_get_index(b_state) == 5);
            b_state[0] = 5;
            b_state[1] = 5;
            CPPUNIT_ASSERT(b->m_get_index(b_state) == 55);
            b_state[0] = 9;
            b_state[1] = 9;
            CPPUNIT_ASSERT(b->m_get_index(b_state) == 99);

            vector<int> d_state(4, 0);
            CPPUNIT_ASSERT(d->m_get_index(d_state) == 0);
            d_state[0] = 2;
            d_state[1] = 3;
            d_state[2] = 4;
            d_state[3] = 5;
            CPPUNIT_ASSERT(d->m_get_index(d_state) == 359);

            d_state[0] = 1;
            d_state[1] = 2;
            d_state[2] = 0;
            d_state[3] = 2;
            CPPUNIT_ASSERT(d->m_get_index(d_state) == 1*(4*5*6) + 2*(5*6) + 0*(6) + 2);
        }
        catch (Error &e) {
            cerr << "Error: " << e.get_error_message() << endl;
            CPPUNIT_ASSERT(false);
        }
    }

    void test_print_xml_data_a()
    {
        try {
            a->set_send_p(true);
            XMLWriter writer;
            dynamic_cast<XDOutput*>(a)->print_xml_data(&writer, true);

            DBG2(cerr << writer.get_doc() << endl);
#if 0
            CPPUNIT_ASSERT(
                    str_to_file_cmp(writer.get_doc(), (string)TEST_SRC_DIR + "/testsuite/xdarraytest_a.xml") == 0);
#endif
        }
        catch (InternalErr &e) {
            cerr << "Caught an InternalErr: " << e.get_error_message() << endl;
            CPPUNIT_FAIL("Caught an InternalErr");
        }
    }

    void test_print_xml_data_b()
    {
        try {
            b->set_send_p(true);
            XMLWriter writer;
            dynamic_cast<XDOutput*>(b)->print_xml_data(&writer, true);
            DBG2(cerr << writer.get_doc() << endl);

            CPPUNIT_ASSERT(
                    str_to_file_cmp(writer.get_doc(), (string)TEST_SRC_DIR + "/testsuite/xdarraytest_b.xml") == 0);

        }
        catch (InternalErr &e) {
            cerr << "Caught an InternalErr: " << e.get_error_message() << endl;
            CPPUNIT_FAIL("Caught an InternalErr");
        }
    }

    void test_print_xml_data_c()
    {
        try {
            c->set_send_p(true);
            XMLWriter writer;
            dynamic_cast<XDOutput*>(c)->print_xml_data(&writer, true);
            DBG2(cerr << writer.get_doc() << endl);

            CPPUNIT_ASSERT(
                    str_to_file_cmp(writer.get_doc(), (string)TEST_SRC_DIR + "/testsuite/xdarraytest_c.xml") == 0);

        }
        catch (InternalErr &e) {
            cerr << "Caught an InternalErr: " << e.get_error_message() << endl;
            CPPUNIT_FAIL("Caught an InternalErr");
        }
    }

    void test_print_xml_data_d()
    {
        try {
            d->set_send_p(true);
            XMLWriter writer;
            dynamic_cast<XDOutput*>(d)->print_xml_data(&writer, true);
            DBG2(cerr << writer.get_doc() << endl);

            CPPUNIT_ASSERT(
                    str_to_file_cmp(writer.get_doc(), (string)TEST_SRC_DIR + "/testsuite/xdarraytest_d.xml") == 0);

        }
        catch (InternalErr &e) {
            cerr << "Caught an InternalErr: " << e.get_error_message() << endl;
            CPPUNIT_FAIL("Caught an InternalErr");
        }
    }

    void test_print_xml_data_e()
    {
        try {
            e->set_send_p(true);
            XMLWriter writer;
            dynamic_cast<XDOutput*>(e)->print_xml_data(&writer, true);
            DBG(cerr << writer.get_doc() << endl);

            CPPUNIT_ASSERT(
                    str_to_file_cmp(writer.get_doc(), (string)TEST_SRC_DIR + "/testsuite/xdarraytest_e.xml") == 0);

        }
        catch (InternalErr &e) {
            xmlErrorPtr error = xmlGetLastError();
            if (error)
                cerr << "Caught an InternalErr: " << e.get_error_message() << "libxml: " << error->message << endl;
            else
                cerr << "Caught an InternalErr: " << e.get_error_message() << "libxml: no message" << endl;
            CPPUNIT_FAIL("Caught an InternalErr");
        }
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION(XDArrayTest);

int main(int /*argc*/, char* /*argv*/[])
{
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    bool wasSuccessful = runner.run("", false);

    return wasSuccessful ? 0 : 1;
}

