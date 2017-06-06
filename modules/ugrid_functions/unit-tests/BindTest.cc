// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2013 OPeNDAP, Inc.
// Author: Nathan David Potter <ndp@opendap.org>
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

//#include <cstdio>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include "GetOpt.h"

#include "debug.h"
#include "util.h"

#include "gridfields/grid.h"
#include "gridfields/gridfield.h"
#include "gridfields/bind.h"
#include "gridfields/array.h"
#include "gridfields/restrict.h"
#include "gridfields/refrestrict.h"
#include "gridfields/arrayreader.h"
#include "gridfields/accumulate.h"

static bool debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

using namespace GF;

namespace ugrid {

#if 0
// Unused
static ArrayReader *new_makeArrayReader(double *array, int size) {
    stringstream *ss = new stringstream();
    stringbuf *pbuf;
    pbuf=ss->rdbuf();
    pbuf->sputn((char *) array, sizeof(double)*size);
    return new ArrayReader(ss);
}
#endif

class BindTest: public CppUnit::TestFixture {

private:

    Grid *makeGrid(int scale, string name)
    {
        CellArray *twocells;
        CellArray *onecells;
        // Unused CellArray *zerocells;
        Grid *grid;
        Node triangle[3];
        Node segment[2];
        // Unused Node node;

        // Unused bool wf;
        int i;
        twocells = new CellArray();
        for (i = 0; i < scale / 2; i++) {
            triangle[0] = i;
            triangle[1] = i + 1;
            triangle[2] = i + 2;
            twocells->addCellNodes(triangle, 3);
        }
        //twocells->print();
        //getchar();
        onecells = new CellArray();
        for (i = 0; i < scale - 1; i++) {
            segment[0] = i;
            segment[1] = i + 1;
            onecells->addCellNodes(segment, 2);
        }
        //onecells->print();

        //getchar();
        grid = new Grid(name, 2);
        grid->setImplicit0Cells(scale);
        grid->setKCells(onecells, 1);
        grid->setKCells(twocells, 2);
        //grid->print(0);
        //getchar();
        return grid;
    }

    Array *makeFloatArray(int size, const char *name)
    {
        Array *arr;
        arr = new Array(name, FLOAT, size);
        float *data;
        arr->getData(data);
        int i;

        for (i = 0; i < size; i++) {
            data[i] = 2 * i - 10;
        }
        return arr;
    }

    /**
     *
     */
    GridField *makeGridField(int size, string /*gridname*/, const char */*datname*/, int k)
    {

        Grid *G;
        GridField *GF;
        Array *data;

        G = makeGrid(size, "A");
        k = 0;
        data = makeFloatArray(size, "x");

        GF = new GridField(G, k, data);
        //printf("Valid? %i\n", !notValid(GF));
        //GF->print();

        return GF;
    }

public:

    // Called once before everything gets tested
    BindTest()
    {
        //    DBG(cerr << " BindTest - Constructor" << endl);

    }

    // Called at the end of the test
    ~BindTest()
    {
        //    DBG(cerr << " BindTest - Destructor" << endl);
    }

    // Called before each test
    void setup()
    {
        //    DBG(cerr << " BindTest - setup()" << endl);
    }

    // Called after each test
    void tearDown()
    {
        //    DBG(cerr << " tearDown()" << endl);
    }

CPPUNIT_TEST_SUITE( BindTest );

    CPPUNIT_TEST(bind_test);

    CPPUNIT_TEST_SUITE_END()
    ;

    void bind_test()
    {
        DBG(cerr << " bind_test - BEGIN" << endl);

        try {
#if 0
            GridField *gfResult;
            GridField *GF;
            GridField *Result;
            GF::Array* gfa;
            string msg;
            int size = 12;
            double dbls[size];
            int ints[size];
            ArrayReader arf;
            ArrayReader *memArrayReader;

            GF = makeGridField(size, "A", "x", 0);
            Array *arr_floats = new Array("io_floats", FLOAT, size);
            GF->Bind(0, arr_floats);

            Array *arr_ints = new Array("io_ints", INT, size);
            GF->Bind(0, arr_ints);

            GridField *aGF = AccumulateOp::Accumulate(GF, 0, "result", "result+1", "0", 0);
            DBG(GF->PrintTo(cerr,9));

            DBG( cerr << "restricting..." << endl);
            Result = RefRestrictOp::Restrict("x<4", 0, GF);
            DBG(Result->PrintTo(cerr,0));

            Result = RefRestrictOp::Restrict("x>-4", 0, Result);
            DBG(Result->PrintTo(cerr,10));
#endif

#if 0
            FileArrayReader *far = new FileArrayReader("bindtest.dat", 0);

            far->setPatternAttribute("result");
            gfResult = BindOp::Bind("io_floats", FLOAT, far, 0, Result);
            DBG(gfResult->PrintTo(cerr,0))

            gfa = gfResult->GetAttribute(0, "result");
            vector<double> file_dbls = gfa->makeArrayf();
            msg.clear();
            for(int i=0; i<file_dbls.size(); i++)
            msg += libdap::double_to_string(file_dbls[i]) + "   ";

            DBG(cerr << endl << "File array reader result: "<< msg << endl << endl << endl);

#endif
#if 0

            for (int i = 0; i < size; i++) {
                ints[i] = i - size/2.0;
                msg += libdap::long_to_string(ints[i]) + "   ";
            }DBG(cerr << endl << "Int array input: "<< msg << endl);

            DBG(cerr << endl <<"Memory Backed ArrayReader - integers"<< endl);
            memArrayReader = arf.makeArrayReader(ints, size);
            memArrayReader->setPatternAttribute("result");
            gfResult = BindOp::Bind("io_ints", INT, memArrayReader, 0, Result);
            DBG(cerr << endl << "Result "; gfResult->PrintTo(cerr,0));

            gfa = gfResult->GetAttribute(0, "result");
            DBG(cerr << endl << "Retrieving Int Result Values... " << endl);
            vector<int> gf_ints = gfa->makeArray();
            msg.clear();
            for (int i = 0; i < gf_ints.size(); i++)
            msg += libdap::long_to_string(gf_ints[i]) + "   ";

            DBG(cerr << endl << "Int array result: "<< msg << endl << endl << endl);
#endif

#if 0
            for(int i=0; i<size;i++) {
                dbls[i] = (i - size/2.0) * 11.01;
                msg += libdap::double_to_string(dbls[i]) + "   ";
            }
            DBG(cerr << endl << "Doubles array input: "<< msg << endl);

            DBG(cerr << endl << "Memory Backed ArrayReader - doubles" << endl);
            memArrayReader = new_makeArrayReader(dbls,size);
            memArrayReader->setPatternAttribute("result");
            gfResult = BindOp::Bind("io_floats", FLOAT, memArrayReader, 0, Result);
            DBG(cerr << endl << "Array of doubles 'Bound' to restricted Grid result: "<< endl);
            DBG(gfResult->PrintTo(cerr,8));

            gfa = gfResult->GetAttribute(0, "result");
            vector<double> gf_dbls = gfa->makeArrayf();
            msg.clear();
            for(int i=0; i<gf_dbls.size(); i++)
            msg += libdap::double_to_string(gf_dbls[i]) + "   ";

            DBG(cerr << endl << "Doubles array result: "<< msg << endl);

#endif

#if 0
            delete GF;
            delete gfResult;
            delete aGF;
            delete arr_floats;
            delete arr_ints;
#endif

            CPPUNIT_ASSERT(true);
        }
        catch (std::string &e) {
            cerr << "Error: " << e << endl;
            CPPUNIT_ASSERT(false);
        }
        catch (...) {
            cerr << "Unknown Error." << endl;
            CPPUNIT_ASSERT(false);
        }
    }

};
// BindTest

CPPUNIT_TEST_SUITE_REGISTRATION(BindTest);

} // namespace ugrid

int main(int argc, char*argv[])
{
    GetOpt getopt(argc, argv, "dh");
    int option_char;
    while ((option_char = getopt()) != -1)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;
        case 'h': {     // help - show test names
            std::cerr << "Usage: BindTest has the following tests:" << std::endl;
            const std::vector<CppUnit::Test*> &tests = ugrid::BindTest::suite()->getTests();
            unsigned int prefix_len = ugrid::BindTest::suite()->getName().append("::").length();
            for (std::vector<CppUnit::Test*>::const_iterator i = tests.begin(), e = tests.end(); i != e; ++i) {
                std::cerr << (*i)->getName().replace(0, prefix_len, "") << std::endl;
            }
            break;
        }
        default:
            break;
        }

    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    bool wasSuccessful = true;
    string test = "";
    int i = getopt.optind;
    if (i == argc) {
        // run them all
        wasSuccessful = runner.run("");
    }
    else {
        while (i < argc) {
            if (debug) cerr << "Running " << argv[i] << endl;
            test = ugrid::BindTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}

#if 0
int main(int, char**) {
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    bool wasSuccessful = runner.run("", false);

    return wasSuccessful ? 0 : 1;
}
#endif
