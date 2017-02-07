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

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

// #define DODS_DEBUG

#include <DDS.h>
#include <debug.h>

#include "XDOutput.h"
#include "XDOutputFactory.h"
#include "XDInt32.h"
#include "XDStr.h"
#include "XDStructure.h"
#include "XDGrid.h"
#include "XDArray.h"

#include "test_config.h"

// These globals are defined in ascii_val.cc and are needed by the XD*
// classes. This code has to be linked with those so that the XD*
// specializations of Byte, ..., Grid will be instantiated by DDS when it
// parses a .dds file. Each of those subclasses is a child of XDOutput in
// addition to its regular lineage. This test code depends on being able to
// cast each variable to an XDOutput object. 01/24/03 jhrg
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

class XDOutputTest: public TestFixture {
private:
    DDS *dds;
    XDOutputFactory *aof;

public:
    XDOutputTest()
    {
    }
    ~XDOutputTest()
    {
    }

    void setUp()
    {
        aof = new XDOutputFactory;
        dds = new DDS(aof, "xml_data_output_test");
        string parsefile = (string) TEST_SRC_DIR + "/testsuite/XDOutputTest1.dds";
        dds->parse(parsefile);

        XDInt32 *a = static_cast<XDInt32*>(dds->var("a"));
        a->set_value(0);

        XDStructure *e = static_cast<XDStructure*>(dds->var("e"));
        XDInt32 *b = static_cast<XDInt32*>(e->var("b"));
        b->set_value(10);
        XDStr *d = static_cast<XDStr*>(e->var("d"));
        d->set_value("");

        // Load various arrays with data
        // g.xy[12][12], g.x[12], g.y[12]
        XDGrid *g = dynamic_cast<XDGrid*>(dds->var("g"));
        XDArray *xy = dynamic_cast<XDArray*>(g->array_var());
        // See XDArrayTest for a note about this trick.
        //xy->set_redirect(xy);
        Grid::Map_iter m = g->map_begin();
        XDArray *x = dynamic_cast<XDArray*>(*m++);
        //x->set_redirect(x);
        XDArray *y = dynamic_cast<XDArray*>(*m);
        //y->set_redirect(y);

        vector<dods_float64> f64x;
        for (int i = 0; i < 12; i++)
            f64x.push_back(i * (-51.2));
        x->set_value(f64x, f64x.size());
        y->set_value(f64x, f64x.size());

        vector<dods_byte> bxy;
        for (int i = 0; i < 12; i++)
            for (int j = 0; j < 12; j++)
                bxy.push_back(i * j * (2));
        xy->set_value(bxy, bxy.size());
    }

    void tearDown()
    {
        delete aof;
        aof = 0;
        delete dds;
        dds = 0;
    }

    CPPUNIT_TEST_SUITE(XDOutputTest);

        CPPUNIT_TEST(test_print_xml_data);
        CPPUNIT_TEST(test_print_xml_data_structure);
        CPPUNIT_TEST(test_print_xml_data_grid);

    CPPUNIT_TEST_SUITE_END();

    void test_print_xml_data()
    {
        try {
            dds->var("a")->set_send_p(true);
            XMLWriter writer;
            // See note in XDArrayTest about this trick.
            //dynamic_cast<XDOutput*> (dds->var("a"))->set_redirect(dds->var("a"));
            dynamic_cast<XDOutput*>(dds->var("a"))->print_xml_data(&writer, true);

            DBG(cerr << writer.get_doc() << endl);

            CPPUNIT_ASSERT(
                    str_to_file_cmp(writer.get_doc(), (string)TEST_SRC_DIR + "/testsuite/xdoutputtest_a.xml") == 0);
        }
        catch (InternalErr &e) {
            cerr << "Caught an InternalErr: " << e.get_error_message() << endl;
            CPPUNIT_FAIL("Caught an InternalErr");
        }
    }

    void test_print_xml_data_structure()
    {
        try {
            dds->var("e")->set_send_p(true);
            XMLWriter writer;
            //dynamic_cast<XDOutput*> (dds->var("e"))->set_redirect(dds->var("e"));
            dynamic_cast<XDOutput*>(dds->var("e"))->print_xml_data(&writer, true);

            DBG(cerr << writer.get_doc() << endl);

            CPPUNIT_ASSERT(
                    str_to_file_cmp(writer.get_doc(), (string)TEST_SRC_DIR + "/testsuite/xdoutputtest_e.xml") == 0);
        }
        catch (InternalErr &e) {
            cerr << "Caught an InternalErr: " << e.get_error_message() << endl;
            CPPUNIT_FAIL("Caught an InternalErr");
        }
    }

    void test_print_xml_data_grid()
    {
        try {
            dds->var("g")->set_send_p(true);
            XMLWriter writer;
            //dynamic_cast<XDOutput*> (dds->var("g"))->set_redirect(dds->var("g"));
            dynamic_cast<XDOutput*>(dds->var("g"))->print_xml_data(&writer, true);

            DBG(cerr << writer.get_doc() << endl);

            CPPUNIT_ASSERT(
                    str_to_file_cmp(writer.get_doc(), (string)TEST_SRC_DIR + "/testsuite/xdoutputtest_g.xml") == 0);
        }
        catch (InternalErr &e) {
            cerr << "Caught an InternalErr: " << e.get_error_message() << endl;
            CPPUNIT_FAIL("Caught an InternalErr");
        }
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION(XDOutputTest);

int main(int /*argc*/, char* /*argv*/[])
{
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    bool wasSuccessful = runner.run("", false);

    return wasSuccessful ? 0 : 1;
}

