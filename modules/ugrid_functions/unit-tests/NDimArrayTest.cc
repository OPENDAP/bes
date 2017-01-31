// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2005 OPeNDAP, Inc.
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

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#define DODS_DEBUG

#include <BESDebug.h>

#include "util.h"
#include "debug.h"
#include "Array.h"
#include "Int32.h"
#include "Float64.h"
#include "NDimensionalArray.h"

#include "GetOpt.h"

static bool debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

namespace libdap {

class NDimArrayTest: public CppUnit::TestFixture {
public:
    // Called once before everything gets tested
    NDimArrayTest()
    {

    }

    // Called at the end of the test
    ~NDimArrayTest()
    {
    }

    // Called before each test
    void setup()
    {
        // do stuff;
    }

    // Called after each test
    void tearDown()
    {
        // undo stuff
    }

CPPUNIT_TEST_SUITE( NDimArrayTest );

    CPPUNIT_TEST(getStorageIndex_test);
    CPPUNIT_TEST(getLastDimesnionHyperSlab_test);
    CPPUNIT_TEST(setLastDimesnionHyperSlab_test);

    CPPUNIT_TEST_SUITE_END()
    ;

    void setLastDimesnionHyperSlab_test()
    {
        DBG(cerr << " setLastDimesnionHyperSlab_test() - BEGIN." << endl);

        Float64 tmplt("foo");
        libdap::Array test("foo", &tmplt);
        test.append_dim(5, "dim1");
        test.append_dim(1081, "dim2");
        test.append_dim(1000, "dim3");

        unsigned int start[test.dimensions(true)], stride[test.dimensions(true)], stop[test.dimensions(true)];
        vector<unsigned int> shape(test.dimensions(true));

        start[0] = 0;
        stride[0] = 1;
        stop[0] = 4;

        start[1] = 97;
        stride[1] = 1;
        stop[1] = 98;

        start[2] = 0;
        stride[2] = 1;
        stop[2] = 999;
        libdap::Array::Dim_iter dIt;
        int i = 0;
        for (dIt = test.dim_begin(); dIt != test.dim_end(); dIt++, i++) {
            test.add_constraint(dIt, start[i], stride[i], stop[i]);
        }
        long constrainedSize = libdap::NDimensionalArray::computeConstrainedShape(&test, &shape);

        DBG(
            cerr << " setLastDimesnionHyperSlab_test() - constrainedSize=" << libdap::long_to_string(constrainedSize)
                << endl);
        CPPUNIT_ASSERT(constrainedSize == 10000);

        NDimensionalArray nda(&test);
        nda.setAll(0);

        unsigned long lastDimSize = nda.getLastDimensionElementCount();
        DBG(cerr << " setLastDimesnionHyperSlab_test() - lastDimSize=" << libdap::long_to_string(lastDimSize) << endl);
        CPPUNIT_ASSERT(lastDimSize == 1000);

        vector<unsigned int> location(test.dimensions(true) - 1);
        location[0] = 2;
        location[1] = 1;

        double pi = std::atan(1) * 4;
        dods_float64 stuff[lastDimSize];
        for (unsigned long i = 0; i < lastDimSize; i++)
            stuff[i] = pi;

        DBG(
            cerr << " setLastDimesnionHyperSlab_test() - setting slab values to " << libdap::double_to_string(pi)
                << endl);
        nda.setLastDimensionHyperSlab(&location, stuff, nda.getLastDimensionElementCount());

        dods_float64 *slab;
        unsigned int slabElementCount = 0;
        nda.getLastDimensionHyperSlab(&location, (void**) &slab, &slabElementCount);
        DBG(
            cerr << " setLastDimesnionHyperSlab_test() - Retrieved  slab. Slab element count " << slabElementCount
                << endl);

        CPPUNIT_ASSERT(slabElementCount == lastDimSize);

        DBG(cerr << " setLastDimesnionHyperSlab_test() - Checking slab values... " << endl);
        for (unsigned int i = 0; i < slabElementCount; i++)
            CPPUNIT_ASSERT(slab[i] == pi);

        DBG(cerr << " setLastDimesnionHyperSlab_test() - END." << endl);
    }

    void getLastDimesnionHyperSlab_test()
    {
        DBG(cerr << " getLastDimesnionHyperSlab_test() - BEGIN." << endl);

        Float64 tmplt("foo");
        libdap::Array test("foo", &tmplt);
        test.append_dim(5, "dim1");
        test.append_dim(1081, "dim2");
        test.append_dim(1000, "dim3");

        unsigned int start[test.dimensions(true)], stride[test.dimensions(true)], stop[test.dimensions(true)];
        vector<unsigned int> shape(test.dimensions(true));

        start[0] = 0;
        stride[0] = 1;
        stop[0] = 4;

        start[1] = 107;
        stride[1] = 1;
        stop[1] = 116;

        start[2] = 0;
        stride[2] = 1;
        stop[2] = 999;

        libdap::Array::Dim_iter dIt;
        int i = 0;
        for (dIt = test.dim_begin(); dIt != test.dim_end(); dIt++, i++) {
            test.add_constraint(dIt, start[i], stride[i], stop[i]);
        }

        unsigned long constrainedSize = libdap::NDimensionalArray::computeConstrainedShape(&test, &shape);
        DBG(
            cerr << " getLastDimesnionHyperSlab_test() - constrainedSize=" << libdap::long_to_string(constrainedSize)
                << endl);
        CPPUNIT_ASSERT(constrainedSize == 50000);

        NDimensionalArray nda(&test);

        vector<unsigned int> location(test.dimensions(true) - 1);
        void *slab, *firstSlab;
        unsigned int slabElementCount;
        unsigned long offset;

        void *internalStorage = nda.getStorage();
        DBG(cerr << " getLastDimesnionHyperSlab_test() - internalStorage=" << internalStorage << endl);

        location[0] = 0;
        location[1] = 0;
        nda.getLastDimensionHyperSlab(&location, &firstSlab, &slabElementCount);
        DBG(
            cerr << " getLastDimesnionHyperSlab_test() - slab elementCount=" << libdap::long_to_string(slabElementCount)
                << endl);
        DBG(cerr << " getLastDimesnionHyperSlab_test() - first Slab address=" << firstSlab << endl);

        location[0] = 0;
        location[1] = 1;
        nda.getLastDimensionHyperSlab(&location, &slab, &slabElementCount);
        DBG(cerr << " getLastDimesnionHyperSlab_test() - 2nd slab address=" << slab << endl);

        offset = ((char *) slab - (char *) firstSlab) / nda.sizeOfElement();
        DBG(cerr << " getLastDimesnionHyperSlab_test() - Offset " << offset << " elements" << endl);
        CPPUNIT_ASSERT(offset == slabElementCount);

        location[0] = 4;
        location[1] = 9;
        nda.getLastDimensionHyperSlab(&location, &slab, &slabElementCount);
        DBG(cerr << " getLastDimesnionHyperSlab_test() - last slab address=" << slab << endl);

        offset = ((char *) slab - (char *) firstSlab) / nda.sizeOfElement();
        DBG(cerr << " getLastDimesnionHyperSlab_test() - Offset " << offset << " elements" << endl);
        CPPUNIT_ASSERT(offset == (constrainedSize - slabElementCount));

        DBG(cerr << " getLastDimesnionHyperSlab_test() - END." << endl);

    }

    void getStorageIndex_test()
    {
        DBG(cerr << " getStorageIndex_test() - BEGIN." << endl);

        Int32 tmplt("foo");
        libdap::Array test("foo", &tmplt);

        test.append_dim(5, "dim1");
        test.append_dim(5, "dim2");
        test.append_dim(5, "dim3");

        vector<unsigned int> shape(test.dimensions(true));

        DBG(cerr << " getStorageIndex_test() - shape.size()=" << shape.size() << endl);

        long constrainedSize = libdap::NDimensionalArray::computeConstrainedShape(&test, &shape);
        DBG(cerr << " getStorageIndex_test() - constrainedSize=" << constrainedSize << endl);
        CPPUNIT_ASSERT(constrainedSize == test.length());

        for (unsigned int i = 0; i < shape.size(); i++) {
            DBG(cerr << " getStorageIndex_test() - shape[" << i << "]=" << shape[i] << endl);
        }

        vector<unsigned int> location(test.dimensions(true));
        DBG(cerr << " getStorageIndex_test() - location.size()=" << location.size() << endl);
        location[0] = 2;
        location[1] = 2;
        location[2] = 2;
        checkLocationIndex(&shape, &location, 62);

        location[1] = 4;
        checkLocationIndex(&shape, &location, 72);

        location[0] = 3;
        checkLocationIndex(&shape, &location, 97);

        location[0] = 0;
        location[1] = 0;
        location[2] = 0;
        checkLocationIndex(&shape, &location, 0);

        location[0] = 4;
        location[1] = 4;
        location[2] = 4;
        checkLocationIndex(&shape, &location, 124);

        location[2] = 17;

        try {
            checkLocationIndex(&shape, &location, 0);
            DBG(cerr << " getStorageIndex_test() - Failed to Detect Bounds Violation." << endl);
            CPPUNIT_ASSERT(false);
        }
        catch (libdap::Error &e) {
            DBG(
                cerr << " getStorageIndex_test() - Correctly Detected Bounds Violation. Error Message: "
                    << e.get_error_message() << endl);
            CPPUNIT_ASSERT(true);

        }

        DBG(cerr << " getStorageIndex_test() - END." << endl);
    }

    void checkLocationIndex(vector<unsigned int> *shape, vector<unsigned int> *location, long index)
    {
        for (unsigned int i = 0; i < location->size(); i++) {
            DBG(
                cerr << " checkLocation() - location[" << libdap::long_to_string(i) << "]="
                    << libdap::long_to_string((*location)[i]) << endl);
        }

        long storageIndex = libdap::NDimensionalArray::getStorageIndex(shape, location);
        DBG(cerr << " checkLocation() - storageIndex=" << libdap::long_to_string(storageIndex) << endl);
        CPPUNIT_ASSERT(storageIndex == index);
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(NDimArrayTest);

} /* namespace libdap */

int main(int argc, char*argv[])
{
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    GetOpt getopt(argc, argv, "d");
    int option_char;
    while ((option_char = getopt()) != -1)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            BESDebug::SetUp("cerr,ugrid");
            break;
        default:
            break;
        }

    bool wasSuccessful = true;
    string test = "";
    int i = getopt.optind;
    if (i == argc) {
        // run them all
        wasSuccessful = runner.run("");
    }
    else {
        while (i < argc) {
            test = string("libdap::NDimArrayTest::") + argv[i++];

            DBG(cerr << endl << "Running test " << test << endl << endl);

            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
