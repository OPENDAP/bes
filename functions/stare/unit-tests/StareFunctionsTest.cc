#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <GetOpt.h>
#include <BaseType.h>
#include <Int32.h>
#include <Float64.h>
#include <Str.h>
#include <Array.h>
#include <Grid.h>
#include <DDS.h>

#include <util.h>
#include <debug.h>
#include <BESDebug.h>

#include "StareIterateFunction.h"

#include "test_config.h"

#include "test/TestTypeFactory.h"

#include "debug.h"

using namespace CppUnit;
using namespace libdap;
using namespace std;
using namespace functions;

class StareFunctionsTest: public TestFixture {
private:
	DDS *dds;

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
        DBG(cerr << "setup() - Built DMR(D4BaseTypeFactory *) " << endl);

		BaseType *a_var = two_arrays_dmr->root()->var("a");
		BaseType *b_var = two_arrays_dmr->root()->var("b");

		// Load values into the grid variables
		Array & a = dynamic_cast<Array &>(*a_var);
		Array & b = dynamic_cast<Array &>(*b_var);

		//Lat - 32.2739, 32.2736, 32.2733, 32.2731, 32.2728, 32.2725, 32.2723, 32.272, 32.2718, 32.2715
		//Lon - -98.8324, -98.8388, -98.8452, -98.8516, -98.858, -98.8644, -98.8708, -98.8772, -98.8836, -98.8899
		//Stare - 3440016191299518474 x 10
		//Array a - uint64 for stare indices
		dods_uint64 first_a[10] = { [0 ... 9] = 3440016191299518474};

		a.set_value(first_a, 10);
		a.set_read_p(true);
	}

	void tearDown() {
		delete dds;
		dds = 0;

		delete two_arrays_dmr;
		two_arrays_dmr = 0;
		delete d4_btf;
		d4_btf = 0;
	}

CPPUNIT_TEST_SUITE( StareFunctionsTest );

	CPPUNIT_TEST(serverside_compare_test);

	CPPUNIT_TEST_SUITE_END()
	;

	void serverside_compare_test() {
		DBG(cerr << "--Test basic setup of function--" << endl);
		try {
			D4RValueList params;

			Array & stareIndices = dynamic_cast<Array &>(*two_arrays_dmr->root()->var("a"));

			//Parameters will be an array of uint64
			params.add_rvalue(new D4RValue(&stareIndices));

			BaseType *btype = stare_dap4_function(&params, &btf);


			CPPUNIT_ASSERT(checkHasValue == true);
		}
		catch(Error &e) {
			DBG(cerr << e.get_error_message() << endl);
			CPPUNIT_FAIL("Compare test failed");
		}
	}

};

CPPUNIT_TEST_REGISTRATION( StareFunctionsTest );

int main(int argc, char*argv[]) {
	GetOpt getopt(argc, argv, "dh");
	char option_char;

	while ((option_char = getopt()) != EOF) {
		switch (option_char) {
		case d:
			debug = 1;
			break;
		case h:
			cerr << "StareFunctionsTest has the following tests: " << endl;
			const std::vector<Test*> &tests = StareFunctionsTest::suite()->getTests();
			unsigned int prefix_len = StareFunctionsTest::suite()->getName().append("::").length();
			for (std::vector<Test*>::const_iterator i = tests.begin(), e = tests.end(); i != e; ++i) {
				cerr << (*i)->getName().replace(0, prefix_len, "") << endl;
			}
			break;
		default:
			break;
		}
	}

	CppUnit::TextTestRunner runner;
	runner.addTest(CppUnit::StareFunctionsTest::getRegistry().makeTest());

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
