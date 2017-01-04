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

class DmrppTypeReadTest: public CppUnit::TestFixture {
private:
    DmrppParserSax2 parser;

public:
    // Called once before everything gets tested
    DmrppTypeReadTest() :
        parser()
    {
    }

    // Called at the end of the test
    ~DmrppTypeReadTest()
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
    void set_data_url_in_chunks(DmrppCommon *dc){
        // Get the chunks and make sure there's at least one
        vector<H4ByteStream> *chunks = dc->get_chunk_vec();
        CPPUNIT_ASSERT((*chunks).size() > 0);
        // Tweak the data URLs for the test
        for(unsigned int i=0; i<(*chunks).size() ;i++){
            BESDEBUG("dmrpp", "chunk_refs[" << i << "]: " << (*chunks)[i].to_string() << endl);
        }
        for(unsigned int i=0; i<(*chunks).size() ;i++){
            string data_url = string("file://").append(TEST_DATA_DIR).append((*chunks)[i].get_data_url());
            (*chunks)[i].set_data_url(data_url);
        }
        for(unsigned int i=0; i<(*chunks).size() ;i++){
            BESDEBUG("dmrpp", "altered chunk_refs[" << i << "]: " << (*chunks)[i].to_string() << endl);
        }
    }
    /**
     * Checks name, reads data, checks # of read bytes.
     */
    void read_var_check_name_and_length(DmrppArray *array, string name, int length){
        BESDEBUG("dmrpp", "array->name(): " << array->name() << endl);
        CPPUNIT_ASSERT(array->name() == name);

        set_data_url_in_chunks(array);

        // Read the data
        array->read();
        BESDEBUG("dmrpp", "array->length(): " << array->length() << endl);
        CPPUNIT_ASSERT(array->length() == length);
    }


    /**
     * Since we have a lot of test data files that contain a single array here
     * is a complete test of reading the array and verifying its content.
     */
    void check_f32_test_array(string filename, string variable_name, unsigned long long array_length){
        auto_ptr<DMR> dmr(new DMR);
        DmrppTypeFactory dtf;
        dmr->set_factory(&dtf);
        dods_float32 test_float32;

        BESDEBUG("dmrpp", "Opening: " << filename << endl);

        ifstream in(filename.c_str());
        parser.intern(in, dmr.get(), debug);
        BESDEBUG("dmrpp", "Parsing complete"<< endl);

        // Check to make sure we have something that smells like our test array
        D4Group *root = dmr->root();
        checkGroupsAndVars(root, "/", 0, 1);
        // Walk the vars and testy testy
        D4Group::Vars_iter vIter = root->var_begin();
        try {
            // Read the variable and transfer the data
        	DmrppArray *var = dynamic_cast<DmrppArray*>(*vIter);
            read_var_check_name_and_length(var,variable_name,array_length);
            vector<dods_float32> values(var->length());
            var->value(&values[0]);

            // Test data set is incrementally valued: Check Them All!
            for(unsigned long long a_index=0; a_index < array_length ;a_index++){
                test_float32 = a_index;
                BESDEBUG("dmrpp", "values[" << a_index << "]: " << values[a_index] << "  test_float32: " << test_float32 << endl);
                CPPUNIT_ASSERT(double_eq(values[a_index], test_float32 ));
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

    void test_read_oneD_chunked_array() {
        string chnkd_oneD = string(TEST_DATA_DIR).append("/").append("chunked_oneD.h5.dmrpp");
        check_f32_test_array(chnkd_oneD, "d_4_chunks", 40000);
    }

    void test_read_twoD_chunked_array() {
        string chnkd_twoD = string(TEST_DATA_DIR).append("/").append("chunked_twoD.h5.dmrpp");
        check_f32_test_array(chnkd_twoD, "d_4_chunks", 10000);
    }

    void test_read_twoD_chunked_asymmetric_array() {
    	string chnkd_twoD_asym = string(TEST_DATA_DIR).append("/").append("chunked_twoD_asymmetric.h5.dmrpp");
        check_f32_test_array(chnkd_twoD_asym, "d_8_chunks", 20000);
    }

    void test_read_threeD_chunked_array() {
        string chnkd_threeD = string(TEST_DATA_DIR).append("/").append("chunked_threeD.h5.dmrpp");
        check_f32_test_array(chnkd_threeD, "d_8_chunks", 1000000);
    }

    void test_read_threeD_chunked_asymmetric_array() {
        string chnkd_threeD_asym = string(TEST_DATA_DIR).append("/").append("chunked_threeD_asymmetric.h5.dmrpp");
        check_f32_test_array(chnkd_threeD_asym, "d_8_chunks", 1000000);
    }

    void test_read_fourD_chunked_array() {
        string chnkd_fourD = string(TEST_DATA_DIR).append("/").append("chunked_fourD.h5.dmrpp");
        check_f32_test_array(chnkd_fourD, "d_16_chunks", 2560000);
    }



    CPPUNIT_TEST_SUITE( DmrppTypeReadTest );

    CPPUNIT_TEST(test_read_oneD_chunked_array);
    CPPUNIT_TEST(test_read_twoD_chunked_array);
    CPPUNIT_TEST(test_read_twoD_chunked_asymmetric_array);
    CPPUNIT_TEST(test_read_threeD_chunked_array);
    CPPUNIT_TEST(test_read_threeD_chunked_asymmetric_array);
    CPPUNIT_TEST(test_read_fourD_chunked_array);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(DmrppTypeReadTest);

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
            test = string("dmrpp::DmrppTypeReadTest::") + argv[i++];

            cerr << endl << "Running test " << test << endl << endl;

            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}

