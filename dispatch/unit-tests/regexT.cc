// regexT.C

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
#include "test_config.h"

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

using namespace CppUnit;

#include <iostream>
#include <regex>
#include <unistd.h>

using std::cerr;
using std::cout;
using std::endl;
using std::string;

#include "BESRegex.h"
#include "BESError.h"
#include "BESStopWatch.h"
#include "TheBESKeys.h"

static bool debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);
#define prolog std::string("regexT::").append(__func__).append("() - ")

class regexT: public TestFixture {
private:

public:
    regexT() {
    }

    ~regexT() {
    }

    void setUp() {
        cout << endl;
        string bes_conf = (string) TEST_SRC_DIR + "/bes.conf";
        TheBESKeys::ConfigFile = bes_conf;
        TheBESKeys::TheKeys()->set_key("BES.Data.RootDirectory=/dev/null");
        TheBESKeys::TheKeys()->set_key("BES.Info.Buffered=no");
        TheBESKeys::TheKeys()->set_key("BES.Info.Type=xml");
    }

    void tearDown() {
    }


    void do_bes_regex_worker(std::string caller_prolog, std::string regex_str, std::string subject_str, int expected_match_length) {
        int br_result;
        {
            BESStopWatch bes_bnm;
            bes_bnm.start(caller_prolog + "BESRegex build_and_match");
            BESRegex *bes_regex;
            {
                BESStopWatch bes_b;
                bes_b.start(caller_prolog + "BESRegex BUILD");
                bes_regex = new BESRegex(regex_str.c_str());
            }
            {
                BESStopWatch bes_m;
                bes_m.start(caller_prolog + "BESRegex MATCH");
                br_result = bes_regex->match(subject_str.c_str(), subject_str.length());
            }
            delete bes_regex;
        }
        cout << caller_prolog << "BESRegex::match(): " << br_result << endl;
        CPPUNIT_ASSERT(br_result == expected_match_length);
    }



    void do_test_worker(std::string caller_prolog, std::string regex_str, std::string subject_str, int expected_match_length) {
        try {
            cout << caller_prolog << "  regex_str: \"" << regex_str << "\""<< endl;
            cout << caller_prolog << "subject_str: \"" << subject_str << "\"" << endl;
            if (expected_match_length < 0) {
                cout << caller_prolog << "Expected match failure (" << expected_match_length << ")" << endl;
            } else {
                cout << caller_prolog << "Expected match length: " << expected_match_length << endl;
            }


            int br_result;
            {
                BESStopWatch bes_bnm;
                bes_bnm.start(caller_prolog + "BESRegex build_and_match");
                BESRegex *bes_regex;
                {
                    BESStopWatch bes_b;
                    bes_b.start(caller_prolog + "BESRegex BUILD");
                    bes_regex = new BESRegex(regex_str.c_str());
                }
                {
                    BESStopWatch bes_m;
                    bes_m.start(caller_prolog + "BESRegex MATCH");
                    br_result = bes_regex->match(subject_str.c_str(), subject_str.length());
                }
                delete bes_regex;
            }
            cout << caller_prolog << "BESRegex::match(): " << br_result << endl;
            CPPUNIT_ASSERT(br_result == expected_match_length);

            bool cpp_result;
            {
                BESStopWatch cpp_bnm;
                cpp_bnm.start(caller_prolog + "std::regex build_and_match");
                std::regex cpp_regex;
                {
                    BESStopWatch cpp_b;
                    cpp_b.start(caller_prolog + "std::regex assign");
                    cpp_regex.assign(regex_str);
                }
                {
                    BESStopWatch cpp_m;
                    cpp_m.start(caller_prolog + "std::regex match");
                    cpp_result = std::regex_match(subject_str, cpp_regex);
                }
            }
            cout << caller_prolog << "std::regex_match(): " << (cpp_result?"true":"false") << endl;
            bool expected = expected_match_length >= 0;
            cout << caller_prolog << "expected: " << (expected?"true":"false") << endl;
            CPPUNIT_ASSERT(cpp_result == expected);

        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT(!"Failed to match");
        }
    }


    void test_1(){
        cout << prolog << "BEGIN" << endl;
        string regex_str;
        string inQuestion;
        regex_str = "123456";
        inQuestion = "01234567";
        do_test_worker(prolog, regex_str, inQuestion, 6);
        cout << prolog << "END" << endl;
    }

    void test_2(){
        cout << prolog << "BEGIN" << endl;
        string regex_str;
        string inQuestion;
        regex_str = "^123456$";
        inQuestion = "01234567";
        do_test_worker(prolog, regex_str, inQuestion, -1);

        cout << prolog << "END" << endl;
    }

    void test_3(){
        cout << prolog << "BEGIN" << endl;
        string regex_str;
        string inQuestion;
        regex_str = "^123456$";
        inQuestion = "123456";
        do_test_worker(prolog, regex_str, inQuestion, 6);

        cout << prolog << "END" << endl;
    }

    void test_4(){
        cout << prolog << "BEGIN" << endl;
        string regex_str;
        string inQuestion;
        regex_str = ".*\\.nc$";
        inQuestion = "fnoc1.nc";
        do_test_worker(prolog, regex_str, inQuestion, 8);
        cout << prolog << "END" << endl;
    }

    void test_5(){
        cout << prolog << "BEGIN" << endl;
        string regex_str;
        string inQuestion;
        regex_str = ".*\\.nc$";
        inQuestion = "fnoc1.ncd";
        do_test_worker(prolog, regex_str, inQuestion, -1);
        cout << prolog << "END" << endl;
    }

    void test_6(){
        cout << prolog << "BEGIN" << endl;
        string regex_str;
        string inQuestion;
        regex_str = ".*\\.(nc|NC)(\\.gz|\\.bz2|\\.Z)?$";
        inQuestion = "fnoc1.nc";
        do_test_worker(prolog, regex_str, inQuestion, 8);
        cout << prolog << "END" << endl;
    }

    void test_7(){
        cout << prolog << "BEGIN" << endl;
        string regex_str;
        string inQuestion;
        regex_str = ".*\\.(nc|NC)(\\.gz|\\.bz2|\\.Z)?$";
        inQuestion = "fnoc1.nc.gz";
        do_test_worker(prolog, regex_str, inQuestion, 11);
        cout << prolog << "END" << endl;
    }

    void zero_length_match() {
        string regex_str = ".*";
        string inQuestion = "";
        do_test_worker(prolog, regex_str, inQuestion, 0);
    }

    void complex_regex_1() {
        string regex_str = R"(^https?:\/\/([a-z]|[0-9])(([a-z]|[0-9]|\.|-){1,61})([a-z]|[0-9])\.s3((\.|-)us-(east|west)-(1|2))?\.amazonaws\.com\/.*$)";
        string inQuestion = "http://wombat.s3-us-west-2.amazonaws.com/Dont_touch_that";
        do_test_worker(prolog, regex_str, inQuestion, 56);
    }
    void complex_regex_2() {
        string regex_str = R"(^https?:\/\/([a-z0-9])(([a-z0-9]|\.|-){1,61})([a-z0-9])\.s3((\.|-)us-(east|west)-(1|2))?\.amazonaws\.com\/.*$)";
        string inQuestion = "http://wombat.s3-us-west-2.amazonaws.com/Dont_touch_that";
        do_test_worker(prolog, regex_str, inQuestion, 56);
    }


    CPPUNIT_TEST_SUITE(regexT);
        CPPUNIT_TEST(test_1);
        CPPUNIT_TEST(test_2);
        CPPUNIT_TEST(test_3);
        CPPUNIT_TEST(test_4);
        CPPUNIT_TEST(test_5);
        CPPUNIT_TEST(test_6);
        CPPUNIT_TEST(test_7);
        CPPUNIT_TEST(complex_regex_1);
        CPPUNIT_TEST(complex_regex_2);
    CPPUNIT_TEST_SUITE_END();

};

    CPPUNIT_TEST_SUITE_REGISTRATION(regexT);

int main(int argc, char*argv[])
{
    int option_char;
    while ((option_char = getopt(argc, argv, "dh")) != EOF)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;
        case 'h': {     // help - show test names
            cerr << "Usage: regexT has the following tests:" << endl;
            const std::vector<Test*> &tests = regexT::suite()->getTests();
            unsigned int prefix_len = regexT::suite()->getName().append("::").length();
            for (std::vector<Test*>::const_iterator i = tests.begin(), e = tests.end(); i != e; ++i) {
                cerr << (*i)->getName().replace(0, prefix_len, "") << endl;
            }
            break;
        }
        default:
            break;
        }

    argc -= optind;
    argv += optind;

    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    bool wasSuccessful = true;
    string test = "";
    if (0 == argc) {
        // run them all
        wasSuccessful = runner.run("");
    }
    else {
        int i = 0;
        while (i < argc) {
            if (debug) cerr << "Running " << argv[i] << endl;
            test = regexT::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}

