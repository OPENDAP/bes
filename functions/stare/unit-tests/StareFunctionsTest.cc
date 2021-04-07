// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2019 OPeNDAP, Inc.
// Authors: Kodi Neumiller <kneumiller@opendap.org>
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

#include <unistd.h>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <GetOpt.h>
#include <BaseType.h>
#include <Float32.h>
#include <Array.h>
#include <Byte.h>
#include <Int32.h>
#include <UInt64.h>
#include <Structure.h>
#include <D4Group.h>
#include <D4RValue.h>
#include <DMR.h>
#include <test/D4TestTypeFactory.h>
#include <test/TestByte.h>
#include <test/TestArray.h>

#include <util.h>
#include <debug.h>

#include <TheBESKeys.h>
#include <BESError.h>
#include <BESDebug.h>

#include "StareFunctions.h"

#include "test_config.h"

using namespace CppUnit;
using namespace libdap;
using namespace std;
using namespace functions;

int test_variable_sleep_interval = 0;

static bool debug = false;
#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

static bool bes_debug = false;

class StareFunctionsTest: public TestFixture {
private:
	DMR *two_arrays_dmr;
	D4BaseTypeFactory *d4_btf;
public:
	StareFunctionsTest() : two_arrays_dmr(0), d4_btf(0)
	{
        TheBESKeys::ConfigFile = "bes.conf";
        // The key names and module variables used here are defined in StareFunctions.cc
        // These two lines duplicate DapFunctions Module behavior. jhrg 5/21/20
        stare_storage_path = TheBESKeys::TheKeys()->read_string_key(STARE_STORAGE_PATH_KEY, stare_storage_path);
        stare_sidecar_suffix = TheBESKeys::TheKeys()->read_string_key(STARE_SIDECAR_SUFFIX_KEY, stare_sidecar_suffix);

        if (bes_debug) BESDebug::SetUp("cerr,stare");
    }

	virtual ~StareFunctionsTest()
	{
	}

	virtual void setUp() {
        d4_btf = new D4BaseTypeFactory();

        two_arrays_dmr = new DMR(d4_btf);
        two_arrays_dmr->set_name("test_dmr");

		string filename = string(TOP_SRC_DIR) + "/functions/stare/data/MYD09.A2019003_hacked.h5";
		// Old file name: "/MYD09.A2019003.2040.006.2019005020913.h5";

		two_arrays_dmr->set_filename(filename);
	}

	virtual void tearDown() {
		delete two_arrays_dmr;
		two_arrays_dmr = 0;
		delete d4_btf;
		d4_btf = 0;
	}

	CPPUNIT_TEST_SUITE( StareFunctionsTest );

	// Deprecated test - breaks distcheck CPPUNIT_TEST(test_get_sidecar_file_pathname);
	// jhrg 1.14.20
    CPPUNIT_TEST(test_has_value);
    CPPUNIT_TEST(test_has_value_2);
    CPPUNIT_TEST(test_has_value_3);
    CPPUNIT_TEST(test_count_1);
    CPPUNIT_TEST(test_count_2);
    CPPUNIT_TEST(test_count_3);
    CPPUNIT_TEST(test_stare_subset);
    CPPUNIT_TEST(test_stare_get_sidecar_uint64_values_2);

    CPPUNIT_TEST(intersection_function_test);
    CPPUNIT_TEST(count_function_test);
    CPPUNIT_TEST(subset_function_test);

    CPPUNIT_TEST(test_stare_subset_array_helper);

	CPPUNIT_TEST_SUITE_END();

	// Deprecated
	void test_get_sidecar_file_pathname() {
        DBG(cerr << "--- test_get_sidecar_file_pathname() test - BEGIN ---" << endl);

        string sidecar_pathname = get_sidecar_file_pathname("/data/sub_dir/bogus.h5");
        string expected_pathname = string(TOP_SRC_DIR) + "/functions/stare/data/bogus_sidecar.h5";

        DBG(cerr << "expected_pathname: " << expected_pathname << endl);
        DBG(cerr << "sidecar_pathname: " << sidecar_pathname << endl);

        // These tests fail with distcheck because autoconf/make borks the paths.
        // jhrg 12/31/19
        CPPUNIT_ASSERT(sidecar_pathname == expected_pathname);

        sidecar_pathname = get_sidecar_file_pathname("/data/different_extension.hdf5");
        expected_pathname = string(TOP_SRC_DIR) + "/functions/stare/data/different_extension_sidecar.hdf5";

        DBG(cerr << "expected_pathname: " << expected_pathname << endl);
        DBG(cerr << "sidecar_pathname: " << sidecar_pathname << endl);

        CPPUNIT_ASSERT(sidecar_pathname == expected_pathname
                       || sidecar_pathname.find("_build/..") != string::npos);
	}

	void test_stare_subset_array_helper() {
        DBG(cerr << "--- test_stare_subset_array_helper() test - BEGIN ---" << endl);

        vector<dods_uint64> target_indices = {3440016191299518474, 3440016191299518400, 3440016191299518401};
        // In these data indices, 3440012343008821258 overlaps 3440016191299518400, 3440016191299518401
        // and 3440016191299518474 overlaps 3440016191299518474, 3440016191299518400, 3440016191299518401
        // I think this is kind of a degenerate example since the three target indices seem to be at different
        // levels. jhrg 1.14.20
        vector<dods_uint64> data_indices = {9223372034707292159, 3440012343008821258, 3440016191299518474};

        vector<dods_int16> src_data{100, 200, 300};
        vector<dods_int16> result_data{0, 0, 0};    // result_data is initialized to the mask value

        stare_subset_array_helper(result_data, src_data, target_indices, data_indices);


        CPPUNIT_ASSERT(result_data[0] == 0);
        CPPUNIT_ASSERT(result_data[1] == 200);
        CPPUNIT_ASSERT(result_data[2] == 300);
	}

    void test_stare_subset() {
        DBG(cerr << "--- test_stare_subset() test - BEGIN ---" << endl);

        vector<dods_uint64> target_indices = {3440016191299518474, 3440016191299518400, 3440016191299518401};
        // In these data indices, 3440012343008821258 overlaps 3440016191299518400, 3440016191299518401
        // and 3440016191299518474 overlaps 3440016191299518474, 3440016191299518400, 3440016191299518401
        // I think this is kind of a degenerate example since the three target indices seem to be at different
        // levels. jhrg 1.14.20
        vector<dods_uint64> data_indices = {9223372034707292159, 3440012343008821258, 3440016191299518474};
        vector<int> x_indices = {0, 1, 2};
        vector<int> y_indices = {0, 1, 2};

        unique_ptr<stare_matches> result = stare_subset_helper(target_indices, data_indices, x_indices, y_indices);

        DBG(cerr << "result->x_indices.size(): " << result->x_indices.size() << endl);


        CPPUNIT_ASSERT(result->x_indices.size() == 5);
        CPPUNIT_ASSERT(result->y_indices.size() == 5);
        CPPUNIT_ASSERT(result->stare_indices.size() == 5);

        DBG(cerr << *result << endl);

        CPPUNIT_ASSERT(result->stare_indices.at(0) == 3440012343008821258);
        CPPUNIT_ASSERT(result->x_indices.at(0) == 1);
        CPPUNIT_ASSERT(result->y_indices.at(0) == 1);

        CPPUNIT_ASSERT(result->stare_indices.at(2) == 3440016191299518474);
        CPPUNIT_ASSERT(result->x_indices.at(2) == 2);
        CPPUNIT_ASSERT(result->y_indices.at(2) == 2);
    }

    void test_stare_get_sidecar_uint64_values_2() {
        DBG(cerr << "--- test_stare_get_sidecar_uint64_values_2() test - BEGIN ---" << endl);

	try
	{
	    const string filename_1 = string(TOP_SRC_DIR) + "/functions/stare/data/t1_sidecar.nc";
	    Float32 *variable = new Float32("Solar_Zenith");
	    vector<dods_uint64> values;
	    
	    // Call our function.
	    get_sidecar_uint64_values_2(filename_1, variable, values);

	    // Check the results.
	    if (values.size() != 406 * 270)
		CPPUNIT_FAIL("test_stare_get_sidecar_uint64_values_2() test failed bad size");
	    if (values.at(0) != 3461703427396677225)
		CPPUNIT_FAIL("test_stare_get_sidecar_uint64_values_2() test failed bad value");
	}
	catch(BESError &e) {
            DBG(cerr << e.get_verbose_message() << endl);
	    cout << e.get_verbose_message() << endl;
            CPPUNIT_FAIL("test_stare_get_sidecar_uint64_values_2() test failed" + e.get_verbose_message());
        }
    }

    // The one and only target index is in the 'dataset'
	void test_count_1() {
        DBG(cerr << "--- test_count_1() test - BEGIN ---" << endl);

        vector<dods_uint64> target_indices = {3440016191299518474};
        vector<dods_uint64> data_indices = {9223372034707292159, 3440012343008821258, 3440016191299518474};

        CPPUNIT_ASSERT(count(target_indices, data_indices) == 1);
	}

	// Of the four target_indices, two are in the 'dataset' and two are not
    void test_count_2() {
        DBG(cerr << "--- test_count_2() test - BEGIN ---" << endl);

        vector<dods_uint64> target_indices = {3440016191299518474, 5440016191299518475, 3440016191299518400, 3440016191299518401};
        vector<dods_uint64> data_indices = {9223372034707292159, 3440012343008821258, 3440016191299518474};

        DBG(cerr << "test_count_2, count(target, dataset): " << count(target_indices, data_indices) << endl);

        CPPUNIT_ASSERT(count(target_indices, data_indices) == 2);
    }

    // Of the two target_indices, none are in the 'dataset.'
    void test_count_3() {
        DBG(cerr << "--- test_count_3() test - BEGIN ---" << endl);

        vector<dods_uint64> target_indices = {3440016191299518400, 3440016191299518401};
        vector<dods_uint64> data_indices = {9223372034707292159, 3440012343008821258, 3440016191299518474};

        DBG(cerr << "test_count_3, count(target, dataset): " << count(target_indices, data_indices) << endl);

        CPPUNIT_ASSERT(count(target_indices, data_indices) == 2);
    }

    // target in the 'dataset.'
    void test_has_value() {
        DBG(cerr << "--- test_has_value() test - BEGIN ---" << endl);

        vector<dods_uint64> target_indices = {3440016191299518474};
        vector<dods_uint64> data_indices = {9223372034707292159, 3440012343008821258, 3440016191299518474};

        CPPUNIT_ASSERT(target_in_dataset(target_indices, data_indices));
    }

    // target not in the 'dataset.'
    void test_has_value_2() {
        DBG(cerr << "--- test_has_value_2() test - BEGIN ---" << endl);

        vector<dods_uint64> target_indices = {5440016191299518475};// {3440016191299518500};
        vector<dods_uint64> data_indices = {9223372034707292159, 3440012343008821258, 3440016191299518474};

        CPPUNIT_ASSERT(!target_in_dataset(target_indices, data_indices));
    }

    // Second target in the 'dataset.'
    void test_has_value_3() {
        DBG(cerr << "--- test_has_value_3() test - BEGIN ---" << endl);

        vector<dods_uint64> target_indices = {3440016191299518500, 3440016191299518474};
        vector<dods_uint64> data_indices = {9223372034707292159, 3440012343008821258, 3440016191299518474};

        CPPUNIT_ASSERT(target_in_dataset(target_indices, data_indices));
    }

    void intersection_function_test() {
		DBG(cerr << "--- intersection_function_test() test - BEGIN ---" << endl);

        try {
            // 'a_var' is a dependent variable in the dataset.
            Array *a_var = new TestArray("a_var", new TestByte("a_var"));
            a_var->append_dim(10);

            two_arrays_dmr->root()->add_var_nocopy(a_var);

            //MYD09.A2019003.2040.006.2019005020913_sidecar.h5 values:
            //Lat - 32.2739, 32.2736, 32.2733, 32.2731, 32.2728, 32.2725, 32.2723, 32.272, 32.2718, 32.2715
            //Lon - -98.8324, -98.8388, -98.8452, -98.8516, -98.858, -98.8644, -98.8708, -98.8772, -98.8836, -98.8899
            //Stare - 3440016191299518474 x 10

            //The first index is an actual stare value from: MYD09.A2019003.2040.006.2019005020913_sidecar.h5
            //The final value is made up.
            vector<dods_uint64> target_indices = {3440016721727979534, 3440012343008821258, 3440016322296021006};

			D4RValueList params;
            params.add_rvalue(new D4RValue(a_var));
			params.add_rvalue(new D4RValue(target_indices));

			BaseType *checkHasValue = StareIntersectionFunction::stare_intersection_dap4_function(&params, *two_arrays_dmr);

			CPPUNIT_ASSERT(dynamic_cast<Int32*> (checkHasValue)->value() == 1);
		}
		catch(Error &e) {
			DBG(cerr << e.get_error_message() << endl);
			CPPUNIT_FAIL("intersection_function_test() test failed");
		}
        catch(BESError &e) {
            DBG(cerr << e.get_verbose_message() << endl);
            CPPUNIT_FAIL("intersection_function_test() test failed");
        }
	}

    void count_function_test() {
        DBG(cerr << "--- count_function_test() test - BEGIN ---" << endl);

        try {
            Array *a_var = new TestArray("a_var", new TestByte("a_var"));
             a_var->append_dim(10);

            two_arrays_dmr->root()->add_var_nocopy(a_var);

            //MYD09.A2019003.2040.006.2019005020913_sidecar.h5 values:
            //Lat - 32.2739, 32.2736, 32.2733, 32.2731, 32.2728, 32.2725, 32.2723, 32.272, 32.2718, 32.2715
            //Lon - -98.8324, -98.8388, -98.8452, -98.8516, -98.858, -98.8644, -98.8708, -98.8772, -98.8836, -98.8899
            //Stare - 3440016191299518474 x 10

            //Array a_var - uint64 for stare indices
            //The first index is an actual stare value from: MYD09.A2019003.2040.006.2019005020913_stare.h5
            //The final value is made up.
            vector<dods_uint64> target_indices = {3440016721727979534, 3440012343008821258, 3440016322296021006};

            D4RValueList params;
            params.add_rvalue(new D4RValue(a_var));
            params.add_rvalue(new D4RValue(target_indices));

            BaseType *checkHasValue = StareCountFunction::stare_count_dap4_function(&params, *two_arrays_dmr);

            CPPUNIT_ASSERT(dynamic_cast<Int32*> (checkHasValue)->value() == 3);
        }
        catch(Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_FAIL("count_function_test() test failed");
        }
        catch(BESError &e) {
            DBG(cerr << e.get_verbose_message() << endl);
            CPPUNIT_FAIL("count_function_test() test failed");
        }
    }

    void subset_function_test() {
        DBG(cerr << "--- subset_function_test() test - BEGIN ---" << endl);

        try {
            Array *a_var = new TestArray("a_var", new TestByte("a_var"));
            a_var->append_dim(10);

            two_arrays_dmr->root()->add_var_nocopy(a_var);

            //MYD09.A2019003.2040.006.2019005020913_sidecar.h5 values:
            //Lat - 32.2739, 32.2736, 32.2733, 32.2731, 32.2728, 32.2725, 32.2723, 32.272, 32.2718, 32.2715
            //Lon - -98.8324, -98.8388, -98.8452, -98.8516, -98.858, -98.8644, -98.8708, -98.8772, -98.8836, -98.8899
            //Stare - 3440016191299518474 x 10

            //Array a_var - uint64 for stare indices
            //The first index is an actual stare value from: MYD09.A2019003.2040.006.2019005020913_sidecar.h5
            //The final value is made up.
            vector<dods_uint64> target_indices = {3440016721727979534, 3440012343008821258, 3440016322296021006};

            D4RValueList params;
            params.add_rvalue(new D4RValue(a_var));
            params.add_rvalue(new D4RValue(target_indices));

            BaseType *result = StareSubsetFunction::stare_subset_dap4_function(&params, *two_arrays_dmr);

            CPPUNIT_ASSERT(dynamic_cast<Structure*>(result) != nullptr);

            Structure *subset_result = dynamic_cast<Structure*>(result);
            Array *stare = dynamic_cast<Array*>(subset_result->var("stare"));

            CPPUNIT_ASSERT(stare != nullptr);
            vector<dods_uint64> result_s_indices;
            stare->value(&result_s_indices[0]);

            DBG(cerr << "S Indices length: " << result_s_indices.size() << endl);
        }
        catch(Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_FAIL("count_function_test() test failed");
        }
        catch(BESError &e) {
            DBG(cerr << e.get_verbose_message() << endl);
            CPPUNIT_FAIL("count_function_test() test failed");
        }
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION( StareFunctionsTest );

int main(int argc, char*argv[]) {

    int ch;

    while ((ch = getopt(argc, argv, "dDh")) != -1) {
        switch (ch) {
            case 'd':
                debug = true;
                break;
            case 'D':
                bes_debug = true;
                break;
            case 'h': {
                cerr << "StareFunctionsTest has the following tests: " << endl;
                const std::vector<Test*> &tests = StareFunctionsTest::suite()->getTests();
                unsigned int prefix_len = StareFunctionsTest::suite()->getName().append("::").length();
                for (std::vector<Test*>::const_iterator i = tests.begin(), e = tests.end(); i != e; ++i) {
                    cerr << (*i)->getName().replace(0, prefix_len, "") << endl;
                }
                break;
            }
            default:
                break;
        }
    }
    argc -= optind;
    argv += optind;

	CppUnit::TextTestRunner runner;
	runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

	bool wasSuccessful = true;
	if (argc == 0) {
		// run them all
		wasSuccessful = runner.run("");
	} else {
        int i = 0;
		while (i < argc) {
			if (debug) cerr << "Running " << argv[i] << endl;
			string test = StareFunctionsTest::suite()->getName().append("::").append(argv[i]);
			wasSuccessful = wasSuccessful && runner.run(test);
			++i;
		}
	}

	return wasSuccessful ? 0 : 1;
}
