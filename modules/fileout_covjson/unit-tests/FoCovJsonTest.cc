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
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// (c) COPYRIGHT URI/MIT 1995-1999
// Please read the full copyright statement in the file COPYRIGHT_URI.
//

//#include <cstdio>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>
#include <math.h>       /* atan */

#include <GetOpt.h>
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
#include "FoDapCovJsonValidation.h"

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

    // Add unit test functions to the FoCovJsonTest test suite here
    CPPUNIT_TEST(testAbstractObjectMetadataRepresentation);
    CPPUNIT_TEST(testAbstractObjectDataRepresentation);
    CPPUNIT_TEST(testWriteAxesMetadata);
    CPPUNIT_TEST(testWriteParameterMetadata);
    //CPPUNIT_TEST(testGetParameterAttributes);
    //CPPUNIT_TEST(testTransformAtomic);
    //CPPUNIT_TEST(testTransformAxesWorker);
    //CPPUNIT_TEST(testTransformReferenceWorker);
    //CPPUNIT_TEST(testTransformParametersWorker);
    //CPPUNIT_TEST(testTransformRangesWorker);
    //CPPUNIT_TEST(testCovjsonSimpleTypeArray);
    //CPPUNIT_TEST(testCovjsonStringArray);

    CPPUNIT_TEST_SUITE_END();

    /**
     * @brief
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
            FoDapCovJsonValidation fv(test_DDS);
            fv.validateDataset();

            DBG(cerr << "FoCovJsonTest::testAbstractObjectMetadataRepresentation() - Calling FoDapCovJsonTransform::transform(false) - Send metadata" << endl);

            fstream output;
            output.open(tmpFile.c_str(), std::fstream::out);
            ft.transform(output, false, fv);

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

            DBG(cerr << "FoCovJsonTest::testAbstractObjectMetadataRepresentation() - FoDapCovJsonTransform::transform() SUCCESS. Deleting DDS..." << endl);

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
     * @brief
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
            FoDapCovJsonValidation fv(test_DDS);
            fv.validateDataset();

            DBG(cerr << "FoCovJsonTest::testAbstractObjectDataRepresentation() - Calling FoDapCovJsonTransform::transform(true) - Send data." << endl);

            fstream output;
            output.open(tmpFile.c_str(), std::fstream::out);
            ft.transform(output, true, fv);

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

            DBG(cerr << "FoCovJsonTest::testAbstractObjectDataRepresentation() - FoDapCovJsonTransform::transform() SUCCESS. Deleting DDS..." << endl);

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
     * @brief
     */
    void testWriteAxesMetadata()
    {
        DBG(cerr << endl);
        try {
            libdap::DataDDS *test_DDS = makeTestDDS();
            libdap::DDS::Vars_iter vi = test_DDS->var_begin();
            libdap::BaseType *bt = *vi;
            string indent = "";

            //############################# DATA TEST ####################################
            DBG(cerr << endl << "FoCovJsonTest::testWriteAxesMetadata() - BEGIN" << endl);
            DBG(cerr << "FoCovJsonTest::testWriteAxesMetadata() - d_tmpDir: " << d_tmpDir << endl);
            string tmpFile(d_tmpDir + "/test_write_axes_metadata.covjson");
            DBG(cerr << "FoCovJsonTest::testWriteAxesMetadata() - tmpFile: " << tmpFile << endl);

            FoDapCovJsonTransform ft(test_DDS);
            FoDapCovJsonValidation fv(test_DDS);
            fv.validateDataset();

            DBG(cerr << "FoCovJsonTest::testWriteAxesMetadata() - Calling FoDapCovJsonTransform::testWriteAxesMetadata()" << endl);

            fstream output;
            output.open(tmpFile.c_str(), std::fstream::out);

            ft.testWriteAxesMetadata(output, bt, indent);

            // Compare the result with the baseline file.
            string baseline = fileToString((string)TEST_SRC_DIR + "/baselines/test_write_axes_metadata.covjson.baseline");
            string result = fileToString(tmpFile);

            DBG(cerr << "FoCovJsonTest::testWriteAxesMetadata() - baseline:" << endl << endl << baseline << endl);
            DBG(cerr << "FoCovJsonTest::testWriteAxesMetadata() - result:" << endl << endl << result << endl);
            DBG(cerr << "FoCovJsonTest::testWriteAxesMetadata() - baseline.compare(result): " << baseline.compare(result) << endl);

            DBG(cerr << "FoCovJsonTest::testWriteAxesMetadata() - baseline length: " << baseline.length() << endl);
            DBG(cerr << "FoCovJsonTest::testWriteAxesMetadata() - result length: " << result.length() << endl);

            CPPUNIT_ASSERT(baseline.length() == result.length());
            CPPUNIT_ASSERT(baseline.compare(result) == 0);

            DBG(cerr << "FoCovJsonTest::testWriteAxesMetadata() - FoDapCovJsonTransform::transform() SUCCESS. Deleting DDS..." << endl);

            delete test_DDS;

            DBG(cerr << "FoCovJsonTest::testWriteAxesMetadata() - END" << endl);
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
     * @brief
     */
    void testWriteParameterMetadata()
    {
        DBG(cerr << endl);
        try {
            libdap::DataDDS *test_DDS = makeTestDDS();
            libdap::DDS::Vars_iter vi = test_DDS->var_begin();
            libdap::BaseType *bt = *vi;
            string indent = "";

            //############################# DATA TEST ####################################
            DBG(cerr << endl << "FoCovJsonTest::testWriteParameterMetadata() - BEGIN" << endl);
            DBG(cerr << "FoCovJsonTest::testWriteParameterMetadata() - d_tmpDir: " << d_tmpDir << endl);
            string tmpFile(d_tmpDir + "/test_write_parameter_metadata.covjson");
            DBG(cerr << "FoCovJsonTest::testWriteParameterMetadata() - tmpFile: " << tmpFile << endl);

            FoDapCovJsonTransform ft(test_DDS);
            FoDapCovJsonValidation fv(test_DDS);
            fv.validateDataset();

            DBG(cerr << "FoCovJsonTest::testWriteParameterMetadata() - Calling FoDapCovJsonTransform::testWriteParameterMetadata()" << endl);

            fstream output;
            output.open(tmpFile.c_str(), std::fstream::out);

            ft.testWriteParameterMetadata(output, bt, indent);

            // Compare the result with the baseline file.
            string baseline = fileToString((string)TEST_SRC_DIR + "/baselines/test_write_parameter_metadata.covjson.baseline");
            string result = fileToString(tmpFile);

            DBG(cerr << "FoCovJsonTest::testWriteParameterMetadata() - baseline:" << endl << endl << baseline << endl);
            DBG(cerr << "FoCovJsonTest::testWriteParameterMetadata() - result:" << endl << endl << result << endl);
            DBG(cerr << "FoCovJsonTest::testWriteParameterMetadata() - baseline.compare(result): " << baseline.compare(result) << endl);

            DBG(cerr << "FoCovJsonTest::testWriteParameterMetadata() - baseline length: " << baseline.length() << endl);
            DBG(cerr << "FoCovJsonTest::testWriteParameterMetadata() - result length: " << result.length() << endl);

            CPPUNIT_ASSERT(baseline.length() == result.length());
            CPPUNIT_ASSERT(baseline.compare(result) == 0);

            DBG(cerr << "FoCovJsonTest::testWriteParameterMetadata() - FoDapCovJsonTransform::transform() SUCCESS. Deleting DDS..." << endl);

            delete test_DDS;

            DBG(cerr << "FoCovJsonTest::testWriteParameterMetadata() - END" << endl);
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
     * @brief
     */
    void testGetParameterAttributes()
    {

    }

    /**
     * @brief
     */
    void testTransformAtomic()
    {

    }

    /**
     * @brief
     */
    void testTransformAxesWorker()
    {

    }

    /**
     * @brief
     */
    void testTransformReferenceWorker()
    {

    }

    /**
     * @brief
     */
    void testTransformParametersWorker()
    {

    }

    /**
     * @brief
     */
    void testTransformRangesWorker()
    {

    }

    /**
     * @brief
     */
    void testCovjsonSimpleTypeArray()
    {

    }

    /**
     * @brief
     */
    void testCovjsonStringArray()
    {

    }

    /**
     * @brief
     */
    libdap::DataDDS *makeSimpleTypesDDS()
    {
        // Build a DataDDS of simple types and set values for each of the simple types.
        libdap::DataDDS *dds = new libdap::DataDDS(NULL, "SimpleTypes");

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

        return dds;
    }

    /**
     * @brief
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

        int lngSize = 36;
        int latSize = 18;
        totalSize = lngSize * latSize;
        libdap::dods_float64 testData[totalSize];
        libdap::dods_float64 latData[latSize];
        libdap::dods_float64 lngData[lngSize];

        int i = 0;
        for (int lngVal = 0; lngVal < lngSize; lngVal++) {
            lngData[lngVal] = (lngVal - lngSize / 2) + 0.0;
            for (int latVal = 0; latVal < latSize; latVal++) {
                latData[latVal] = (latVal - latSize / 2) + 0.0;
                testData[i] = pi * ((lngData[lngVal] + latData[latVal]) * 0.01);
                i++;
            }
        }

        sstArray.append_dim(lngSize, "longitude");
        sstArray.append_dim(latSize, "latitude");
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
    GetOpt getopt(argc, argv, "dh");
    int option_char;
    while ((option_char = getopt()) != -1)
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

    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    bool wasSuccessful = true;
    string test = "";
    int i = getopt.optind;
    if (i == argc) {
        // Run them all
        wasSuccessful = runner.run("");
    }
    else {
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
