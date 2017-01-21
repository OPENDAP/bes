// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2016 OPeNDAP, Inc.
// Author: Nathan David Potter <ndp@opendap.org>, James Gallagher
// <jgallagher@opendap.org>
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

#include <memory>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <DMR.h>

#include <BESDebug.h>

#include "H4ByteStream.h"
#include "DmrppArray.h"
#include "DmrppByte.h"
#include "DmrppCommon.h"
#include "DmrppD4Enum.h"
#include "DmrppD4Group.h"
#include "DmrppD4Opaque.h"
#include "DmrppD4Sequence.h"
#include "DmrppFloat32.h"
#include "DmrppFloat64.h"
#include "DmrppInt16.h"
#include "DmrppInt32.h"
#include "DmrppInt64.h"
#include "DmrppInt8.h"
#include "DmrppModule.h"
#include "DmrppStr.h"
#include "DmrppStructure.h"
#include "DmrppUInt16.h"
#include "DmrppUInt32.h"
#include "DmrppUInt64.h"
#include "DmrppUrl.h"

#include "DmrppParserSax2.h"
#include "DmrppRequestHandler.h"
#include "DmrppTypeFactory.h"

#include "GetOpt.h"
#include "test_config.h"
#include "util.h"

using namespace libdap;

static bool debug = false;

namespace dmrpp {

class DmrppChunkedReadTest: public CppUnit::TestFixture {
private:
    DmrppParserSax2 parser;

public:
    // Called once before everything gets tested
    DmrppChunkedReadTest() :
            parser()
    {
    }

    // Called at the end of the test
    ~DmrppChunkedReadTest()
    {
    }

    // Called before each test
    void setUp()
    {
        if (debug) BESDebug::SetUp("cerr,dmrpp");
    }

    // Called after each test
    void tearDown()
    {
    }

    /**
     * Evaluates a D4Group against exected values for the name, number of child
     * groups and the number of child variables.
     */
    void checkGroupsAndVars(D4Group *grp, string name, int expectedNumGrps, int expectedNumVars)
    {
        CPPUNIT_ASSERT(grp);
        BESDEBUG("dmrpp", "Checking D4Group '" << grp->name() << "'" << endl);

        CPPUNIT_ASSERT(grp->name() == name);

        int numGroups = grp->grp_end() - grp->grp_begin();
        BESDEBUG("dmrpp", "The D4Group '" << grp->name() << "' has " << numGroups << " child groups." << endl);
        CPPUNIT_ASSERT(numGroups == expectedNumGrps);

        int numVars = grp->var_end() - grp->var_begin();
        BESDEBUG("dmrpp", "The D4Group '" << grp->name() << "' has " << numVars << " child variables." << endl);
        CPPUNIT_ASSERT(numVars == expectedNumVars);
    }

    /**
     * This is a test hack in which we capitalize on the fact that we know
     * that the test Dmrpp files all have "local" names stored for their
     * xml:base URLs. Here we make these full paths on the file system
     * so that the source data files can be located at test runtime.
     */
    void set_data_url_in_chunks(DmrppCommon *dc)
    {
        // Get the chunks and make sure there's at least one
        vector<H4ByteStream> *chunks = dc->get_chunk_vec();
        CPPUNIT_ASSERT((*chunks).size() > 0);
        // Tweak the data URLs for the test
        for (unsigned int i = 0; i < (*chunks).size(); i++) {
            BESDEBUG("dmrpp", "chunk_refs[" << i << "]: " << (*chunks)[i].to_string() << endl);
        }
        for (unsigned int i = 0; i < (*chunks).size(); i++) {
            string data_url = BESUtil::assemblePath(TEST_DMRPP_CATALOG, (*chunks)[i].get_data_url(), true);
            data_url = "file://" + data_url;
            (*chunks)[i].set_data_url(data_url);
        }
        for (unsigned int i = 0; i < (*chunks).size(); i++) {
            BESDEBUG("dmrpp", "altered chunk_refs[" << i << "]: " << (*chunks)[i].to_string() << endl);
        }
    }

    /**
     * Checks name, reads data, checks # of read bytes.
     */
    void read_var_check_name_and_length(DmrppArray *array, string name, int length)
    {
        BESDEBUG("dmrpp", __func__ << "() - array->name(): " << array->name() << endl);
        CPPUNIT_ASSERT(array->name() == name);

        set_data_url_in_chunks(array);

        // Read the data
        array->read();
        BESDEBUG("dmrpp", __func__ << "() - array->length(): " << array->length() << endl);
        CPPUNIT_ASSERT(array->length() == length);
    }

    /**
     * Since we have a lot of test data files that contain a single array here
     * is a complete test of reading the array and verifying its content.
     */
    void check_f32_test_array(string filename, string variable_name, unsigned long long array_length)
    {
        auto_ptr<DMR> dmr(new DMR);
        DmrppTypeFactory dtf;
        dmr->set_factory(&dtf);
        dods_float32 test_float32;

        BESDEBUG("dmrpp", __func__ << "() - Opening: " << filename << endl);

        ifstream in(filename.c_str());
        parser.intern(in, dmr.get(), debug);
        BESDEBUG("dmrpp", __func__ << "() - Parsing complete"<< endl);

        // Check to make sure we have something that smells like our test array
        D4Group *root = dmr->root();
        checkGroupsAndVars(root, "/", 0, 1);
        // Walk the vars and testy testy
        D4Group::Vars_iter vIter = root->var_begin();
        try {
            // Read the variable and transfer the data
            DmrppArray *var = dynamic_cast<DmrppArray*>(*vIter);
            read_var_check_name_and_length(var, variable_name, array_length);
            vector<dods_float32> values(var->length());
            var->value(&values[0]);

            // Test data set is incrementally valued: Check Them All!
            for (unsigned long long a_index = 0; a_index < array_length; a_index++) {
                test_float32 = a_index;
                if (!double_eq(values[a_index], test_float32))
                BESDEBUG("dmrpp",
                        "values[" << a_index << "]: " << values[a_index] << "  test_float32: " << test_float32 << endl);
                CPPUNIT_ASSERT(double_eq(values[a_index], test_float32));
            }
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
        catch (std::exception &e) {
            CPPUNIT_FAIL(e.what());
        }

        CPPUNIT_ASSERT("Passed");
    }

    /**
     * Tests oneD array against a CE with start stride and stop that
     * retrieves data from all chunks. The stride >1 means that each value
     * must be individually copied.
     */
    void test_chunked_oneD_CE_00()
    {
        string chnkd_oneD = string(TEST_DATA_DIR).append("/").append("chunked_oneD.h5.dmrpp");

        unsigned long long array_length = 40000;
        string variable_name = "d_4_chunks";
        string filename = chnkd_oneD;
        auto_ptr<DMR> dmr(new DMR);
        DmrppTypeFactory dtf;
        dmr->set_factory(&dtf);
        dods_float32 test_float32;

        BESDEBUG("dmrpp", __func__ << "() - Opening: " << filename << endl);

        ifstream in(filename.c_str());
        parser.intern(in, dmr.get(), debug);
        BESDEBUG("dmrpp", __func__ << "() - Parsing complete"<< endl);

        // Check to make sure we have something that smells like our test array
        D4Group *root = dmr->root();
        checkGroupsAndVars(root, "/", 0, 1);
        // Walk the vars and testy testy
        D4Group::Vars_iter vIter = root->var_begin();
        try {
            DmrppArray *var = dynamic_cast<DmrppArray*>(*vIter);

            unsigned int start = 10;
            unsigned int stride = 1; //100
            unsigned int stop = 35010;
            // Constrain the array
            array_length = 1 + (stop - start) / stride;
            BESDEBUG("dmrpp", __func__ << "() - array_length:  " << array_length << endl);
            var->add_constraint(var->dim_begin(), start, stride, stop);

            BESDEBUG("dmrpp", __func__ << "() - dim_start:  " << var->dimension_start(var->dim_begin()) << endl);
            BESDEBUG("dmrpp", __func__ << "() - dim_stride: " << var->dimension_stride(var->dim_begin()) << endl);
            BESDEBUG("dmrpp", __func__ << "() - dim_stop:   " << var->dimension_stop(var->dim_begin()) << endl);
            BESDEBUG("dmrpp", __func__ << "() - dim_size:   " << var->dimension_size(var->dim_begin()) << endl);
            BESDEBUG("dmrpp", __func__ << "() - length: " << var->length() << endl);

            // Read the variable and transfer the data
            read_var_check_name_and_length(var, variable_name, array_length);
            vector<dods_float32> values(var->length());
            var->value(&values[0]);

            // Test data set is incrementally valued: Check Them All!
            for (unsigned long long a_index = 0; a_index < array_length; a_index++) {
                test_float32 = a_index * stride + start;
                if (!double_eq(values[a_index], test_float32))
                BESDEBUG("dmrpp",
                        "values[" << a_index << "]: " << values[a_index] << "  test_float32: " << test_float32 << endl);
                CPPUNIT_ASSERT(double_eq(values[a_index], test_float32));
            }
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
        catch (std::exception &e) {
            CPPUNIT_FAIL(e.what());
        }

        CPPUNIT_ASSERT("Passed");
    }

    /**
     * Here we use a CE which:
     * a) Does not retrieve from all chunks
     * b) Uses a stride of 1
     * Against the chunked oneD test array. The stride==1 means that
     * contiguous blocks may be copied.
     */
    void test_chunked_oneD_CE_01()
    {
        string chnkd_oneD = string(TEST_DATA_DIR).append("/").append("chunked_oneD.h5.dmrpp");

        unsigned long long array_length = 40000;
        string variable_name = "d_4_chunks";
        string filename = chnkd_oneD;
        auto_ptr<DMR> dmr(new DMR);
        DmrppTypeFactory dtf;
        dmr->set_factory(&dtf);
        dods_float32 test_float32;

        BESDEBUG("dmrpp", __func__ << "() - Opening: " << filename << endl);

        ifstream in(filename.c_str());
        parser.intern(in, dmr.get(), debug);
        BESDEBUG("dmrpp", __func__ << "() - Parsing complete"<< endl);

        // Check to make sure we have something that smells like our test array
        D4Group *root = dmr->root();
        checkGroupsAndVars(root, "/", 0, 1);
        // Walk the vars and testy testy
        D4Group::Vars_iter vIter = root->var_begin();
        try {
            DmrppArray *var = dynamic_cast<DmrppArray*>(*vIter);

            unsigned int start = 10;
            unsigned int stride = 1;
            unsigned int stop = 15009;
            // Constrain the array
            array_length = 1 + (stop - start) / stride;
            BESDEBUG("dmrpp", __func__ << "() - array_length:  " << array_length << endl);
            var->add_constraint(var->dim_begin(), start, stride, stop);

            BESDEBUG("dmrpp", __func__ << "() - dim_start:  " << var->dimension_start(var->dim_begin()) << endl);
            BESDEBUG("dmrpp", __func__ << "() - dim_stride: " << var->dimension_stride(var->dim_begin()) << endl);
            BESDEBUG("dmrpp", __func__ << "() - dim_stop:   " << var->dimension_stop(var->dim_begin()) << endl);
            BESDEBUG("dmrpp", __func__ << "() - dim_size:   " << var->dimension_size(var->dim_begin()) << endl);
            BESDEBUG("dmrpp", __func__ << "() - length: " << var->length() << endl);

            // Read the variable and transfer the data
            read_var_check_name_and_length(var, variable_name, array_length);
            vector<dods_float32> values(var->length());
            var->value(&values[0]);

            // Test data set is incrementally valued: Check Them All!
            for (unsigned long long a_index = 0; a_index < array_length; a_index++) {
                test_float32 = a_index + 10;
                if (!double_eq(values[a_index], test_float32))
                BESDEBUG("dmrpp",
                        "values[" << a_index << "]: " << values[a_index] << "  test_float32: " << test_float32 << endl);
                CPPUNIT_ASSERT(double_eq(values[a_index], test_float32));
            }
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
        catch (std::exception &e) {
            CPPUNIT_FAIL(e.what());
        }

        CPPUNIT_ASSERT("Passed");
    }

    void test_read_oneD_chunked_array()
    {
        string chnkd_oneD = string(TEST_DATA_DIR).append("/").append("chunked_oneD.h5.dmrpp");
        check_f32_test_array(chnkd_oneD, "d_4_chunks", 40000);
    }

    void test_read_oneD_uneven_chunked_array()
    {
        string chnkd_oneD = string(TEST_DATA_DIR).append("/").append("chunked_oneD_uneven.h5.dmrpp");
        check_f32_test_array(chnkd_oneD, "d_5_odd_chunks", 40000);
    }

    void test_read_twoD_chunked_array()
    {
        string chnkd_twoD = string(TEST_DATA_DIR).append("/").append("chunked_twoD.h5.dmrpp");
        check_f32_test_array(chnkd_twoD, "d_4_chunks", 10000);
    }

    void test_read_twoD_uneven_chunked_array()
    {
        string chnkd_twoD = string(TEST_DATA_DIR).append("/").append("chunked_twoD_uneven.h5.dmrpp");
        check_f32_test_array(chnkd_twoD, "d_10_odd_chunks", 10000);
    }

    void test_read_twoD_chunked_asymmetric_array()
    {
        string chnkd_twoD_asym = string(TEST_DATA_DIR).append("/").append("chunked_twoD_asymmetric.h5.dmrpp");
        check_f32_test_array(chnkd_twoD_asym, "d_8_chunks", 20000);
    }

    void test_read_threeD_chunked_array()
    {
        string chnkd_threeD = string(TEST_DATA_DIR).append("/").append("chunked_threeD.h5.dmrpp");
        check_f32_test_array(chnkd_threeD, "d_8_chunks", 1000000);
    }

    void test_read_threeD_chunked_asymmetric_array()
    {
        string chnkd_threeD_asym = string(TEST_DATA_DIR).append("/").append("chunked_threeD_asymmetric.h5.dmrpp");
        check_f32_test_array(chnkd_threeD_asym, "d_8_chunks", 1000000);
    }

    void test_read_fourD_chunked_array()
    {
        string chnkd_fourD = string(TEST_DATA_DIR).append("/").append("chunked_fourD.h5.dmrpp");
        check_f32_test_array(chnkd_fourD, "d_16_chunks", 2560000);
    }

    void test_chunked_gzipped_oneD()
    {
        string chnkd_oneD = string(TEST_DATA_DIR).append("/").append("chunked_gzipped_oneD.h5.dmrpp");
        check_f32_test_array(chnkd_oneD, "d_4_gzipped_chunks", 40000);
    }

    void test_chunked_gzipped_twoD()
    {
        string chnkd_twoD = string(TEST_DATA_DIR).append("/").append("chunked_gzipped_twoD.h5.dmrpp");
        check_f32_test_array(chnkd_twoD, "d_4_gzipped_chunks", 100*100);
    }

    void test_chunked_gzipped_threeD()
    {
        string chnkd_threeD = string(TEST_DATA_DIR).append("/").append("chunked_gzipped_threeD.h5.dmrpp");
        check_f32_test_array(chnkd_threeD, "d_8_gzipped_chunks", 100*100*100);
    }
    void test_chunked_gzipped_fourD()
    {
        string chnkd_fourD = string(TEST_DATA_DIR).append("/").append("chunked_gzipped_fourD.h5.dmrpp");
        check_f32_test_array(chnkd_fourD, "d_16_gzipped_chunks", 40*40*40*40);
    }



    void test_chunked_gzipped_oneD_CE_00()
    {
        string chnkd_oneD = test_data_dir + "/chunked_gzipped_oneD.h5.dmrpp";

        unsigned long long array_length = 40000;
        string variable_name = "d_4_gzipped_chunks";
        string filename = chnkd_oneD;
        auto_ptr<DMR> dmr(new DMR);
        DmrppTypeFactory dtf;
        dmr->set_factory(&dtf);
        dods_float32 test_float32;

        BESDEBUG("dmrpp", __func__ << "() - Opening: " << filename << endl);

        ifstream in(filename.c_str());
        parser.intern(in, dmr.get(), debug);
        BESDEBUG("dmrpp", __func__ << "() - Parsing complete"<< endl);

        // Check to make sure we have something that smells like our test array
        D4Group *root = dmr->root();
        checkGroupsAndVars(root, "/", 0, 1);
        // Walk the vars and testy testy
        D4Group::Vars_iter vIter = root->var_begin();
        try {
            DmrppArray *var = dynamic_cast<DmrppArray*>(*vIter);

            unsigned int start = 10;
            unsigned int stride = 1; //100
            unsigned int stop = 35010;
            // Constrain the array
            array_length = 1 + (stop - start) / stride;
            BESDEBUG("dmrpp", __func__ << "() - array_length:  " << array_length << endl);
            var->add_constraint(var->dim_begin(), start, stride, stop);

            BESDEBUG("dmrpp", __func__ << "() - dim_start:  " << var->dimension_start(var->dim_begin()) << endl);
            BESDEBUG("dmrpp", __func__ << "() - dim_stride: " << var->dimension_stride(var->dim_begin()) << endl);
            BESDEBUG("dmrpp", __func__ << "() - dim_stop:   " << var->dimension_stop(var->dim_begin()) << endl);
            BESDEBUG("dmrpp", __func__ << "() - dim_size:   " << var->dimension_size(var->dim_begin()) << endl);
            BESDEBUG("dmrpp", __func__ << "() - length: " << var->length() << endl);

            // Read the variable and transfer the data
            read_var_check_name_and_length(var, variable_name, array_length);
            vector<dods_float32> values(var->length());
            var->value(&values[0]);

            // Test data set is incrementally valued: Check Them All!
            for (unsigned long long a_index = 0; a_index < array_length; a_index++) {
                test_float32 = a_index * stride + start;
                if (!double_eq(values[a_index], test_float32))
                BESDEBUG("dmrpp",
                        "values[" << a_index << "]: " << values[a_index] << "  test_float32: " << test_float32 << endl);
                CPPUNIT_ASSERT(double_eq(values[a_index], test_float32));
            }
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
        catch (std::exception &e) {
            CPPUNIT_FAIL(e.what());
        }

        CPPUNIT_ASSERT("Passed");
    }


    void test_chunked_shuffled_oneD()
    {
        string chnkd_fourD = string(TEST_DATA_DIR).append("/").append("chunked_shuffled_oneD.h5.dmrpp");
        check_f32_test_array(chnkd_fourD, "d_4_shuffled_chunks", 40000);
    }

    void test_chunked_shuffled_twoD()
    {
        string chnkd_fourD = string(TEST_DATA_DIR).append("/").append("chunked_shuffled_twoD.h5.dmrpp");
        check_f32_test_array(chnkd_fourD, "d_4_shuffled_chunks", 10000);
    }
    void test_chunked_shuffled_threeD()
    {
        string chnkd_fourD = string(TEST_DATA_DIR).append("/").append("chunked_shuffled_threeD.h5.dmrpp");
        check_f32_test_array(chnkd_fourD, "d_8_shuffled_chunks", 1000000);
    }
    void test_chunked_shuffled_fourD()
    {
        string chnkd_fourD = string(TEST_DATA_DIR).append("/").append("chunked_shuffled_fourD.h5.dmrpp");
        check_f32_test_array(chnkd_fourD, "d_16_shuffled_chunks", 2560000);
    }

    void test_chunked_shuffled_zipped_oneD()
    {
        string chnkd_fourD = string(TEST_DATA_DIR).append("/").append("chunked_shufzip_oneD.h5.dmrpp");
        check_f32_test_array(chnkd_fourD, "d_4_shufzip_chunks", 40000);
    }

    void test_chunked_shuffled_zipped_twoD()
    {
        string chnkd_fourD = string(TEST_DATA_DIR).append("/").append("chunked_shufzip_twoD.h5.dmrpp");
        check_f32_test_array(chnkd_fourD, "d_4_shufzip_chunks", 10000);
    }
    void test_chunked_shuffled_zipped_threeD()
    {
        string chnkd_fourD = string(TEST_DATA_DIR).append("/").append("chunked_shufzip_threeD.h5.dmrpp");
        check_f32_test_array(chnkd_fourD, "d_8_shufzip_chunks", 1000000);
    }
    void test_chunked_shuffled_zipped_fourD()
    {
        string chnkd_fourD = string(TEST_DATA_DIR).append("/").append("chunked_shufzip_fourD.h5.dmrpp");
        check_f32_test_array(chnkd_fourD, "d_16_shufzip_chunks", 2560000);
    }




    CPPUNIT_TEST_SUITE( DmrppChunkedReadTest );

    CPPUNIT_TEST(test_read_oneD_chunked_array);
#if 1
    CPPUNIT_TEST(test_read_twoD_chunked_array);
    CPPUNIT_TEST(test_read_twoD_chunked_asymmetric_array);
    CPPUNIT_TEST(test_read_threeD_chunked_array);
    CPPUNIT_TEST(test_read_threeD_chunked_asymmetric_array);
    CPPUNIT_TEST(test_read_fourD_chunked_array);
    CPPUNIT_TEST(test_chunked_oneD_CE_00);
    CPPUNIT_TEST(test_chunked_oneD_CE_01);
    CPPUNIT_TEST(test_read_oneD_uneven_chunked_array);
    CPPUNIT_TEST(test_read_twoD_uneven_chunked_array);

    CPPUNIT_TEST(test_chunked_gzipped_oneD);
    CPPUNIT_TEST(test_chunked_gzipped_twoD);
    CPPUNIT_TEST(test_chunked_gzipped_threeD);
    CPPUNIT_TEST(test_chunked_gzipped_fourD);
    CPPUNIT_TEST(test_chunked_gzipped_oneD_CE_00);

    CPPUNIT_TEST(test_chunked_gzipped_oneD_CE_00);
    CPPUNIT_TEST(test_chunked_shuffled_oneD);
    CPPUNIT_TEST(test_chunked_shuffled_twoD);
    CPPUNIT_TEST(test_chunked_shuffled_threeD);
    CPPUNIT_TEST(test_chunked_shuffled_fourD);


    CPPUNIT_TEST(test_chunked_shuffled_zipped_oneD);
    CPPUNIT_TEST(test_chunked_shuffled_zipped_twoD);
    CPPUNIT_TEST(test_chunked_shuffled_zipped_threeD);
    CPPUNIT_TEST(test_chunked_shuffled_zipped_fourD);
#endif
    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(DmrppChunkedReadTest);

} // namespace dmrpp

int main(int argc, char*argv[])
{
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    GetOpt getopt(argc, argv, "d");
    int option_char;
    while ((option_char = getopt()) != -1)
        switch (option_char) {
        case 'd':
            debug = true;  // debug is a static global
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
            test = string("dmrpp::DmrppChunkedReadTest::") + argv[i++];

            cerr << endl << "Running test " << test << endl << endl;

            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}

