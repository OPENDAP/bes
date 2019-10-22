#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <GetOpt.h>
#include <BaseType.h>
#include <Array.h>
#include <Int32.h>
#include <UInt64.h>
#include <D4Group.h>
#include <D4RValue.h>
#include <DMR.h>

#include <util.h>
#include <debug.h>

#include <TheBESKeys.h>
#include <BESDebug.h>

#include "StareIterateFunction.h"

#include "test_config.h"

#include "test/D4TestTypeFactory.h"

#include "debug.h"

using namespace CppUnit;
using namespace libdap;
using namespace std;
using namespace functions;

static bool debug = false;
#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

class StareFunctionsTest: public TestFixture {
private:
	DMR *two_arrays_dmr;
	D4BaseTypeFactory *d4_btf;
public:
	StareFunctionsTest() : two_arrays_dmr(0), d4_btf(0)
	{
	}

	~StareFunctionsTest()
	{
	}

	void setUp() {
        d4_btf = new D4BaseTypeFactory();
        DBG(cerr << "setup() - Built D4BaseTypeFactory() " << endl);

        two_arrays_dmr = new DMR(d4_btf);
        two_arrays_dmr->set_name("test_dmr");
        DBG(cerr << "setup() - Built DMR(D4BaseTypeFactory *) " << endl);

		string filename = "MYD09.A2019003.2040.006.2019005020913.h5";

		two_arrays_dmr->set_filename(filename);

		TheBESKeys::ConfigFile = "/Users/kodi/src/hyrax/bes/functions/stare/unit-tests/bes.conf";
	}

	void tearDown() {
		delete two_arrays_dmr;
		two_arrays_dmr = 0;
		delete d4_btf;
		d4_btf = 0;
	}

	CPPUNIT_TEST_SUITE( StareFunctionsTest );

	CPPUNIT_TEST(serverside_compare_test);

	CPPUNIT_TEST_SUITE_END();

	void serverside_compare_test() {
		DBG(cerr << "--- hasValue() test - BEGIN ---" << endl);

		Array *a_var = new Array("a_var", new UInt64("a_var"));

		two_arrays_dmr->root()->add_var_nocopy(a_var);

		//MYD09.A2019003.2040.006.2019005020913_sidecar.h5 values:
		//Lat - 32.2739, 32.2736, 32.2733, 32.2731, 32.2728, 32.2725, 32.2723, 32.272, 32.2718, 32.2715
		//Lon - -98.8324, -98.8388, -98.8452, -98.8516, -98.858, -98.8644, -98.8708, -98.8772, -98.8836, -98.8899
		//Stare - 3440016191299518474 x 10

		//Array a_var - uint64 for stare indices
		//First two indices are actual stare values from: MYD09.A2019003.2040.006.2019005020913_sidecar.h5
		//The final value is made up.
		vector<dods_uint64> target_indices = {9223372034707292159, 3440012343008821258, 3440016191299518400};

		try {
			D4RValueList params;
			params.add_rvalue(new D4RValue(target_indices));

			BaseType *checkHasValue = stare_dap4_function(&params, *two_arrays_dmr);

			CPPUNIT_ASSERT(dynamic_cast<Int32*> (checkHasValue)->value() == 1);
		}
		catch(Error &e) {
			DBG(cerr << e.get_error_message() << endl);
			CPPUNIT_FAIL("hasValue() test failed");
		}

        DBG(cerr << "--- hasValue() test - END ---" << endl);
	}

};

CPPUNIT_TEST_SUITE_REGISTRATION( StareFunctionsTest );

int main(int argc, char*argv[]) {
	GetOpt getopt(argc, argv, "dh");
	char option_char;

	while ((option_char = getopt()) != EOF) {
		switch (option_char) {
		case 'd':
			debug = 1;
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

	CppUnit::TextTestRunner runner;
	runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

	bool wasSuccessful = true;
	string test = "";
	int i = getopt.optind;
	if (i == argc) {
		// run them all
		wasSuccessful = runner.run("");
	} else {
		while (i < argc) {
			if (debug) cerr << "Running " << argv[i] << endl;
			test = StareFunctionsTest::suite()->getName().append("::").append(argv[i]);
			wasSuccessful = wasSuccessful && runner.run(test);
			++i;
		}
	}

	return wasSuccessful ? 0 : 1;
}
