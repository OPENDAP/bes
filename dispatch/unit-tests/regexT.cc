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

// #include <unistd.h>
#include <chrono>
#include <iostream>

#include "BESError.h"
#include "BESRegex.h"

#include "modules/common/run_tests_cppunit.h"

using namespace CppUnit;
using namespace std;

class regexT : public TestFixture {

public:
    regexT() = default;
    ~regexT() override = default;

    CPPUNIT_TEST_SUITE(regexT);

    CPPUNIT_TEST(digit_test_1);
    CPPUNIT_TEST(digit_test_2);
    CPPUNIT_TEST(digit_test_3);
    CPPUNIT_TEST(digit_test_4);
    CPPUNIT_TEST(begin_end_test_1);
    CPPUNIT_TEST(begin_end_test_2);
    CPPUNIT_TEST(char_class_test_1);
    CPPUNIT_TEST(char_class_test_2);
    CPPUNIT_TEST(alternatives_test_1);
    CPPUNIT_TEST(alternatives_test_2);

#if INCLUDE_REGEX_TIMING || true
    // By default, don't run this wiht every build. THe short answer is that
    // 'const' alone is not a big win with regexes. For the best performance,
    // the BESRegex objects need to be factored out of code they are built as
    // infrequently as possible. jhrg 12/8/21
    CPPUNIT_TEST(const_test);
#endif
    CPPUNIT_TEST_SUITE_END();

    void digit_test_1() {
        try {
            DBG(cerr << "*****************************************" << endl);
            DBG(cerr << "Match reg ex 123456 against string 01234567" << endl);
            BESRegex reg_expr("123456");
            string inQuestion = "01234567";
            int result = reg_expr.match(inQuestion.c_str(), (int)inQuestion.size());
            DBG(cerr << "result = " << result << endl);
            CPPUNIT_ASSERT(result == 6);
        } catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT(!"Failed to match");
        }
    }

    void digit_test_2() {
        try {
            DBG(cerr << "*****************************************" << endl);
            DBG(cerr << "Match reg ex 123456 against string 01234567" << endl);
            BESRegex reg_expr("9");
            string inQuestion = "01234567";
            int result = reg_expr.match(inQuestion.c_str(), (int)inQuestion.size());
            DBG(cerr << "result = " << result << endl);
            CPPUNIT_ASSERT(result == -1);
        } catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT(!"Failed to match");
        }
    }

    void digit_test_3() {
        try {
            DBG(cerr << "*****************************************" << endl);
            DBG(cerr << "Match reg ex 123456 against string 01234567" << endl);
            BESRegex reg_expr("123456");
            string inQuestion = "01234567";
            int match_len = 0;
            int pos = reg_expr.search(inQuestion, match_len);
            DBG(cerr << "position = " << pos << ", match len: " << match_len << endl);
            CPPUNIT_ASSERT(pos == 1);
            CPPUNIT_ASSERT(match_len == 6);
        } catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT(!"Failed to match");
        }
    }

    void digit_test_4() {
        try {
            DBG(cerr << "*****************************************" << endl);
            DBG(cerr << "Match reg ex 123456 against string 01234567" << endl);
            BESRegex reg_expr("123456");
            string inQuestion = "01091234567";
            int match_len = 0;
            int pos = reg_expr.search(inQuestion, match_len);
            DBG(cerr << "position = " << pos << ", match len: " << match_len << endl);
            CPPUNIT_ASSERT(pos == 4);
            CPPUNIT_ASSERT(match_len == 6);
        } catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT(!"Failed to match");
        }
    }

    void begin_end_test_1() {
        try {
            DBG(cerr << "*****************************************" << endl);
            DBG(cerr << "Match reg ex ^123456$ against string 01234567" << endl);
            BESRegex reg_expr("^123456$");
            string inQuestion = "01234567";
            int result = reg_expr.match(inQuestion.c_str(), (int)inQuestion.size());
            CPPUNIT_ASSERT(result == -1);
        } catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT(!"Failed to match");
        }
    }

    void begin_end_test_2() {
        try {
            DBG(cerr << "*****************************************" << endl);
            DBG(cerr << "Match reg ex ^123456$ against string 123456" << endl);
            DBG(cerr << "    besregtest include \"^123456$;\" 123456 matches all 6 of 6 characters" << endl);
            BESRegex reg_expr("^123456$");
            string inQuestion = "123456";
            int result = reg_expr.match(inQuestion.c_str(), inQuestion.size());
            DBG(cerr << "result = " << result << endl);
            CPPUNIT_ASSERT(result == 6);
        } catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT(!"Failed to match");
        }
    }

    void char_class_test_1() {
        try {
            DBG(cerr << "*****************************************" << endl);
            DBG(cerr << R"(Match reg ex ".*\.nc$;" against string fnoc1.nc)" << endl);
            BESRegex reg_expr(".*\\.nc$");
            string inQuestion = "fnoc1.nc";
            int result = reg_expr.match(inQuestion.c_str(), (int)inQuestion.size());
            DBG(cerr << "result = " << result << endl);
            CPPUNIT_ASSERT(result == 8);
        } catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT(!"Failed to match");
        }
    }

    void char_class_test_2() {
        try {
            DBG(cerr << "*****************************************" << endl);
            DBG(cerr << "Match reg ex \".*\\.nc$;\" against string fnoc1.ncd" << endl);
            BESRegex reg_expr(".*\\.nc$");
            string inQuestion = "fnoc1.ncd";
            int result = reg_expr.match(inQuestion.c_str(), (int)inQuestion.size());
            CPPUNIT_ASSERT(result == -1);
        } catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT(!"Failed to match");
        }
    }

    void alternatives_test_1() {
        try {
            DBG(cerr << "*****************************************" << endl);
            DBG(cerr << "Match reg ex .*\\.(nc|NC)(\\.gz|\\.bz2|\\.Z)?$ against string fnoc1.nc" << endl);
            BESRegex reg_expr(".*\\.(nc|NC)(\\.gz|\\.bz2|\\.Z)?$");
            string inQuestion = "fnoc1.nc";
            int result = reg_expr.match(inQuestion.c_str(), (int)inQuestion.size());
            CPPUNIT_ASSERT(result == 8);
        } catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT(!"Failed to match");
        }
    }

    void alternatives_test_2() {
        try {
            DBG(cerr << "*****************************************" << endl);
            DBG(cerr << "Match reg ex .*\\.(nc|NC)(\\.gz|\\.bz2|\\.Z)?$ against string fnoc1.nc.gz" << endl);
            BESRegex reg_expr(".*\\.(nc|NC)(\\.gz|\\.bz2|\\.Z)?$");
            string inQuestion = "fnoc1.nc.gz";
            int result = reg_expr.match(inQuestion.c_str(), (int)inQuestion.size());
            CPPUNIT_ASSERT(result == 11);
        } catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT(!"Failed to match");
        }
    }

    // We want to know if it's necessary to factor a regex out of a loop or if
    // adding 'const' alone will speed up runtimes.
    // Answer: Adding const alone does speed up runtimes, but for a complex regex,
    // not nearly as much as factoring the regex out of a loop. Thus, if we have
    // regexes that are called repeatedly without changing, those should be stored
    // in objects as regexes (compiled) and not strings. jhrg 12/8/21
    //
    // This code should not be running by default.
    void const_test() {
        using namespace std::chrono;

        // A simple regex
        string simple_regex = R"(T|t.*)";

        DBG(cerr << "Using a simple regex:" << endl);
        test_regex_times(simple_regex, "The answer.", "the answer");

        // This is a complex regex.
        string s3_path_regex_str =
            R"(^https?:\/\/s3((\.|-)us-(east|west)-(1|2))?\.amazonaws\.com\/([a-z]|[0-9])(([a-z]|[0-9]|\.|-){1,61})([a-z]|[0-9])\/.*$)";
        string test1 = "https://s3-us-east-1.amazonaws.com/aa.a/etc";
        string test2 = "https://s3-us-west-1.amazonaws.com/aa.a/etc";

        DBG(cerr << "Using a complex regex:" << endl);
        test_regex_times(s3_path_regex_str, test1, test2);
    }

    void test_regex_times(const string &s3_path_regex_str, const string &test1, const string &test2) const {
        chrono::steady_clock::time_point t1 = chrono::steady_clock::now();
        DBG(cerr << "Running 10 000 regex compiles..." << endl);
        for (int i = 0; i < 10000; ++i) {
            BESRegex r(s3_path_regex_str);
            int m1 = r.match(test1);
            int m2 = r.match(test2);
            CPPUNIT_ASSERT(m1 != -1 && m2 != -1);
        }
        chrono::steady_clock::time_point t2 = chrono::steady_clock::now();
        chrono::duration<double> time_span = chrono::duration_cast<chrono::duration<double>>(t2 - t1);
        DBG(cerr << "It took me " << time_span.count() << " seconds." << endl);

        t1 = chrono::steady_clock::now();
        DBG(cerr << "Running 10 000 regex compares with const Regexes..." << endl);
        for (int i = 0; i < 10000; ++i) {
            const BESRegex r2(s3_path_regex_str);
            int m1 = r2.match(test1);
            int m2 = r2.match(test2);
            CPPUNIT_ASSERT(m1 != -1 && m2 != -1);
        }
        t2 = chrono::steady_clock::now();
        time_span = chrono::duration_cast<chrono::duration<double>>(t2 - t1);
        DBG(cerr << "It took me " << time_span.count() << " seconds." << endl);

        t1 = chrono::steady_clock::now();
        DBG(cerr << "Running 10 000 regex compares with const Regexes moved out of the loop..." << endl);
        const BESRegex r3(s3_path_regex_str);
        for (int i = 0; i < 10000; ++i) {
            int m1 = r3.match(test1);
            int m2 = r3.match(test2);
            CPPUNIT_ASSERT(m1 != -1 && m2 != -1);
        }
        t2 = chrono::steady_clock::now();
        time_span = chrono::duration_cast<chrono::duration<double>>(t2 - t1);
        DBG(cerr << "It took me " << time_span.count() << " seconds." << endl);
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(regexT);

int main(int argc, char *argv[]) { return bes_run_tests<regexT>(argc, argv, "bes") ? 0 : 1; }
