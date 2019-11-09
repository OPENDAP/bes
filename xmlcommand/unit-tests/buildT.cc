// buildT.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
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
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

using namespace CppUnit;

#include <string>
#include <iostream>
#include <sstream>

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::ostringstream;

#include "BuildTInterface.h"
#include "BuildTCmd1.h"
#include "BuildTCmd2.h"
#include "BESError.h"
#include "TheBESKeys.h"

#include <GetOpt.h>

#include "test_config.h"

int what_test = 0;

static bool debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

class buildT: public TestFixture {
private:

public:
    buildT()
    {
    }
    ~buildT()
    {
    }

    void setUp()
    {
    }

    void tearDown()
    {
    }

    CPPUNIT_TEST_SUITE( buildT );

    CPPUNIT_TEST( do_test );

    CPPUNIT_TEST_SUITE_END();

    void do_test()
    {
        TheBESKeys::ConfigFile = string(TEST_SRC_DIR) + "/bes.conf";

        DBG(cerr << "TheBESKeys::ConfigFile = " << TheBESKeys::ConfigFile << endl);

        try {
            BESXMLCommand::add_command("cmd1.1", BuildTCmd1::Cmd1Builder);
            BESXMLCommand::add_command("cmd2.1", BuildTCmd2::Cmd2Builder);
            BESXMLCommand::add_command("cmd1.2", BuildTCmd1::Cmd1Builder);
            BESXMLCommand::add_command("cmd2.2", BuildTCmd2::Cmd2Builder);
        }
        catch (BESError &e) {
            CPPUNIT_FAIL( "Failed to add commands: " + e.get_message() );
        }

        try {
            string myDoc =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?> \
<request reqID =\"some_unique_value\" > \
   <cmd1.1 prop1=\"prop1val\"> \
	<element1>element1val</element1> \
	<element2 prop2.1=\"prop2.1val\">element2val</element2> \
    </cmd1.1> \
    <cmd2.1 prop2=\"prop2val\"> \
	<element3> \
	    <element3.1 prop3.1.1=\"prop3.1.1val\" /> \
	</element3> \
	<element4> \
	    element4val \
	</element4> \
    </cmd2.1> \
</request>";
            BuildTInterface bti(myDoc);
            bti.run();
            CPPUNIT_ASSERT( what_test == 2 );
        }
        catch (BESError &e) {
            CPPUNIT_FAIL( "Failed with exception: " + e.get_message() );
        }
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION( buildT );

int main(int argc, char*argv[])
{

    GetOpt getopt(argc, argv, "dh");

    string env_var = (string) "BES_CONF=" + TEST_SRC_DIR + "/bes.conf";
    putenv((char *) env_var.c_str());

    int option_char;
    while ((option_char = getopt()) != EOF)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;
        case 'h': {     // help - show test names
            cerr << "Usage: buildT has the following tests:" << endl;
            const std::vector<Test*> &tests = buildT::suite()->getTests();
            unsigned int prefix_len = buildT::suite()->getName().append("::").length();
            for (std::vector<Test*>::const_iterator i = tests.begin(), e = tests.end(); i != e; ++i) {
                cerr << (*i)->getName().replace(0, prefix_len, "") << endl;
            }
            break;
        }
        default:
            break;
        }

    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    bool wasSuccessful = true;
    string test = "";
    int i = getopt.optind;
    if (i == argc) {
        // run them all
        wasSuccessful = runner.run("");
    }
    else {
        while (i < argc) {
            if (debug) cerr << "Running " << argv[i] << endl;
            test = buildT::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}

