// utilT.C

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

#include <iostream>
#include <fstream>
#include <cstdlib>

using std::cerr;
using std::cout;
using std::endl;
using std::ifstream;

#include "BESUtil.h"
#include "BESError.h"
#include "TheBESKeys.h"
#include "test_config.h"
#include <GetOpt.h>

static bool debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

class utilT: public TestFixture {
private:

public:
    utilT()
    {
    }
    ~utilT()
    {
    }

    void display_values(const list<string> &values)
    {
        list<string>::const_iterator i = values.begin();
        list<string>::const_iterator e = values.end();
        for (; i != e; i++) {
            cout << "  " << (*i) << endl;
        }
    }

    void setUp()
    {
        string bes_conf = (string) TEST_SRC_DIR + "/bes.conf";
        TheBESKeys::ConfigFile = bes_conf;
    }

    void tearDown()
    {
    }

CPPUNIT_TEST_SUITE( utilT );

    CPPUNIT_TEST( do_test );

    CPPUNIT_TEST_SUITE_END()
    ;

    void do_test()
    {
        cout << "*****************************************" << endl;
        cout << "Entered utilT::run" << endl;

        cout << "*****************************************" << endl;
        cout << "Remove escaped quotes" << endl;
        string s = BESUtil::unescape("\\\"This is a test, this is \\\"ONLY\\\" a test\\\"");
        string result = "\"This is a test, this is \"ONLY\" a test\"";
        CPPUNIT_ASSERT( s == result );

        cout << "*****************************************" << endl;
        cout << "Remove Leading and Trailing Blanks" << endl;
        s = "This is a test";
        result = s;
        BESUtil::removeLeadingAndTrailingBlanks(s);
        CPPUNIT_ASSERT( s == result );

        s = " This is a test";
        BESUtil::removeLeadingAndTrailingBlanks(s);
        CPPUNIT_ASSERT( s == result );

        s = "	This is a test";
        BESUtil::removeLeadingAndTrailingBlanks(s);
        CPPUNIT_ASSERT( s == result );

        s = "    This is a test";
        BESUtil::removeLeadingAndTrailingBlanks(s);
        CPPUNIT_ASSERT( s == result );

        s = "    	This is a test";
        BESUtil::removeLeadingAndTrailingBlanks(s);
        CPPUNIT_ASSERT( s == result );

        s = "    	This is a test ";
        BESUtil::removeLeadingAndTrailingBlanks(s);
        CPPUNIT_ASSERT( s == result );

        s = "    	This is a test    ";
        BESUtil::removeLeadingAndTrailingBlanks(s);
        CPPUNIT_ASSERT( s == result );

        s = "    	This is a test    	";
        BESUtil::removeLeadingAndTrailingBlanks(s);
        CPPUNIT_ASSERT( s == result );

        s = "This is a test    ";
        BESUtil::removeLeadingAndTrailingBlanks(s);
        CPPUNIT_ASSERT( s == result );

        s = "This is a test    	";
        BESUtil::removeLeadingAndTrailingBlanks(s);
        CPPUNIT_ASSERT( s == result );

        s = " 	This is a test 	\n";
        BESUtil::removeLeadingAndTrailingBlanks(s);
        CPPUNIT_ASSERT( s == result );

        s = "    ";
        result = "";
        BESUtil::removeLeadingAndTrailingBlanks(s);
        CPPUNIT_ASSERT( s == result );

        s = "    	";
        BESUtil::removeLeadingAndTrailingBlanks(s);
        CPPUNIT_ASSERT( s == result );

        cout << "*****************************************" << endl;
        cout << "Exploding delimited strings" << endl;
        list<string> values;

        string value = "val1,val2,val3,val4";
        cout << value << endl;
        BESUtil::explode(',', value, values);
        display_values(values);
        CPPUNIT_ASSERT( values.size() == 4 );

        list<string>::iterator i = values.begin();
        list<string>::iterator i1 = i++;
        list<string>::iterator i2 = i++;
        list<string>::iterator i3 = i++;
        list<string>::iterator i4 = i++;
        CPPUNIT_ASSERT( (*i1) == "val1" );
        CPPUNIT_ASSERT( (*i2) == "val2" );
        CPPUNIT_ASSERT( (*i3) == "val3" );
        CPPUNIT_ASSERT( (*i4) == "val4" );

        values.clear();

        value = "val1,val2,val3,val4,";
        cout << value << endl;
        BESUtil::explode(',', value, values);
        display_values(values);
        CPPUNIT_ASSERT( values.size() == 5 );
        i = values.begin();
        i1 = i++;
        i2 = i++;
        i3 = i++;
        i4 = i++;
        list<string>::iterator i5 = i++;
        CPPUNIT_ASSERT( (*i1) == "val1" );
        CPPUNIT_ASSERT( (*i2) == "val2" );
        CPPUNIT_ASSERT( (*i3) == "val3" );
        CPPUNIT_ASSERT( (*i4) == "val4" );
        CPPUNIT_ASSERT( (*i5) == "" );

        values.clear();

        value = "val1;\"val2 with quotes\";val3;\"val4 with quotes\"";
        cout << value << endl;
        BESUtil::explode(';', value, values);
        display_values(values);
        CPPUNIT_ASSERT( values.size() == 4 );
        i = values.begin();
        i1 = i++;
        i2 = i++;
        i3 = i++;
        i4 = i++;
        CPPUNIT_ASSERT( (*i1) == "val1" );
        CPPUNIT_ASSERT( (*i2) == "\"val2 with quotes\"" );
        CPPUNIT_ASSERT( (*i3) == "val3" );
        CPPUNIT_ASSERT( (*i4) == "\"val4 with quotes\"" );

        values.clear();

        value = "val1;\"val2 with \\\"embedded quotes\\\"\";val3;\"val4 with quotes\";";
        cout << value << endl;
        BESUtil::explode(';', value, values);
        display_values(values);
        CPPUNIT_ASSERT( values.size() == 5 );
        i = values.begin();
        i1 = i++;
        i2 = i++;
        i3 = i++;
        i4 = i++;
        i5 = i++;
        CPPUNIT_ASSERT( (*i1) == "val1" );
        CPPUNIT_ASSERT( (*i2) == "\"val2 with \\\"embedded quotes\\\"\"" );
        CPPUNIT_ASSERT( (*i3) == "val3" );
        CPPUNIT_ASSERT( (*i4) == "\"val4 with quotes\"" );
        CPPUNIT_ASSERT( (*i5) == "" );

        cout << "*****************************************" << endl;
        cout << "Imploding list to delimited string" << endl;
        values.clear();
        values.push_back("a");
        values.push_back("b");
        values.push_back("c");
        values.push_back("d");
        result = BESUtil::implode(values, ',');
        CPPUNIT_ASSERT( result == "a,b,c,d" );

        cout << "*****************************************" << endl;
        cout << "Imploding list with a delimiter in a value" << endl;
        values.push_back("a,b");
        try {
            result = BESUtil::implode(values, ',');
            CPPUNIT_ASSERT( !"imploding of value with comma" );
        }
        catch (BESError &e) {
        }
        values.clear();
        values.push_back("a");
        values.push_back("\"a,b\"");
        values.push_back("b");
        result = BESUtil::implode(values, ',');
        CPPUNIT_ASSERT( result == "a,\"a,b\",b" );

        cout << "*****************************************" << endl;
        cout << "Returning from utilT::run" << endl;
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( utilT );

int main(int argc, char*argv[])
{

    GetOpt getopt(argc, argv, "dh");
    int option_char;
    while ((option_char = getopt()) != EOF)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;
        case 'h': {     // help - show test names
            cerr << "Usage: utilT has the following tests:" << endl;
            const std::vector<Test*> &tests = utilT::suite()->getTests();
            unsigned int prefix_len = utilT::suite()->getName().append("::").length();
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
            test = utilT::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}

