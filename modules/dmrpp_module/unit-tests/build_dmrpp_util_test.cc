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

#include <unistd.h>

#include <H5Ppublic.h>
#include <H5Dpublic.h>
#include <H5Epublic.h>
#include <H5Zpublic.h>  // Constants for compression filters
#include <H5Spublic.h>

#include "BESNotFoundError.h"

#include "DMRpp.h"
#include "DmrppTypeFactory.h"

#include "build_dmrpp_util.h"

#include "run_tests_cppunit.h"
#include "test_config.h"

using namespace std;
using namespace dmrpp;
using namespace libdap;

#include <libdap/D4ParserSax2.h>
#include <libdap/D4Group.h>

namespace build_dmrpp_util {

// Functions not listed in the build_dmrpp_util header

bool is_hdf5_fill_value_defined(hid_t dataset_id);

class build_dmrpp_util_test : public CppUnit::TestFixture {
private:
    hid_t fill_value_file; // An HDF5 file for testing
    const string fill_value_file_name {string(TEST_DATA_ROOT_DIR) + "/fill_value/FValue.h5"};
    const string fill_value_dmr_name {string(TEST_DATA_ROOT_DIR) + "/fill_value/FValue.dmr"};

    unique_ptr<dmrpp::DMRpp> dmrpp;
    DmrppTypeFactory dtf;
public:
    // Called once before everything gets tested
    build_dmrpp_util_test() = default;

    // Called at the end of the test
    ~build_dmrpp_util_test() = default;

    // Called before each test
    void setUp() {
        fill_value_file = H5Fopen(fill_value_file_name.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
        if (fill_value_file < 0) {
            throw BESNotFoundError(string("Error: HDF5 file '").append(fill_value_file_name).append("' cannot be opened."), __FILE__, __LINE__);
        }

        dmrpp.reset(new dmrpp::DMRpp);
        dmrpp->set_factory(&dtf);

        ifstream in(fill_value_dmr_name);
        D4ParserSax2 parser;
        parser.intern(in, dmrpp.get(), false);
    }

    // Called after each test
    void tearDown() {
        H5Fclose(fill_value_file);
    }

    void is_hdf5_fill_value_defined_test_1() {
        for (auto v = dmrpp->root()->var_begin(), ve = dmrpp->root()->var_end(); v != ve; ++v) {
            string fqn = (*v)->FQN();
            BESDEBUG("dmrpp", "Working on: " << fqn << endl);
            hid_t dataset = H5Dopen2(fill_value_file, fqn.c_str(), H5P_DEFAULT);

            CPPUNIT_ASSERT_MESSAGE("Could not open hdf5 dataset", dataset != -1);
        }
    }

    CPPUNIT_TEST_SUITE(build_dmrpp_util_test);

    CPPUNIT_TEST(is_hdf5_fill_value_defined_test_1);

    CPPUNIT_TEST_SUITE_END();

};

CPPUNIT_TEST_SUITE_REGISTRATION(build_dmrpp_util_test);

} // namespace dmrpp

int main(int argc, char *argv[]) {
    return bes_run_tests<build_dmrpp_util::build_dmrpp_util_test>(argc, argv, "cerr,dmrpp") ? 0: 1;
}
