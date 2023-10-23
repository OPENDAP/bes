// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2018 OPeNDAP, Inc.
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

#include "config.h"

#include <string>
#include <vector>
#include <sstream>

#include <unistd.h>

#include <H5Ppublic.h>
#include <H5Dpublic.h>
#include <H5Epublic.h>
#include <H5Zpublic.h>  // Constants for compression filters
#include <H5Spublic.h>

#include "BESInternalError.h"
#include "BESInternalFatalError.h"
#include "BESNotFoundError.h"

#include "DMRpp.h"
#include "DmrppTypeFactory.h"

#include "build_dmrpp_util.h"

#include "modules/common/run_tests_cppunit.h"
#include "test_config.h"

using namespace std;
using namespace dmrpp;
using namespace libdap;

#include <libdap/D4ParserSax2.h>
#include <libdap/D4Group.h>

namespace build_dmrpp_util {

// Functions not listed in the build_dmrpp_util header

bool is_hdf5_fill_value_defined(hid_t dataset_id);
string get_value_as_string(hid_t h5_type_id, vector<char> &value);
string get_hdf5_fill_value_str(hid_t dataset_id);

class build_dmrpp_util_test : public CppUnit::TestFixture {
private:
    DmrppTypeFactory dtf;

    hid_t fill_value_file = -1; // An HDF5 file for testing
    const string fill_value_file_name {string(TEST_DATA_ROOT_DIR) + "/fill_value/FValue.h5"};
    const string fill_value_dmr_name {string(TEST_DATA_ROOT_DIR) + "/fill_value/FValue.h5.dmr"};
    unique_ptr<dmrpp::DMRpp> fv_dmrpp;

    hid_t fill_value_chunks_file = -1; // An HDF5 file for testing
    const string fill_value_chunks_file_name {string(TEST_DATA_ROOT_DIR) + "/fill_value/FValue_chunk.h5"};
    const string fill_value_chunks_dmr_name {string(TEST_DATA_ROOT_DIR) + "/fill_value/FValue_chunk.h5.dmr"};
    unique_ptr<dmrpp::DMRpp> fv_chunks_dmrpp;

public:
    // Called once before everything gets tested
    build_dmrpp_util_test() = default;

    // Called at the end of the test
    ~build_dmrpp_util_test() override = default;

    static void setup_data(hid_t &file, DMRpp *dmrpp, const string &file_name, const string &dmr_name) {
        file = H5Fopen(file_name.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
        if (file < 0) {
            throw BESNotFoundError(string("Error: HDF5 file '").append(file_name).append("' cannot be opened."), __FILE__, __LINE__);
        }

        ifstream in(dmr_name);
        D4ParserSax2 parser;
        parser.intern(in, dmrpp, false);

    }
    // Called before each test
    void setUp() override {
        fv_dmrpp.reset(new dmrpp::DMRpp);
        fv_dmrpp->set_factory(&dtf);
        setup_data(fill_value_file, fv_dmrpp.get(), fill_value_file_name, fill_value_dmr_name);

        fv_chunks_dmrpp.reset(new dmrpp::DMRpp);
        fv_chunks_dmrpp->set_factory(&dtf);
        setup_data(fill_value_chunks_file, fv_chunks_dmrpp.get(), fill_value_chunks_file_name, fill_value_chunks_dmr_name);
    }

    // Called after each test
    void tearDown() override {
        H5Fclose(fill_value_file);
        H5Fclose(fill_value_chunks_file);
    }

#if 0
    // Files made by Kent and the variables they hold.

    FValue.h5: /chunks_all_fill
    FValue.h5: /chunks_fill_not_write
    FValue.h5: /chunks_fill_notdefined
    FValue.h5: /chunks_some_fill
    FValue.h5: /compact_all_fill
    FValue.h5: /cont_all_fill
    FValue.h5: /cont_some_fill

    FValue_chunk.h5: /chunks_all_fill
#endif

    // Test that we can open the files and find all the vars in the DMR files.
    void file_and_dmr_test() {
        for (auto v: fv_dmrpp->root()->variables()) {
            string fqn = v->FQN();
            BESDEBUG("dmrpp", "Working on: " << fqn << endl);
            hid_t dataset = H5Dopen2(fill_value_file, fqn.c_str(), H5P_DEFAULT);

            CPPUNIT_ASSERT_MESSAGE("Could not open hdf5 dataset", dataset != -1);
        }

        for (auto v: fv_chunks_dmrpp->root()->variables()) {
            string fqn = v->FQN();
            BESDEBUG("dmrpp", "Working on: " << fqn << endl);
            hid_t dataset = H5Dopen2(fill_value_chunks_file, fqn.c_str(), H5P_DEFAULT);

            CPPUNIT_ASSERT_MESSAGE("Could not open hdf5 dataset", dataset != -1);
        }
    }

    void test_input_file_signature_no_file() {
        CPPUNIT_ASSERT_THROW_MESSAGE("No such file expected.", qc_input_file("/foofoo"), BESInternalFatalError);
    }

    void test_input_file_signature_netcdf3_file() {
        CPPUNIT_ASSERT_THROW_MESSAGE("netcdf3 file expected.", qc_input_file("input-files/testfile.nc"), BESInternalFatalError);
    }

    void test_input_file_signature_hdf_file() {
        CPPUNIT_ASSERT_NO_THROW(qc_input_file(string(TEST_DATA_ROOT_DIR) + "s3_data/AIRS_partial.h5"));
    }

    void is_hdf5_fill_value_defined_test_bad_dataset_id() {
        CPPUNIT_ASSERT_THROW_MESSAGE("This should throw BESInternalError", is_hdf5_fill_value_defined(-1), BESInternalError);
    }

    void is_hdf5_fill_value_defined_test_chunks_all_fill() {
        hid_t dataset = H5Dopen2(fill_value_file, "/chunks_all_fill", H5P_DEFAULT);
        CPPUNIT_ASSERT_MESSAGE("All chunks have fill value defined", is_hdf5_fill_value_defined(dataset));
    }

    void is_hdf5_fill_value_defined_test_chunks_fill_not_write() {
        hid_t dataset = H5Dopen2(fill_value_file, "/chunks_fill_not_write", H5P_DEFAULT);
        CPPUNIT_ASSERT_MESSAGE("All chunks have fill value defined", is_hdf5_fill_value_defined(dataset));
    }

    void is_hdf5_fill_value_defined_test_chunks_fill_notdefined() {
        hid_t dataset = H5Dopen2(fill_value_file, "/chunks_fill_notdefined", H5P_DEFAULT);
        CPPUNIT_ASSERT_MESSAGE("No fill value defined", !is_hdf5_fill_value_defined(dataset));
    }

    void is_hdf5_fill_value_defined_test_chunks_some_fill() {
        hid_t dataset = H5Dopen2(fill_value_file, "/chunks_some_fill", H5P_DEFAULT);
        CPPUNIT_ASSERT_MESSAGE("Chunks, some fill values defined", is_hdf5_fill_value_defined(dataset));
    }

    void is_hdf5_fill_value_defined_test_cont_some_fill() {
        hid_t dataset = H5Dopen2(fill_value_file, "/cont_some_fill", H5P_DEFAULT);
        CPPUNIT_ASSERT_MESSAGE("Contiguous, some fill values defined", is_hdf5_fill_value_defined(dataset));
    }

    void is_hdf5_fill_value_defined_test_compact_all_fill() {
        hid_t dataset = H5Dopen2(fill_value_file, "/compact_all_fill", H5P_DEFAULT);
        CPPUNIT_ASSERT_MESSAGE("Compact, all fill values defined", is_hdf5_fill_value_defined(dataset));
    }

    template <typename T> void get_value_test_helper(T val, const string &expected, hid_t type_id, const string &test_name) {
        vector<char> value(sizeof(T));
        memcpy(value.data(), &val, sizeof(T));

        string str_value = get_value_as_string(type_id, value);
        ostringstream oss;
        oss << test_name << ": Expected " << val << ", but got " << str_value;
        DBG(cerr << oss.str() << endl);

        CPPUNIT_ASSERT_MESSAGE(oss.str(), str_value.find(expected) != string::npos);
    }

    void get_value_as_string_test_char() {
        int8_t v = 1;
        get_value_test_helper<int8_t>(v, "1", H5T_NATIVE_INT8_g, __func__);
    }

    void get_value_as_string_test_short() {
        get_value_test_helper<int16_t>(1024, "1024", H5T_NATIVE_INT16_g, __func__);
    }

    void get_value_as_string_test_short_2() {
        get_value_test_helper<int16_t>(-1024, "-1024", H5T_NATIVE_INT16_g, __func__);
    }

    void get_value_as_string_test_ushort() {
        get_value_test_helper<uint16_t>(65000, "65000", H5T_NATIVE_UINT16_g, __func__);
    }

    void get_value_as_string_test_int() {
        get_value_test_helper<int32_t>(70000, "70000", H5T_NATIVE_INT32_g, __func__);
    }

    void get_value_as_string_test_int_2() {
        get_value_test_helper<int32_t>(-70000, "-70000", H5T_NATIVE_INT32_g, __func__);
    }

    void get_value_as_string_test_uint() {
        get_value_test_helper<uint32_t>(70001, "70001", H5T_NATIVE_UINT32_g, __func__);
    }

    void get_value_as_string_test_float() {
        get_value_test_helper<float>(70001.2, "70001.2", H5T_NATIVE_FLOAT_g, __func__);
    }

    void get_value_as_string_test_double() {
        get_value_test_helper<double>(70.002, "70.002", H5T_NATIVE_DOUBLE_g, __func__);
    }

    static string get_fill_value_test_helper(hid_t h5_file, const string &dataset_name, const string &function_name) {
        hid_t dataset = H5Dopen2(h5_file, dataset_name.c_str(), H5P_DEFAULT);
        string str_value = get_hdf5_fill_value_str(dataset);
        DBG(cerr << function_name << " fill value: " << str_value << endl);
        return str_value;
    }

    void get_hdf5_fill_value_test_chunks_all_fill() {
        CPPUNIT_ASSERT_MESSAGE(string(__func__).append(": Expected -99"),
                               get_fill_value_test_helper(fill_value_file, "/chunks_all_fill", __func__) == "-99");
    }

    void get_hdf5_fill_value_test_chunks_fill_not_write() {
        CPPUNIT_ASSERT_MESSAGE(string(__func__).append(": Expected -99"),
                               get_fill_value_test_helper(fill_value_file, "/chunks_fill_not_write", __func__) == "-99");
    }

    void get_hdf5_fill_value_test_chunks_fill_notdefined() {
        CPPUNIT_ASSERT_THROW_MESSAGE(string(__func__).append(": Expected -99"),
                               get_fill_value_test_helper(fill_value_file, "/chunks_fill_notdefined", __func__), BESInternalError);
    }

    void get_hdf5_fill_value_test_chunks_some_fill() {
        CPPUNIT_ASSERT_MESSAGE(string(__func__).append(": Expected -99"),
                               get_fill_value_test_helper(fill_value_file, "/chunks_some_fill", __func__) == "-99");
    }

    void get_hdf5_fill_value_test_compact_all_fill() {
        CPPUNIT_ASSERT_MESSAGE(string(__func__).append(": Expected -99"),
                               get_fill_value_test_helper(fill_value_file, "/compact_all_fill", __func__) == "-99");
    }

    void get_hdf5_fill_value_test_cont_all_fill() {
        CPPUNIT_ASSERT_MESSAGE(string(__func__).append(": Expected -99"),
                               get_fill_value_test_helper(fill_value_file, "/cont_all_fill", __func__) == "-99");
    }
    
    void get_hdf5_fill_value_test_cont_some_fill() {
        CPPUNIT_ASSERT_MESSAGE(string(__func__).append(": Expected -99"),
                               get_fill_value_test_helper(fill_value_file, "/cont_some_fill", __func__) == "-99");
    }

    void get_hdf5_fill_value_test_chunks_all_fill_2() {
        CPPUNIT_ASSERT_MESSAGE(string(__func__).append(": Expected -99"),
                               get_fill_value_test_helper(fill_value_chunks_file, "/chunks_all_fill", __func__) == "-99");
    }
    void vector_init_test() {

        vector<string> t1 = {""};
        cout << "# t1.size() " << t1.size() << endl;
        CPPUNIT_ASSERT(t1.size()==1);

        for(auto s:t1){
            cout << "#   t1[]: '" << s << "'" << endl;
        }

        vector<string> t2;
        t2.push_back("");
        cout << "# t2.size() " << t2.size() << endl;
        CPPUNIT_ASSERT(t2.size()==1);
        for(auto s:t2){
            cout << "#   t2[]: '" << s << "'" << endl;
        }

    }

    CPPUNIT_TEST_SUITE(build_dmrpp_util_test);

        CPPUNIT_TEST(vector_init_test);
        CPPUNIT_TEST(file_and_dmr_test);

        CPPUNIT_TEST(test_input_file_signature_no_file);
        CPPUNIT_TEST(test_input_file_signature_netcdf3_file);
        CPPUNIT_TEST(test_input_file_signature_hdf_file);

        CPPUNIT_TEST(is_hdf5_fill_value_defined_test_bad_dataset_id);

        CPPUNIT_TEST(is_hdf5_fill_value_defined_test_chunks_all_fill);
        CPPUNIT_TEST(is_hdf5_fill_value_defined_test_chunks_fill_not_write);
        CPPUNIT_TEST(is_hdf5_fill_value_defined_test_chunks_fill_notdefined);
        CPPUNIT_TEST(is_hdf5_fill_value_defined_test_chunks_some_fill);
        CPPUNIT_TEST(is_hdf5_fill_value_defined_test_cont_some_fill);
        CPPUNIT_TEST(is_hdf5_fill_value_defined_test_compact_all_fill);

        CPPUNIT_TEST(get_value_as_string_test_char);

        CPPUNIT_TEST(get_value_as_string_test_short);
        CPPUNIT_TEST(get_value_as_string_test_short_2);
        CPPUNIT_TEST(get_value_as_string_test_ushort);
        CPPUNIT_TEST(get_value_as_string_test_int);
        CPPUNIT_TEST(get_value_as_string_test_int_2);
        CPPUNIT_TEST(get_value_as_string_test_uint);
        CPPUNIT_TEST(get_value_as_string_test_float);
        CPPUNIT_TEST(get_value_as_string_test_double);

        CPPUNIT_TEST(get_hdf5_fill_value_test_chunks_all_fill);
        CPPUNIT_TEST(get_hdf5_fill_value_test_chunks_fill_not_write);
        CPPUNIT_TEST(get_hdf5_fill_value_test_chunks_fill_notdefined);
        CPPUNIT_TEST(get_hdf5_fill_value_test_chunks_some_fill);
        CPPUNIT_TEST(get_hdf5_fill_value_test_compact_all_fill);
        CPPUNIT_TEST(get_hdf5_fill_value_test_cont_all_fill);
        CPPUNIT_TEST(get_hdf5_fill_value_test_cont_some_fill);
        CPPUNIT_TEST(get_hdf5_fill_value_test_chunks_all_fill_2);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(build_dmrpp_util_test);

} // namespace dmrpp

int main(int argc, char *argv[]) {
    return bes_run_tests<build_dmrpp_util::build_dmrpp_util_test>(argc, argv, "cerr,dmrpp") ? 0: 1;
}
