// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include "config.h"

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <sys/types.h>

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <unistd.h>  // for stat
#include <sstream>

//#define DODS_DEBUG

#include <ObjectType.h>
#include <EncodingType.h>
#include <ServerFunction.h>
#include <ServerFunctionsList.h>
#include <ConstraintEvaluator.h>
#include <DAS.h>
#include <DDS.h>
#include <Str.h>

#include <GetOpt.h>

#include <GNURegex.h>
#include <util.h>
#include <mime_util.h>
#include <debug.h>

#include <test/TestTypeFactory.h>
#include <test/TestByte.h>

#include "TheBESKeys.h"
#include "BESDapResponseBuilder.h"
#include "testFile.h"
#include "test_config.h"

using namespace CppUnit;
using namespace std;
using namespace libdap;

int test_variable_sleep_interval = 0;

static bool debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

static void
rb_simple_function(int, BaseType *[], DDS &, BaseType **btpp)
{
    Str *response = new Str("result");

    response->set_value("qwerty");
    *btpp = response;
    return;
}

static void
parse_datadds_response(istream &in, string &prolog, vector<char> &blob)
{
	// Split the stuff in the input stream into two parts:
	// The text prolog and the binary blob
	const int line_length = 1024;
    // Read up to 'Data:'
	char line[line_length];
	while (!in.eof()) {
		in.getline(line, line_length);
		DBG(cerr << "prolog line: " << line << endl);
		if (strncmp(line, "Data:", 5) == 0)
			break;
		prolog += string(line);
		prolog += "\n";
	}

	// Read the blob
	streampos pos = in.tellg();
	in.seekg(0, in.end);
	unsigned int length = in.tellg() - pos;
	DBG(cerr << "blob length: " << length << endl);

	// return to byte just after 'Data:'
	in.seekg(pos, in.beg);

	blob.reserve(length);
	in.read(&blob[0], length);
}

class ResponseBuilderTest: public TestFixture {
private:
    BESDapResponseBuilder *df, *df3, *df5, *df6;

    AttrTable *cont_a;
    DAS *das;
    DDS *dds;
    ostringstream oss;
    time_t now;
    char now_array[256];
    libdap::ServerFunction *rbSSF;

    void loadServerSideFunction() {
        rbSSF = new libdap::ServerFunction(
            // The name of the function as it will appear in a constraint expression
            "rbSimpleFunc",
            // The version of the function
            "1.0",
            // A brief description of the function
            "Returns a string",
            // A usage/syntax statement
            "rbSimpleFunc()",
            // A URL that points two a web page describing the function
            "http://docs.opendap.org/index.php/Hyrax:_Server_Side_Functions",
            // A URI that defines the role of the function
            "http://services.opendap.org/dap4/unit-tests/ResponseBuilderTest",
            // A pointer to the helloWorld() function
            rb_simple_function
        );

        libdap::ServerFunctionsList::TheList()->add_function(rbSSF);
    }

public:
    ResponseBuilderTest(): df(0), df3(0), df5(0), df6(0), cont_a(0), das(0), dds(0) {
        now = time(0);
        ostringstream time_string;
        time_string << (int) now;
        strncpy(now_array, time_string.str().c_str(), 255);
        now_array[255] = '\0';

        loadServerSideFunction();
    }

    ~ResponseBuilderTest() {
    	// delete rbSSF; NB: ServerFunctionsList is a singleton that deletes its entries at exit.
    }

    void setUp() {
        // Test pathname
        df = new BESDapResponseBuilder();

        // This file has an ancillary DAS in the input-files dir.
        // df3 is also used to test escaping stuff in URLs. 5/4/2001 jhrg
        df3 = new BESDapResponseBuilder();
        df3->set_dataset_name((string)TEST_SRC_DIR + "/input-files/coads.data");
        df3->set_ce("u,x,z[0]&grid(u,\"lat<10.0\")");
        df3->set_timeout(1);

        // Test escaping stuff. 5/4/2001 jhrg
        df5 = new BESDapResponseBuilder();
        df5->set_dataset_name("nowhere%5Bmydisk%5Dmyfile");
        df5->set_ce("u%5B0%5D");

        // Try a server side function call.
        // loadServerSideFunction(); NB: This is called by the test's ctor
        df6 = new BESDapResponseBuilder();
        df6->set_dataset_name((string)TEST_SRC_DIR + "/input-files/bears.data");
        //df6->set_ce("rbFuncTest()");
        df6->set_timeout(1);

        cont_a = new AttrTable;
        cont_a->append_attr("size", "Int32", "7");
        cont_a->append_attr("type", "String", "cars");
        das = new DAS;
        das->add_table("a", cont_a);

        // This AttrTable looks like:
        //      Attributes {
        //          a {
        //              Int32 size 7;
        //              String type cars;
        //          }
        //      }

        TestTypeFactory ttf;
        dds = new DDS(&ttf, "test");
        TestByte a("a");
        dds->add_var(&a);

        dds->transfer_attributes(das);
        dds->set_dap_major(3);
        dds->set_dap_minor(2);

        TheBESKeys::ConfigFile = (string)TEST_SRC_DIR + "/input-files/test.keys";
    }

    void tearDown() {
        delete df; df = 0;
        delete df3; df3 = 0;
        delete df5; df5 = 0;
        delete df6; df6 = 0;

        delete das; das = 0;
        delete dds; dds = 0;
    }

    bool re_match(Regex &r, const string &s) {
        DBG(cerr << "s.length(): " << s.length() << endl);
        int pos = r.match(s.c_str(), s.length());
        DBG(cerr << "r.match(s): " << pos << endl);
        return pos > 0 && static_cast<unsigned> (pos) == s.length();
    }

    bool re_match_binary(Regex &r, const string &s) {
        DBG(cerr << "s.length(): " << s.length() << endl);
        int pos = r.match(s.c_str(), s.length());
        DBG(cerr << "r.match(s): " << pos << endl);
        return pos > 0;
    }

	void send_das_test() {
		try {
		string baseline = readTestBaseline((string)TEST_SRC_DIR + "/input-files/send_das_baseline.txt");
		DBG( cerr << "---- start baseline ----" << endl << baseline << "---- end baseline ----" << endl);
		Regex r1(baseline.c_str());

		df->send_das(oss, *das);

		DBG(cerr << "DAS: " << oss.str() << endl);

		CPPUNIT_ASSERT(re_match(r1, oss.str()));
		oss.str("");
		}
		catch (Error &e) {
			CPPUNIT_FAIL(e.get_error_message());
		}
	}

    void send_dds_test() {
    	try {
    	string baseline = readTestBaseline((string)TEST_SRC_DIR + "/input-files/send_dds_baseline.txt");
		DBG( cerr << "---- start baseline ----" << endl << baseline << "---- end baseline ----" << endl);
		Regex r1(baseline.c_str());

        ConstraintEvaluator ce;

        df->send_dds(oss, *dds, ce);

        DBG(cerr << "DDS: " << oss.str() << endl);

        CPPUNIT_ASSERT(re_match(r1, oss.str()));
        oss.str("");
		}
		catch (Error &e) {
			CPPUNIT_FAIL(e.get_error_message());
		}
    }

    void send_ddx_test() {
        string baseline = readTestBaseline((string) TEST_SRC_DIR + "/input-files/response_builder_send_ddx_test.xml");
        Regex r1(baseline.c_str());
        ConstraintEvaluator ce;

        try {
            df->send_ddx(oss, *dds, ce);

            DBG(cerr << "DDX: " << oss.str() << endl);

            CPPUNIT_ASSERT(re_match(r1, baseline));
            //CPPUNIT_ASSERT(re_match(r1, oss.str()));
            //oss.str("");
        } catch (Error &e) {
            CPPUNIT_FAIL("Error: " + e.get_error_message());
        }
    }


    void escape_code_test() {
        // These should NOT be escaped.
        DBG(cerr << df3->get_dataset_name() << endl); DBG(cerr << df3->get_ce() << endl);

        CPPUNIT_ASSERT(df3->get_dataset_name() == (string)TEST_SRC_DIR + "/input-files/coads.data");
        CPPUNIT_ASSERT(df3->get_ce() == "u,x,z[0]&grid(u,\"lat<10.0\")");

        // The ResponseBuilder instance is feed escaped values; they should be
        // unescaped by the ctor and the mutators. 5/4/2001 jhrg

        DBG(cerr << df5->get_dataset_name() << endl); DBG(cerr << df5->get_ce() << endl);

        CPPUNIT_ASSERT(df5->get_dataset_name() == "nowhere[mydisk]myfile");
        CPPUNIT_ASSERT(df5->get_ce() == "u[0]");

        df5->set_ce("u%5B0%5D");
        CPPUNIT_ASSERT(df5->get_ce() == "u[0]");

        df5->set_ce("Grid%20u%5B0%5D");
        CPPUNIT_ASSERT(df5->get_ce() == "Grid%20u[0]");
    }

    // This tests reading the timeout value from argv[].
    void timeout_test() {
        CPPUNIT_ASSERT(df3->get_timeout() == 1);
        CPPUNIT_ASSERT(df5->get_timeout() == 0);
    }

    void invoke_server_side_function_test() {
        try {
            string baseline = readTestBaseline((string)TEST_SRC_DIR + "/input-files/simple_function_baseline.txt");
            Regex r1(baseline.c_str());

            DBG( cerr << "---- start baseline ----" << endl << baseline << "---- end baseline ----" << endl);
#if 1
            df6->set_ce("rbSimpleFunc()");
#else
            df6->set_ce("");
#endif
            ConstraintEvaluator ce;
            df6->send_data(oss, *dds, ce);

            DBG( cerr << "---- start result ----" << endl << oss.str() << "---- end result ----" << endl);

			string prolog;
			vector<char> blob;
			istringstream iss(oss.str());
			parse_datadds_response(iss, prolog, blob);

			DBG( cerr << "prolog: " << prolog << endl);

            CPPUNIT_ASSERT(re_match(r1, prolog));

            // This block of code was going to test if the binary data
            // in the response document matches some sequence of bytes
            // in a baseline file. it's not working and likely not that
            // important - the function under test returns a string and
            // it's clearly present in the output when instrumentation is
            // on. Return to this when there's time. 5/20/13 jhrg
#if 0
            ifstream blob_baseline_in(((string)TEST_SRC_DIR + "/input-files/blob_baseline.bin").c_str());

            blob_baseline_in.seekg(0, blob_baseline_in.end);
        	unsigned int blob_baseline_length = blob_baseline_in.tellg();

        	DBG(cerr << "blob_baseline length: " << blob_baseline_length << endl);
        	DBG(cerr << "blob size: " << blob.size() << endl);

        	CPPUNIT_ASSERT(blob_baseline_length == blob.size());

        	blob_baseline_in.seekg(0, blob_baseline_in.beg);

            char blob_baseline[blob_baseline_length];
            blob_baseline_in.read(blob_baseline, blob_baseline_length);
            blob_baseline_in.close();
            for (int i = 0; i < blob_baseline_length; ++i) {
            	DBG(cerr << "bb[" << i << "]: " << blob_baseline[i] << endl);
            	DBG(cerr << "blob[" << i << "]: " << blob_baseline[i] << endl);
            }
#endif
        } catch (Error &e) {
            CPPUNIT_FAIL("Caught libdap::Error!! Message:" + e.get_error_message());
        }
    }


    CPPUNIT_TEST_SUITE( ResponseBuilderTest );

        CPPUNIT_TEST(send_das_test);
        CPPUNIT_TEST(send_dds_test);
        CPPUNIT_TEST(send_ddx_test);

        CPPUNIT_TEST(escape_code_test);
        CPPUNIT_TEST(invoke_server_side_function_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ResponseBuilderTest);

int main(int argc, char*argv[]) {
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    GetOpt getopt(argc, argv, "d");
    char option_char;
    while ((option_char = getopt()) != EOF)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
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
            test = string("ResponseBuilderTest::") + argv[i++];

            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
