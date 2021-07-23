// -*- mode: c++; c-basic-offset:4 -*-
//
// FoCovJsonTest.cc
//
// This file is part of BES CovJSON File Out Module
//
// Copyright (c) 2018 OPeNDAP, Inc.
// Author: Corey Hemphill <hemphilc@oregonstate.edu>
// Author: River Hendriksen <hendriri@oregonstate.edu>
// Author: Riley Rimer <rrimer@oregonstate.edu>
//
// Adapted from the File Out JSON module implemented by Nathan Potter
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

//#include <cstdio>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>
#include <math.h>       /* atan */

#include <unistd.h>
#include <DataDDS.h>
#include <Byte.h>
#include <Int16.h>
#include <UInt16.h>
#include <Int32.h>
#include <UInt32.h>
#include <Float32.h>
#include <Float64.h>
#include <Str.h>

#include <Structure.h>
#include <Sequence.h>
#include <Grid.h>

#include <debug.h>
#include <util.h>

#include <BESInternalError.h>
#include <BESDebug.h>

#include "test_config.h"
#include "focovjson_utils.h"

#include "FoDapCovJsonTransform.h"

static bool debug = true;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

namespace focovjson {

class FoCovJsonTest: public CppUnit::TestFixture {

private:
    string d_tmpDir;

    string fileToString(const string &fn)
    {
        ifstream is;
        is.open(fn.c_str(), ios::binary);
        if (!is || is.eof()) return "";

        // get length of file:
        is.seekg(0, ios::end);
        int length = is.tellg();

        // back to start
        is.seekg(0, ios::beg);

        // allocate memory:
        vector<char> buffer(length + 1);

        // read data as a block:
        is.read(&buffer[0], length);
        is.close();
        buffer[length] = '\0';

        return string(&buffer[0]);
    }

public:
    // Called once before everything gets tested
    FoCovJsonTest() :
        d_tmpDir(string(TEST_BUILD_DIR) + "/tmp")
    {
        DBG(cerr << "FoCovJsonTest - Constructor" << endl);
    }

    // Called at the end of the test
    ~FoCovJsonTest()
    {
        DBG(cerr << "FoCovJsonTest - Destructor" << endl);
    }

    // Called before each test
    void setUp()
    {
    }

    // Called after each test
    void tearDown()
    {
    }

    CPPUNIT_TEST_SUITE(FoCovJsonTest);

    /* Add unit test functions to the FoCovJsonTest test suite here!! */
    CPPUNIT_TEST(testAbstractObjectMetadataRepresentation);
    CPPUNIT_TEST(testAbstractObjectDataRepresentation);
    CPPUNIT_TEST(testPrintAxes);
    CPPUNIT_TEST(testPrintReference);
    CPPUNIT_TEST(testPrintDomain);
    CPPUNIT_TEST(testPrintParameters);
    CPPUNIT_TEST(testPrintRanges);
    CPPUNIT_TEST(testPrintCoverage);

    CPPUNIT_TEST_SUITE_END();

    /**
     * @brief For testing the abstract object metadata representation
     *     via the FoDapCovJsonTransform::transform functions
     */
    void testAbstractObjectMetadataRepresentation()
    {
        DBG(cerr << endl);
        try {
            libdap::DataDDS *test_DDS = makeTestDDS();

            //############################# DATA TEST ####################################
            DBG(cerr << endl << "FoCovJsonTest::testAbstractObjectMetadataRepresentation() - BEGIN" << endl);
            DBG(cerr << "FoCovJsonTest::testAbstractObjectMetadataRepresentation() - d_tmpDir: " << d_tmpDir << endl);
            string tmpFile(d_tmpDir + "/test_abstract_object_representation_METADATA.covjson");
            DBG(cerr << "FoCovJsonTest::testAbstractObjectMetadataRepresentation() - tmpFile: " << tmpFile << endl);

            FoDapCovJsonTransform ft(test_DDS);

            DBG(cerr << "FoCovJsonTest::testAbstractObjectMetadataRepresentation() - Calling FoDapCovJsonTransform::transform(false) - Send metadata" << endl);

            fstream output;
            output.open(tmpFile.c_str(), std::fstream::out);
            ft.setTestAxesExistence(true, true, false, true);
            ft.transform(output, false, true); // Send metadata only, test override is true

            // Compare the result with the baseline file.
            string baseline = fileToString((string)TEST_SRC_DIR + "/baselines/abstract_object_test_METADATA.covjson.baseline");
            string result = fileToString(tmpFile);

            DBG(cerr << "FoCovJsonTest::testAbstractObjectMetadataRepresentation() - baseline: " << endl << endl << baseline << endl);
            DBG(cerr << "FoCovJsonTest::testAbstractObjectMetadataRepresentation() - result: " << endl << endl << result << endl);
            DBG(cerr << "FoCovJsonTest::testAbstractObjectMetadataRepresentation() - baseline.compare(result): " << baseline.compare(result) << endl);

            DBG(cerr << "FoCovJsonTest::testAbstractObjectMetadataRepresentation() - baseline length: " << baseline.length() << endl);
            DBG(cerr << "FoCovJsonTest::testAbstractObjectMetadataRepresentation() - result length: " << result.length() << endl);

            CPPUNIT_ASSERT(baseline.length() == result.length());
            CPPUNIT_ASSERT(baseline.compare(result) == 0);

            DBG(cerr << "FoCovJsonTest::testAbstractObjectMetadataRepresentation() - FoDapCovJsonTransform::transform(false) SUCCESS. Deleting DDS..." << endl);

            delete test_DDS;

            DBG(cerr << "FoCovJsonTest::testAbstractObjectMetadataRepresentation() - END" << endl);
        }
        catch (BESInternalError &e) {
            cerr << "BESInternalError: " << e.get_message() << endl;
            CPPUNIT_ASSERT(false);
        }
        catch (libdap::Error &e) {
            cerr << "Error: " << e.get_error_message() << endl;
            CPPUNIT_ASSERT(false);
        }
        catch (std::exception &e) {
            DBG(cerr << "std::exception: " << e.what() << endl);
            CPPUNIT_FAIL("Caught std::exception");
        }
        catch (...) {
            CPPUNIT_FAIL("Unknown Error!");
        }
    }

    /**
     * @brief For testing the abstract object data representation
     *     via the FoDapCovJsonTransform::transform functions
     */
    void testAbstractObjectDataRepresentation()
    {
        DBG(cerr << endl);
        try {
            libdap::DataDDS *test_DDS = makeTestDDS();

            //############################# DATA TEST ####################################
            DBG(cerr << endl << "FoCovJsonTest::testAbstractObjectDataRepresentation() - BEGIN" << endl);
            DBG(cerr << "FoCovJsonTest::testAbstractObjectDataRepresentation() - d_tmpDir: " << d_tmpDir << endl);
            string tmpFile(d_tmpDir + "/test_abstract_object_representation_DATA.covjson");
            DBG(cerr << "FoCovJsonTest::testAbstractObjectDataRepresentation() - tmpFile: " << tmpFile << endl);

            FoDapCovJsonTransform ft(test_DDS);

            DBG(cerr << "FoCovJsonTest::testAbstractObjectDataRepresentation() - Calling FoDapCovJsonTransform::transform(true) - Send data." << endl);

            fstream output;
            output.open(tmpFile.c_str(), std::fstream::out);
            //ft.setAxesExistence(true, true, false, true);
            ft.transform(output, true, true); // Send metadata and data, test override is true

            // Compare the result with the baseline file.
            string baseline = fileToString((string)TEST_SRC_DIR + "/baselines/abstract_object_test_DATA.covjson.baseline");
            string result = fileToString(tmpFile);

            DBG(cerr << "FoCovJsonTest::testAbstractObjectDataRepresentation() - baseline:" << endl << endl << baseline << endl);
            DBG(cerr << "FoCovJsonTest::testAbstractObjectDataRepresentation() - result:" << endl << endl << result << endl);
            DBG(cerr << "FoCovJsonTest::testAbstractObjectDataRepresentation() - baseline.compare(result): " << baseline.compare(result) << endl);

            DBG(cerr << "FoCovJsonTest::testAbstractObjectDataRepresentation() - baseline length: " << baseline.length() << endl);
            DBG(cerr << "FoCovJsonTest::testAbstractObjectDataRepresentation() - result length: " << result.length() << endl);

            CPPUNIT_ASSERT(baseline.length() == result.length());
            CPPUNIT_ASSERT(baseline.compare(result) == 0);

            DBG(cerr << "FoCovJsonTest::testAbstractObjectDataRepresentation() - FoDapCovJsonTransform::transform(true) SUCCESS. Deleting DDS..." << endl);

            delete test_DDS;

            DBG(cerr << "FoCovJsonTest::testAbstractObjectDataRepresentation() - END" << endl);
        }
        catch (BESInternalError &e) {
            cerr << "BESInternalError: " << e.get_message() << endl;
            CPPUNIT_ASSERT(false);
        }
        catch (libdap::Error &e) {
            cerr << "Error: " << e.get_error_message() << endl;
            CPPUNIT_ASSERT(false);
        }
        catch (std::exception &e) {
            DBG(cerr << "std::exception: " << e.what() << endl);
            CPPUNIT_FAIL("Caught std::exception");
        }
        catch (...) {
            CPPUNIT_FAIL("Unknown Error!");
        }
    }

    /**
     * @brief For testing the FoDapCovJsonTransform::printAxes
     */
    void testPrintAxes()
    {
        DBG(cerr << endl);
        try {
            libdap::DataDDS *test_DDS = makeTestDDS();

            //############################# DATA TEST ####################################
            DBG(cerr << endl << "FoCovJsonTest::testPrintAxes() - BEGIN" << endl);
            DBG(cerr << "FoCovJsonTest::testPrintAxes() - d_tmpDir: " << d_tmpDir << endl);
            string tmpFile(d_tmpDir + "/test_print_axes_representation.covjson");
            DBG(cerr << "FoCovJsonTest::testPrintAxes() - tmpFile: " << tmpFile << endl);

            FoDapCovJsonTransform ft(test_DDS);

            DBG(cerr << "FoCovJsonTest::testPrintAxes() - Calling FoDapCovJsonTransform::printAxes()" << endl);

            fstream output;
            output.open(tmpFile.c_str(), std::fstream::out);

            ft.addTestAxis("x", "[12.2, 13.5, 15.8]");
            ft.addTestAxis("y", "[33.2, 22.7, 16.9]");
            ft.addTestAxis("t", "[1.0, 2.0, 3.0]");

            ft.setTestAxesExistence(true, true, false, true);

            ft.addTestParameter("testId1", "testParam1", "Parameter", "float", "Celsius", "THIS IS A LONG NAME", "THIS IS A STANDARD NAME", "[3, 3, 3]", "[32765.2, 25222.7, 1431516.9, 3289741.2, 328974268.3]");

            ft.printAxes(output, "");

            ft.addTestAxis("z", "[351.0, 2132.0, 123.0, 4831.0]");

            ft.setTestAxesExistence(true, true, true, true);

            ft.printAxes(output, "");

            // Compare the result with the baseline file.
            string baseline = fileToString((string)TEST_SRC_DIR + "/baselines/print_axes_test.covjson.baseline");
            string result = fileToString(tmpFile);

            DBG(cerr << "FoCovJsonTest::testPrintAxes() - baseline: " << endl << endl << baseline << endl);
            DBG(cerr << "FoCovJsonTest::testPrintAxes() - result: " << endl << endl << result << endl);
            DBG(cerr << "FoCovJsonTest::testPrintAxes() - baseline.compare(result): " << baseline.compare(result) << endl);

            DBG(cerr << "FoCovJsonTest::testPrintAxes() - baseline length: " << baseline.length() << endl);
            DBG(cerr << "FoCovJsonTest::testPrintAxes() - result length: " << result.length() << endl);

            CPPUNIT_ASSERT(baseline.length() == result.length());
            CPPUNIT_ASSERT(baseline.compare(result) == 0);

            DBG(cerr << "FoCovJsonTest::testPrintAxes() - FoDapCovJsonTransform::printAxes() SUCCESS. Deleting DDS..." << endl);

            delete test_DDS;

            DBG(cerr << "FoCovJsonTest::testPrintAxes() - END" << endl);
        }
        catch (BESInternalError &e) {
            cerr << "BESInternalError: " << e.get_message() << endl;
            CPPUNIT_ASSERT(false);
        }
        catch (libdap::Error &e) {
            cerr << "Error: " << e.get_error_message() << endl;
            CPPUNIT_ASSERT(false);
        }
        catch (std::exception &e) {
            DBG(cerr << "std::exception: " << e.what() << endl);
            CPPUNIT_FAIL("Caught std::exception");
        }
        catch (...) {
            CPPUNIT_FAIL("Unknown Error!");
        }
    }

    /**
     * @brief For testing the FoDapCovJsonTransform::printReference
     */
    void testPrintReference()
    {
        DBG(cerr << endl);
        try {
            libdap::DataDDS *test_DDS = makeTestDDS();

            //############################# DATA TEST ####################################
            DBG(cerr << endl << "FoCovJsonTest::testPrintReference() - BEGIN" << endl);
            DBG(cerr << "FoCovJsonTest::testPrintReference() - d_tmpDir: " << d_tmpDir << endl);
            string tmpFile(d_tmpDir + "/test_print_reference_representation.covjson");
            DBG(cerr << "FoCovJsonTest::testPrintReference() - tmpFile: " << tmpFile << endl);

            FoDapCovJsonTransform ft(test_DDS);

            DBG(cerr << "FoCovJsonTest::testPrintReference() - Calling FoDapCovJsonTransform::printReference()" << endl);

            fstream output;
            output.open(tmpFile.c_str(), std::fstream::out);

            ft.addTestAxis("x", "[12.2, 13.5, 15.8]");
            ft.addTestAxis("y", "[33.2, 22.7, 16.9]");
            ft.addTestAxis("t", "[1.0, 2.0, 3.0]");

            ft.setTestAxesExistence(true, true, false, true);

            ft.addTestParameter("testId1", "testParam1", "Parameter", "float", "Celsius", "THIS IS A LONG NAME", "THIS IS A STANDARD NAME", "[3, 3, 3]", "[32765.2, 25222.7, 1431516.9, 3289741.2, 328974268.3]");

            ft.printReference(output, "");

            // Check to see if z prints when true
            ft.setTestAxesExistence(true, true, true, true);

            ft.printReference(output, "");

            // Compare the result with the baseline file.
            string baseline = fileToString((string)TEST_SRC_DIR + "/baselines/print_reference_test.covjson.baseline");
            string result = fileToString(tmpFile);

            DBG(cerr << "FoCovJsonTest::testPrintReference() - baseline: " << endl << endl << baseline << endl);
            DBG(cerr << "FoCovJsonTest::testPrintReference() - result: " << endl << endl << result << endl);
            DBG(cerr << "FoCovJsonTest::testPrintReference() - baseline.compare(result): " << baseline.compare(result) << endl);

            DBG(cerr << "FoCovJsonTest::testPrintReference() - baseline length: " << baseline.length() << endl);
            DBG(cerr << "FoCovJsonTest::testPrintReference() - result length: " << result.length() << endl);

            CPPUNIT_ASSERT(baseline.length() == result.length());
            CPPUNIT_ASSERT(baseline.compare(result) == 0);

            DBG(cerr << "FoCovJsonTest::testPrintReference() - FoDapCovJsonTransform::printReference() SUCCESS. Deleting DDS..." << endl);

            delete test_DDS;

            DBG(cerr << "FoCovJsonTest::testPrintReference() - END" << endl);
        }
        catch (BESInternalError &e) {
            cerr << "BESInternalError: " << e.get_message() << endl;
            CPPUNIT_ASSERT(false);
        }
        catch (libdap::Error &e) {
            cerr << "Error: " << e.get_error_message() << endl;
            CPPUNIT_ASSERT(false);
        }
        catch (std::exception &e) {
            DBG(cerr << "std::exception: " << e.what() << endl);
            CPPUNIT_FAIL("Caught std::exception");
        }
        catch (...) {
            CPPUNIT_FAIL("Unknown Error!");
        }
    }

    /**
     * @brief For testing the FoDapCovJsonTransform::printDomain
     */
    void testPrintDomain()
    {
        DBG(cerr << endl);
        try {
            libdap::DataDDS *test_DDS = makeTestDDS();

            //############################# DATA TEST ####################################
            DBG(cerr << endl << "FoCovJsonTest::testPrintDomain() - BEGIN" << endl);
            DBG(cerr << "FoCovJsonTest::testPrintDomain() - d_tmpDir: " << d_tmpDir << endl);
            string tmpFile(d_tmpDir + "/test_print_domain_representation.covjson");
            DBG(cerr << "FoCovJsonTest::testPrintDomain() - tmpFile: " << tmpFile << endl);

            FoDapCovJsonTransform ft(test_DDS);

            DBG(cerr << "FoCovJsonTest::testPrintDomain() - Calling FoDapCovJsonTransform::testPrintDomain()" << endl);

            fstream output;
            output.open(tmpFile.c_str(), std::fstream::out);

            ft.addTestAxis("x", "[12.2, 13.5, 15.8]");
            ft.addTestAxis("y", "[33.2, 22.7, 16.9]");
            ft.addTestAxis("t", "[1.0, 2.0, 3.0]");

            ft.setTestAxesExistence(true, true, false, true);

            ft.addTestParameter("testId1", "testParam1", "Parameter", "float", "Celsius", "THIS IS A LONG NAME", "THIS IS A STANDARD NAME", "[3, 3, 3]", "[32765.2, 25222.7, 1431516.9, 3289741.2, 328974268.3]");

            ft.printDomain(output, "");

            ft.addTestParameter("testId2", "testParam2", "Parameter", "integer", "Fahrenheit", "THIS IS A LONGER NAME", "THIS IS A MORE STANDARD NAME", "[1, 2, 3]", "[372, 142, 1142, 12, 45233]");

            ft.printDomain(output, "");

            // Test domain type printing
            ft.setTestDomainType(0); // Grid
            ft.printDomain(output, "");

            ft.setTestDomainType(1); // Vertical Profile
            ft.printDomain(output, "");

            ft.setTestDomainType(2); // Point Series
            ft.printDomain(output, "");

            ft.setTestDomainType(3); // Point
            ft.printDomain(output, "");

            // Compare the result with the baseline file.
            string baseline = fileToString((string)TEST_SRC_DIR + "/baselines/print_domain_test.covjson.baseline");
            string result = fileToString(tmpFile);

            DBG(cerr << "FoCovJsonTest::testPrintDomain() - baseline: " << endl << endl << baseline << endl);
            DBG(cerr << "FoCovJsonTest::testPrintDomain() - result: " << endl << endl << result << endl);
            DBG(cerr << "FoCovJsonTest::testPrintDomain() - baseline.compare(result): " << baseline.compare(result) << endl);

            DBG(cerr << "FoCovJsonTest::testPrintDomain() - baseline length: " << baseline.length() << endl);
            DBG(cerr << "FoCovJsonTest::testPrintDomain() - result length: " << result.length() << endl);

            CPPUNIT_ASSERT(baseline.length() == result.length());
            CPPUNIT_ASSERT(baseline.compare(result) == 0);

            DBG(cerr << "FoCovJsonTest::testPrintDomain() - FoDapCovJsonTransform::printDomain() SUCCESS. Deleting DDS..." << endl);

            delete test_DDS;

            DBG(cerr << "FoCovJsonTest::testPrintDomain() - END" << endl);
        }
        catch (BESInternalError &e) {
            cerr << "BESInternalError: " << e.get_message() << endl;
            CPPUNIT_ASSERT(false);
        }
        catch (libdap::Error &e) {
            cerr << "Error: " << e.get_error_message() << endl;
            CPPUNIT_ASSERT(false);
        }
        catch (std::exception &e) {
            DBG(cerr << "std::exception: " << e.what() << endl);
            CPPUNIT_FAIL("Caught std::exception");
        }
        catch (...) {
            CPPUNIT_FAIL("Unknown Error!");
        }
    }

    /**
     * @brief For testing the FoDapCovJsonTransform::printParameters
     */
    void testPrintParameters()
    {
        DBG(cerr << endl);
        try {
            libdap::DataDDS *test_DDS = makeTestDDS();

            //############################# DATA TEST ####################################
            DBG(cerr << endl << "FoCovJsonTest::testPrintParameters() - BEGIN" << endl);
            DBG(cerr << "FoCovJsonTest::testPrintParameters() - d_tmpDir: " << d_tmpDir << endl);
            string tmpFile(d_tmpDir + "/test_print_parameters_representation.covjson");
            DBG(cerr << "FoCovJsonTest::testPrintParameters() - tmpFile: " << tmpFile << endl);

            FoDapCovJsonTransform ft(test_DDS);

            DBG(cerr << "FoCovJsonTest::testPrintParameters() - Calling FoDapCovJsonTransform::printParameters()" << endl);

            fstream output;
            output.open(tmpFile.c_str(), std::fstream::out);

            ft.addTestAxis("x", "[12.2, 13.5, 15.8]");
            ft.addTestAxis("y", "[33.2, 22.7, 16.9]");
            ft.addTestAxis("t", "[1.0, 2.0, 3.0]");

            ft.setTestAxesExistence(true, true, false, true);

            ft.addTestParameter("testId1", "testParam1", "Parameter", "float", "Celsius", "THIS IS A LONG NAME", "THIS IS A STANDARD NAME", "[3, 3, 3]", "[32765.2, 25222.7, 1431516.9, 3289741.2, 328974268.3]");

            ft.printParameters(output, "");

            ft.addTestParameter("testId2", "testParam2", "Parameter", "integer", "Fahrenheit", "THIS IS A LONGER NAME", "THIS IS A MORE STANDARD NAME", "[1, 2, 3]", "[372, 142, 1142, 12, 45233]");

            ft.printParameters(output, "");

            ft.addTestParameter("testId3", "testParam3", "Parameter", "integer", "Kelvin", "THIS IS THE LONGEST NAME", "THIS IS AN EVEN MORE STANDARD NAME", "[3, 2, 1]", "[32521, 576784, 345765, 343455, 8900645]");

            ft.printParameters(output, "");

            // Compare the result with the baseline file.
            string baseline = fileToString((string)TEST_SRC_DIR + "/baselines/print_parameters_test.covjson.baseline");
            string result = fileToString(tmpFile);

            DBG(cerr << "FoCovJsonTest::testPrintParameters() - baseline: " << endl << endl << baseline << endl);
            DBG(cerr << "FoCovJsonTest::testPrintParameters() - result: " << endl << endl << result << endl);
            DBG(cerr << "FoCovJsonTest::testPrintParameters() - baseline.compare(result): " << baseline.compare(result) << endl);

            DBG(cerr << "FoCovJsonTest::testPrintParameters() - baseline length: " << baseline.length() << endl);
            DBG(cerr << "FoCovJsonTest::testPrintParameters() - result length: " << result.length() << endl);

            CPPUNIT_ASSERT(baseline.length() == result.length());
            CPPUNIT_ASSERT(baseline.compare(result) == 0);

            DBG(cerr << "FoCovJsonTest::testPrintParameters() - FoDapCovJsonTransform::printParameters() SUCCESS. Deleting DDS..." << endl);

            delete test_DDS;

            DBG(cerr << "FoCovJsonTest::testPrintParameters() - END" << endl);
        }
        catch (BESInternalError &e) {
            cerr << "BESInternalError: " << e.get_message() << endl;
            CPPUNIT_ASSERT(false);
        }
        catch (libdap::Error &e) {
            cerr << "Error: " << e.get_error_message() << endl;
            CPPUNIT_ASSERT(false);
        }
        catch (std::exception &e) {
            DBG(cerr << "std::exception: " << e.what() << endl);
            CPPUNIT_FAIL("Caught std::exception");
        }
        catch (...) {
            CPPUNIT_FAIL("Unknown Error!");
        }
    }

    /**
     * @brief For testing the FoDapCovJsonTransform::printRanges
     */
    void testPrintRanges()
    {
        DBG(cerr << endl);
        try {
            libdap::DataDDS *test_DDS = makeTestDDS();

            //############################# DATA TEST ####################################
            DBG(cerr << endl << "FoCovJsonTest::testPrintRanges() - BEGIN" << endl);
            DBG(cerr << "FoCovJsonTest::testPrintRanges() - d_tmpDir: " << d_tmpDir << endl);
            string tmpFile(d_tmpDir + "/test_print_ranges_representation.covjson");
            DBG(cerr << "FoCovJsonTest::testPrintRanges() - tmpFile: " << tmpFile << endl);

            FoDapCovJsonTransform ft(test_DDS);

            DBG(cerr << "FoCovJsonTest::testPrintRanges() - Calling FoDapCovJsonTransform::printRanges()" << endl);

            fstream output;
            output.open(tmpFile.c_str(), std::fstream::out);

            ft.addTestAxis("x", "[12.2, 13.5, 15.8]");
            ft.addTestAxis("y", "[33.2, 22.7, 16.9]");
            ft.addTestAxis("t", "[1.0, 2.0, 3.0]");

            ft.setTestAxesExistence(true, true, false, true);

            ft.addTestParameter("testId1", "testParam1", "Parameter", "float", "Celsius", "THIS IS A LONG NAME", "THIS IS A STANDARD NAME", "[3, 3, 3]", "[32765.2, 25222.7, 1431516.9, 3289741.2, 328974268.3]");

            ft.printRanges(output, "");

            // Compare the result with the baseline file.
            string baseline = fileToString((string)TEST_SRC_DIR + "/baselines/print_ranges_test.covjson.baseline");
            string result = fileToString(tmpFile);

            DBG(cerr << "FoCovJsonTest::testPrintRanges() - baseline: " << endl << endl << baseline << endl);
            DBG(cerr << "FoCovJsonTest::testPrintRanges() - result: " << endl << endl << result << endl);
            DBG(cerr << "FoCovJsonTest::testPrintRanges() - baseline.compare(result): " << baseline.compare(result) << endl);

            DBG(cerr << "FoCovJsonTest::testPrintRanges() - baseline length: " << baseline.length() << endl);
            DBG(cerr << "FoCovJsonTest::testPrintRanges() - result length: " << result.length() << endl);

            CPPUNIT_ASSERT(baseline.length() == result.length());
            CPPUNIT_ASSERT(baseline.compare(result) == 0);

            DBG(cerr << "FoCovJsonTest::testPrintRanges() - FoDapCovJsonTransform::printRanges() SUCCESS. Deleting DDS..." << endl);

            delete test_DDS;

            DBG(cerr << "FoCovJsonTest::testPrintRanges() - END" << endl);
        }
        catch (BESInternalError &e) {
            cerr << "BESInternalError: " << e.get_message() << endl;
            CPPUNIT_ASSERT(false);
        }
        catch (libdap::Error &e) {
            cerr << "Error: " << e.get_error_message() << endl;
            CPPUNIT_ASSERT(false);
        }
        catch (std::exception &e) {
            DBG(cerr << "std::exception: " << e.what() << endl);
            CPPUNIT_FAIL("Caught std::exception");
        }
        catch (...) {
            CPPUNIT_FAIL("Unknown Error!");
        }
    }

        /**
     * @brief For testing the FoDapCovJsonTransform::printRanges
     */
    void testPrintCoverage()
    {
        DBG(cerr << endl);
        try {
            libdap::DataDDS *test_DDS = makeTestDDS();

            //############################# DATA TEST ####################################
            DBG(cerr << endl << "FoCovJsonTest::testPrintCoverage() - BEGIN" << endl);
            DBG(cerr << "FoCovJsonTest::testPrintCoverage() - d_tmpDir: " << d_tmpDir << endl);
            string tmpFile(d_tmpDir + "/test_print_coverage_representation.covjson");
            DBG(cerr << "FoCovJsonTest::testPrintCoverage() - tmpFile: " << tmpFile << endl);

            FoDapCovJsonTransform ft(test_DDS);

            DBG(cerr << "FoCovJsonTest::testPrintCoverage() - Calling FoDapCovJsonTransform::printCoverage()" << endl);

            fstream output;
            output.open(tmpFile.c_str(), std::fstream::out);

            ft.addTestAxis("x", "[12.2, 13.5, 15.8]");
            ft.addTestAxis("y", "[33.2, 22.7, 16.9]");
            ft.addTestAxis("t", "[1.0, 2.0, 3.0]");

            ft.setTestAxesExistence(true, true, false, true);

            ft.addTestParameter("testId1", "testParam1", "Parameter", "float", "Celsius", "THIS IS A LONG NAME", "THIS IS A STANDARD NAME", "[3, 3, 3]", "[32765.2, 25222.7, 1431516.9, 3289741.2, 328974268.3]");

            ft.printCoverage(output, "");

            // Compare the result with the baseline file.
            string baseline = fileToString((string)TEST_SRC_DIR + "/baselines/print_coverage_test.covjson.baseline");
            string result = fileToString(tmpFile);

            DBG(cerr << "FoCovJsonTest::testPrintCoverage() - baseline: " << endl << endl << baseline << endl);
            DBG(cerr << "FoCovJsonTest::testPrintCoverage() - result: " << endl << endl << result << endl);
            DBG(cerr << "FoCovJsonTest::testPrintCoverage() - baseline.compare(result): " << baseline.compare(result) << endl);

            DBG(cerr << "FoCovJsonTest::testPrintCoverage() - baseline length: " << baseline.length() << endl);
            DBG(cerr << "FoCovJsonTest::testPrintCoverage() - result length: " << result.length() << endl);

            CPPUNIT_ASSERT(baseline.length() == result.length());
            CPPUNIT_ASSERT(baseline.compare(result) == 0);

            DBG(cerr << "FoCovJsonTest::testPrintCoverage() - FoDapCovJsonTransform::printCoverage() SUCCESS. Deleting DDS..." << endl);

            delete test_DDS;

            DBG(cerr << "FoCovJsonTest::testPrintCoverage() - END" << endl);
        }
        catch (BESInternalError &e) {
            cerr << "BESInternalError: " << e.get_message() << endl;
            CPPUNIT_ASSERT(false);
        }
        catch (libdap::Error &e) {
            cerr << "Error: " << e.get_error_message() << endl;
            CPPUNIT_ASSERT(false);
        }
        catch (std::exception &e) {
            DBG(cerr << "std::exception: " << e.what() << endl);
            CPPUNIT_FAIL("Caught std::exception");
        }
        catch (...) {
            CPPUNIT_FAIL("Unknown Error!");
        }
    }

    /**
     * @brief Sets up a test DDS with atomic variables
     */
    libdap::DataDDS *makeTestDDS()
    {
        // build a DataDDS of simple types and set values for each of the simple types.
        libdap::DataDDS *dds = new libdap::DataDDS(NULL, "TestDataset");

        // ###################  SIMPLE TYPES ###################
        libdap::Byte b("byte");
        b.set_value(28);
        b.set_send_p(true);
        dds->add_var(&b);

        libdap::Int16 i16("i16");
        i16.set_value(-2048);
        i16.set_send_p(true);
        dds->add_var(&i16);

        libdap::Int32 i32("i32");
        i32.set_value(-105467);
        i32.set_send_p(true);
        dds->add_var(&i32);

        libdap::UInt16 ui16("ui16");
        ui16.set_value(2048);
        ui16.set_send_p(true);
        dds->add_var(&ui16);

        libdap::UInt32 ui32("ui32");
        ui32.set_value(105467);
        ui32.set_send_p(true);
        dds->add_var(&ui32);

        libdap::Float32 f32("f32");
        f32.set_value(5.7866);
        f32.set_send_p(true);
        dds->add_var(&f32);

        libdap::Float64 f64("f64");
        f64.set_value(10245.1234);
        f64.set_send_p(true);
        dds->add_var(&f64);

        libdap::Str s("str");
        s.set_value("This is a String Value");
        s.set_send_p(true);
        dds->add_var(&s);

        // ###################  ARRAYS OF SIMPLE TYPES ###################
        libdap::Float64 tmplt("oneDArrayF64");
        libdap::Array oneDArrayF64("oneDArrayF64", &tmplt);

        int dim1Size = 2;
        double pi = atan(1) * 4;
        libdap::dods_float64 oneDdata[dim1Size];
        for (long i = 0; i < dim1Size; i++) {
            oneDdata[i] = pi * (i * 0.1);
        }

        oneDArrayF64.append_dim(dim1Size, "dim1");
        oneDArrayF64.set_value(oneDdata, dim1Size);
        oneDArrayF64.set_send_p(true);
        dds->add_var(&oneDArrayF64);

        libdap::Float64 tmplt2("twoDArrayF64");
        libdap::Array twoDArrayF64("twoDArrayF64", &tmplt2);

        int dim2Size = 4;
        int totalSize = dim1Size * dim2Size;
        libdap::dods_float64 twoDdata[totalSize];
        for (long i = 0; i < totalSize; i++) {
            twoDdata[i] = pi * (i * 0.01);
        }

        twoDArrayF64.append_dim(dim1Size, "dim1");
        twoDArrayF64.append_dim(dim2Size, "dim2");
        twoDArrayF64.set_value(twoDdata, totalSize);
        twoDArrayF64.set_send_p(true);
        dds->add_var(&twoDArrayF64);

        libdap::UInt32 tmplt4("twoDArrayUI32");
        libdap::Array twoDArrayUI32("twoDArrayUI32", &tmplt4);

        totalSize = dim1Size * dim2Size;
        libdap::dods_uint32 uint32data[totalSize];
        unsigned int val = 0;
        for (long i = 0; i < totalSize; i++) {
            val = i + val;
            uint32data[i] = val;
        }

        twoDArrayUI32.append_dim(dim1Size, "dim1");
        twoDArrayUI32.append_dim(dim2Size, "dim2");
        twoDArrayUI32.set_value(uint32data, totalSize);
        twoDArrayUI32.set_send_p(true);
        dds->add_var(&twoDArrayUI32);

        libdap::Float64 tmplt3("threeDArrayF64");
        libdap::Array threeDArrayF64("threeDArrayF64", &tmplt3);

        int dim3Size = 5;
        totalSize = dim1Size * dim2Size * dim3Size;
        libdap::dods_float64 threeDdata[totalSize];
        for (long i = 0; i < totalSize; i++) {
            threeDdata[i] = pi * (i * 0.001);
        }

        threeDArrayF64.append_dim(dim1Size, "dim1");
        threeDArrayF64.append_dim(dim2Size, "dim2");
        threeDArrayF64.append_dim(dim3Size, "dim3");
        threeDArrayF64.set_value(threeDdata, totalSize);
        threeDArrayF64.set_send_p(true);
        dds->add_var(&threeDArrayF64);

        // ###################  STRUCTURE   ###################
        libdap::Structure structure("test_structure");

        libdap::Byte sb("byte");
        sb.set_value(238);
        sb.set_send_p(true);
        structure.add_var(&sb);

        libdap::Int16 si16("i16");
        si16.set_value(-1041);
        si16.set_send_p(true);
        structure.add_var(&si16);

        libdap::Str fooStr("fooStr");
        fooStr.set_value("This is the structure foo string.");
        fooStr.set_send_p(true);
        structure.add_var(&fooStr);

        structure.set_send_p(true);
        dds->add_var(&structure);

        // ###################  SEQUENCE   ###################
        libdap::Sequence sequence("test_sequence");

        libdap::Byte sqb("byte");
        sqb.set_value(238);
        sqb.set_send_p(true);
        sequence.add_var(&sqb);

        libdap::Int16 sqi16("i16");
        sqi16.set_value(-1041);
        sqi16.set_send_p(true);
        sequence.add_var(&sqi16);

        libdap::Str sfooStr("fooStr");
        sfooStr.set_value("This is the sequence foo string.");
        sfooStr.set_send_p(true);
        sequence.add_var(&sfooStr);

        sequence.set_send_p(true);
        dds->add_var(&sequence);

        // ###################  GRID   ###################
        libdap::Grid grid("test_grid");

        libdap::Float64 sstTemplate("test_grid");
        libdap::Array sstArray("test_grid", &sstTemplate);

        libdap::Float64 lngtemplate("longitude");
        libdap::Array lngArray("longitude", &lngtemplate);

        libdap::Float64 lattemplate("latitude");
        libdap::Array latArray("latitude", &lattemplate);

        libdap::Float64 timetemplate("time_origin");
        libdap::Array timeArray("time_origin", &timetemplate);

        int lngSize = 36;
        int latSize = 18;
        int timeSize = 9;
        totalSize = lngSize * latSize;
        libdap::dods_float64 testData[totalSize];
        libdap::dods_float64 latData[latSize];
        libdap::dods_float64 lngData[lngSize];
        libdap::dods_float64 timeData[timeSize];

        int i = 0;
        for(int lngVal = 0; lngVal < lngSize; lngVal++) {
            lngData[lngVal] = (lngVal - lngSize / 2) + 0.0;
            for(int latVal = 0; latVal < latSize; latVal++) {
                latData[latVal] = (latVal - latSize / 2) + 0.0;
                testData[i] = pi * ((lngData[lngVal] + latData[latVal]) * 0.01);
                i++;
            }
        }

        for(int timeVal = 0; timeVal < timeSize; timeVal++) {
            timeData[timeVal] = (timeVal / 2) + 0.0;;
        }

        sstArray.append_dim(lngSize, "longitude");
        sstArray.append_dim(latSize, "latitude");
        sstArray.append_dim(timeSize, "time_origin");
        sstArray.set_value(testData, totalSize); // creates space and uses memcopy to transfer values.
        sstArray.set_send_p(true);
        grid.add_var(&sstArray, libdap::array); // add a copy

        lngArray.append_dim(lngSize, "longitude");
        lngArray.set_value(lngData, lngSize); // creates space and uses memcopy to transfer values.
        lngArray.set_send_p(true);
        grid.add_map(&lngArray, true);

        latArray.append_dim(latSize, "latitude");
        latArray.set_value(latData, latSize); // creates space and uses memcopy to transfer values.
        latArray.set_send_p(true);
        grid.add_map(&latArray, true);

        timeArray.append_dim(timeSize, "time_origin");
        timeArray.set_value(timeData, timeSize); // creates space and uses memcopy to transfer values.
        timeArray.set_send_p(true);
        grid.add_map(&timeArray, true);

        grid.set_send_p(true);
        dds->add_var(&grid); // add a copy

        DBG(cerr << "FoCovJsonTest::makeTestDDS(): " << endl);
        DBG(dds->print_constrained(cerr));

        return dds;
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(FoCovJsonTest);

} // namespace focovjson

int main(int argc, char*argv[])
{
    int option_char;
    while ((option_char = getopt(argc, argv, "dh")) != -1)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            cerr << "##### DEBUG is ON" << endl;
            break;
        case 'h': {     // Help - show test names
            std::cerr << "Usage: FoCovJsonTest has the following tests:" << std::endl;
            const std::vector<CppUnit::Test*> &tests = focovjson::FoCovJsonTest::suite()->getTests();
            unsigned int prefix_len = focovjson::FoCovJsonTest::suite()->getName().append("::").length();
            for (std::vector<CppUnit::Test*>::const_iterator i = tests.begin(), e = tests.end(); i != e; ++i) {
                std::cerr << (*i)->getName().replace(0, prefix_len, "") << std::endl;
            }
            break;
        }
        default:
            // KEEP THE NIGHTLY BUILD CLEAN!!!
            // cerr << "##### DEBUG is OFF" << endl;
            break;
        }

    argc -= optind;
    argv += optind;

    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    bool wasSuccessful = true;
    string test = "";
    if (0 == argc) {
        // Run them all
        wasSuccessful = runner.run("");
    }
    else {
        int i = 0;
        while (i < argc) {
            if (debug) {
                cerr << "Running " << argv[i] << endl;
            }
            test = focovjson::FoCovJsonTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
