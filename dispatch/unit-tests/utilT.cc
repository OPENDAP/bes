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
#include <sstream>
#include <cstdlib>
#include <list>

using std::cerr;
using std::cout;
using std::endl;
using std::ifstream;
using std::string;
using std::list;

#include "BESUtil.h"
#include "BESError.h"
#include "TheBESKeys.h"
#include "test_config.h"
#include <GetOpt.h>

static bool debug = false;
static bool Debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);
#define prolog std::string("utilT::").append(__func__).append("() - ")

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
        if(debug) cerr << endl;
        string bes_conf = (string) TEST_SRC_DIR + "/bes.conf";
        TheBESKeys::ConfigFile = bes_conf;
    }

    void tearDown()
    {
    }


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


    void replace_all_worker(
            const string &id,
            string &source_str,
            const string &template_str,
            const string &replace_str,
            const string &baseline_str){

        try {
            if(debug) cerr << id << "(" << prolog <<  ") " << "  Source String: '" << source_str << "'" << endl;
            if(debug) cerr << id << "(" << prolog <<  ") " << "Template String: '" << template_str << "'" << endl;
            if(debug) cerr << id << "(" << prolog <<  ") " << " Replace String: '" << replace_str << "'" << endl;
            if(debug) cerr << id << "(" << prolog <<  ") " << "       Expected: '" << baseline_str << "'" << endl;

            unsigned int replace_count = BESUtil::replace_all(source_str, template_str, replace_str);
            bool result_matched = source_str == baseline_str;
            if(debug) cerr << id << "(" << prolog <<  ") " << "     Result(" << replace_count << "): '" << source_str << "'" << endl;

            std::stringstream info_msg;
            info_msg << id << "(" << prolog <<  ") " << "The filtered string " << (result_matched?"MATCHED ":"DID NOT MATCH ")
                     << "the baseline: " << endl;
            if(debug) cerr << info_msg.str();
            CPPUNIT_ASSERT_MESSAGE(info_msg.str(),result_matched);

        }
        catch(BESError &be){
            std::stringstream msg;
            msg << prolog << "Caught BESError. Message: " << be.get_verbose_message() << " ";
            msg << be.get_file() << " " << be.get_line();
            if(debug) cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }
    }
    /**
     * Test replace_all() string function
     */
    void replace_all_test_01() {
        if(debug) cerr << prolog << "BEGIN" << endl;
        string source_str = "aabaabaabbbbaaaaaabbabbbaaabbbaaaaabbabaabbabbaaabbaabbaaabejklgvaxxxaccaxxxacccaccaaacaabo";
        string template_str = "aab";
        string replace_str = "###";
        string baseline_str = "#########bbbaaaa###babbba###bbaaa###bab###babba###b###ba###ejklgvaxxxaccaxxxacccaccaaac###o";

        replace_all_worker(prolog, source_str,template_str,replace_str, baseline_str );

        if(debug) cerr << prolog << "END" << endl;
    }

    void replace_all_test_02() {
        if(debug) cerr << prolog << "BEGIN" << endl;
        string source_str = "#########bbbaaaa###babbba###bbaaa###bab###babba###b###ba###ejklgvaxxxaccaxxxacccaccaaac###o";
        string template_str = "#";
        string replace_str = "";
        string baseline_str = "bbbaaaababbbabbaaababbabbabbaejklgvaxxxaccaxxxacccaccaaaco";

        replace_all_worker(prolog, source_str,template_str,replace_str, baseline_str );

        if(debug) cerr << prolog << "END" << endl;
    }

    void replace_all_test_03() {
        if(debug) cerr << prolog << "BEGIN" << endl;
        string source_str = "The quick brown fox jumped over the lazy dog.";
        string template_str = "quick brown fox";
        string replace_str  = "grasshopper";
        string baseline_str = "The grasshopper jumped over the lazy dog.";

        replace_all_worker(prolog, source_str,template_str,replace_str, baseline_str );

        if(debug) cerr << prolog << "END" << endl;
    }

    void replace_all_test_04() {
        if(debug) cerr << prolog << "BEGIN" << endl;
        string source_str = "The quick brown fox jumped over the lazy dog.";
        string template_str = "quick brown fox";
        string replace_str  = "reckless skateboarder";
        string baseline_str = "The reckless skateboarder jumped over the lazy dog.";

        replace_all_worker(prolog, source_str,template_str,replace_str, baseline_str );

        if(debug) cerr << prolog << "END" << endl;
    }

CPPUNIT_TEST_SUITE( utilT );

        CPPUNIT_TEST( do_test );
        CPPUNIT_TEST( replace_all_test_01 );
        CPPUNIT_TEST( replace_all_test_02 );
        CPPUNIT_TEST( replace_all_test_03 );
        CPPUNIT_TEST( replace_all_test_04 );

    CPPUNIT_TEST_SUITE_END();


};

CPPUNIT_TEST_SUITE_REGISTRATION( utilT );

int main(int argc, char*argv[])
{

    GetOpt getopt(argc, argv, "dh");
    int option_char;
    while ((option_char = getopt()) != EOF)
        switch (option_char) {
            case 'd':
                debug = true;  // debug is a static global
                break;
            case 'D':
                Debug = true;  // debug is a static global
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

