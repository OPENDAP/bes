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

#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <list>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

#include "BESError.h"
#include "BESInternalError.h"
#include "BESUtil.h"
#include "TheBESKeys.h"
#include "test_config.h"

using namespace CppUnit;
using namespace std;

static bool debug = false;
static bool Debug = false;

#undef DBG
#define DBG(x)                                                                                                         \
    do {                                                                                                               \
        if (debug)                                                                                                     \
            (x);                                                                                                       \
    } while (false);
#define prolog std::string("utilT::").append(__func__).append("() - ")

class utilT : public TestFixture {
private:
public:
    utilT() = default;

    ~utilT() = default;

    void setUp() {
        if (debug)
            cerr << endl;
        string bes_conf = (string)TEST_SRC_DIR + "/bes.conf";
        TheBESKeys::ConfigFile = bes_conf;
    }

    void display_values(const list<string> &values) {
        for (auto &s : values)
            cerr << "  " << s << endl;
    }

    // string BESUtil::assemblePath(const string &firstPart, const string &secondPart, bool leadingSlash, bool
    // trailingSlash)
    void test_assemblePath_1() {
        string path = BESUtil::assemblePath("/first_part", "/second_part", true, true);
        CPPUNIT_ASSERT(path == "/first_part/second_part/");
    }

    void test_assemblePath_2() {
        string path = BESUtil::assemblePath("/first_part/", "/second_part/", true, true);
        CPPUNIT_ASSERT(path == "/first_part/second_part/");
    }

    void test_assemblePath_3() {
        string path = BESUtil::assemblePath("first_part", "second_part", true, true);
        CPPUNIT_ASSERT(path == "/first_part/second_part/");
    }

    // Note that 'false' for leadingSlash means leave the initial character unaltered.
    // jhrg 1/27/22
    void test_assemblePath_4() {
        string path = BESUtil::assemblePath("/first_part", "/second_part", false, false);
        CPPUNIT_ASSERT_MESSAGE(string("path = ") + path, path == "/first_part/second_part");
    }

    void test_assemblePath_5() {
        string path = BESUtil::assemblePath("/first_part/", "/second_part/", false, false);
        CPPUNIT_ASSERT_MESSAGE(string("path = ") + path, path == "/first_part/second_part");
    }

    void test_assemblePath_6() {
        string path = BESUtil::assemblePath("first_part", "second_part", false, false);
        CPPUNIT_ASSERT_MESSAGE(string("path = ") + path, path == "first_part/second_part");
    }

    void test_assemblePath_7() {
        string path = BESUtil::assemblePath("/", "/", false, false);
        CPPUNIT_ASSERT_MESSAGE(string("path = ") + path, path == "/");
    }

    void test_assemblePath_8() {
        string path = BESUtil::assemblePath("/", "/", true, true);
        CPPUNIT_ASSERT_MESSAGE(string("path = ") + path, path == "/");
    }

    void test_assemblePath_9() {
        string path = BESUtil::assemblePath("", "", false, false);
        CPPUNIT_ASSERT_MESSAGE(string("path = ") + path, path == "");
    }

    void test_assemblePath_10() {
        string path = BESUtil::assemblePath("", "", true, true);
        CPPUNIT_ASSERT_MESSAGE(string("path = ") + path, path == "/");
    }

    void test_unescape() {
        string s = BESUtil::unescape("\\\"This is a test, this is \\\"ONLY\\\" a test\\\"");
        string result = "\"This is a test, this is \"ONLY\" a test\"";
        CPPUNIT_ASSERT(s == result);
    }

    void test_removeLeadingAndTrailingBlanks_1() {
        string s = "This is a test";
        BESUtil::removeLeadingAndTrailingBlanks(s);
        CPPUNIT_ASSERT(s == "This is a test");
    }

    void test_removeLeadingAndTrailingBlanks_2() {
        string s = " This is a test";
        BESUtil::removeLeadingAndTrailingBlanks(s);
        CPPUNIT_ASSERT(s == "This is a test");
    }

    void test_removeLeadingAndTrailingBlanks_3() {
        string s = "	This is a test";
        BESUtil::removeLeadingAndTrailingBlanks(s);
        CPPUNIT_ASSERT(s == "This is a test");
    }

    void test_removeLeadingAndTrailingBlanks_4() {
        string s = "This is a test ";
        BESUtil::removeLeadingAndTrailingBlanks(s);
        CPPUNIT_ASSERT(s == "This is a test");
    }

    void test_removeLeadingAndTrailingBlanks_5() {
        string s = "This is a test    ";
        BESUtil::removeLeadingAndTrailingBlanks(s);
        CPPUNIT_ASSERT(s == "This is a test");
    }

    void test_removeLeadingAndTrailingBlanks_6() {
        string s = " This is a test ";
        BESUtil::removeLeadingAndTrailingBlanks(s);
        CPPUNIT_ASSERT(s == "This is a test");
    }

    void test_removeLeadingAndTrailingBlanks_7() {
        string s = "    	This is a test    	";
        BESUtil::removeLeadingAndTrailingBlanks(s);
        CPPUNIT_ASSERT(s == "This is a test");
    }

    void test_removeLeadingAndTrailingBlanks_8() {
        string s = " 	This is a test 	\n";
        BESUtil::removeLeadingAndTrailingBlanks(s);
        CPPUNIT_ASSERT(s == "This is a test");
    }

    void test_removeLeadingAndTrailingBlanks_9() {
        string s = "    ";
        BESUtil::removeLeadingAndTrailingBlanks(s);
        CPPUNIT_ASSERT(s == "");
    }

    void test_removeLeadingAndTrailingBlanks_10() {
        string s = "";
        BESUtil::removeLeadingAndTrailingBlanks(s);
        CPPUNIT_ASSERT(s == "");
    }

    void test_removeLeadingAndTrailingBlanks_11() {
        string s = " ";
        BESUtil::removeLeadingAndTrailingBlanks(s);
        CPPUNIT_ASSERT(s == "");
    }

    void test_explode_1() {
        list<string> values;
        string value = "val1,val2,val3,val4";
        DBG(cerr << value << endl);
        BESUtil::explode(',', value, values);
        DBG(display_values(values));
        CPPUNIT_ASSERT(values.size() == 4);

        list<string>::iterator i = values.begin();
        CPPUNIT_ASSERT((*i++) == "val1");
        CPPUNIT_ASSERT((*i++) == "val2");
        CPPUNIT_ASSERT((*i++) == "val3");
        CPPUNIT_ASSERT((*i) == "val4");
    }

    void test_explode_2() {
        list<string> values;
        string value = "val1,val2,val3,val4,";
        DBG(cerr << value << endl);
        BESUtil::explode(',', value, values);
        DBG(display_values(values));
        CPPUNIT_ASSERT(values.size() == 5);

        list<string>::iterator i = values.begin();
        CPPUNIT_ASSERT((*i++) == "val1");
        CPPUNIT_ASSERT((*i++) == "val2");
        CPPUNIT_ASSERT((*i++) == "val3");
        CPPUNIT_ASSERT((*i++) == "val4");
        CPPUNIT_ASSERT((*i) == "");
    }

    void test_explode_3() {
        list<string> values;
        string value = "val1;\"val2 with quotes\";val3;\"val4 with quotes\"";
        DBG(cerr << value << endl);
        BESUtil::explode(';', value, values);
        DBG(display_values(values));
        CPPUNIT_ASSERT(values.size() == 4);

        list<string>::iterator i = values.begin();
        CPPUNIT_ASSERT((*i++) == "val1");
        CPPUNIT_ASSERT((*i++) == "\"val2 with quotes\"");
        CPPUNIT_ASSERT((*i++) == "val3");
        CPPUNIT_ASSERT((*i++) == "\"val4 with quotes\"");
    }

    void test_explode_4() {
        list<string> values;
        string value = "val1;\"val2 with \\\"embedded quotes\\\"\";val3;\"val4 with quotes\";";
        DBG(cerr << value << endl);
        BESUtil::explode(';', value, values);
        DBG(display_values(values));
        CPPUNIT_ASSERT(values.size() == 5);

        list<string>::iterator i = values.begin();
        CPPUNIT_ASSERT((*i++) == "val1");
        CPPUNIT_ASSERT((*i++) == "\"val2 with \\\"embedded quotes\\\"\"");
        CPPUNIT_ASSERT((*i++) == "val3");
        CPPUNIT_ASSERT((*i++) == "\"val4 with quotes\"");
        CPPUNIT_ASSERT((*i++) == "");
    }

    void test_implode_1() {
        list<string> values;
        values.push_back("a");
        values.push_back("b");
        values.push_back("c");
        values.push_back("d");
        CPPUNIT_ASSERT(BESUtil::implode(values, ',') == "a,b,c,d");
    }

    void test_implode_2() {
        list<string> values;
        values.push_back("a");
        values.push_back("b");
        values.push_back("c");
        values.push_back("d");
        values.push_back("a,b");
        CPPUNIT_ASSERT_THROW_MESSAGE("imploding of value with comma should throw BESError",
                                     BESUtil::implode(values, ','), BESError);
    }

    void test_implode_3() {
        list<string> values;
        values.push_back("a");
        values.push_back("\"a,b\"");
        values.push_back("b");
        CPPUNIT_ASSERT(BESUtil::implode(values, ',') == "a,\"a,b\",b");
    }

    void test_trim_if_trailing_slash_1() {
        string s = "test";
        BESUtil::trim_if_trailing_slash(s);
        CPPUNIT_ASSERT(s == "test");
    }

    void test_trim_if_trailing_slash_2() {
        string s = "test/";
        BESUtil::trim_if_trailing_slash(s);
        CPPUNIT_ASSERT(s == "test");
    }

    void test_trim_if_trailing_slash_3() {
        string s = "test//";
        BESUtil::trim_if_trailing_slash(s);
        CPPUNIT_ASSERT(s == "test/");
    }

    void test_trim_if_trailing_slash_4() {
        string s = "/test/";
        BESUtil::trim_if_trailing_slash(s);
        CPPUNIT_ASSERT(s == "/test");
    }

    void test_trim_if_trailing_slash_5() {
        string s = "";
        BESUtil::trim_if_trailing_slash(s);
        CPPUNIT_ASSERT(s == "");
    }

    void test_trim_if_surrounding_quotes_1() {
        string s = "";
        BESUtil::trim_if_surrounding_quotes(s);
        CPPUNIT_ASSERT(s == "");
    }

    void test_trim_if_surrounding_quotes_2() {
        string s = "\"test\"";
        BESUtil::trim_if_surrounding_quotes(s);
        CPPUNIT_ASSERT(s == "test");
    }

    void test_trim_if_surrounding_quotes_3() {
        string s = "test\"";
        BESUtil::trim_if_surrounding_quotes(s);
        CPPUNIT_ASSERT(s == "test");
    }

    void test_trim_if_surrounding_quotes_4() {
        string s = "\"test";
        BESUtil::trim_if_surrounding_quotes(s);
        CPPUNIT_ASSERT(s == "test");
    }

    void test_trim_if_surrounding_quotes_5() {
        string s = "test";
        BESUtil::trim_if_surrounding_quotes(s);
        CPPUNIT_ASSERT(s == "test");
    }

    void replace_all_worker(const string &id, string &source_str, const string &template_str, const string &replace_str,
                            const string &baseline_str) {

        try {
            if (debug)
                cerr << id << "(" << prolog << ") " << "  Source String: '" << source_str << "'" << endl;
            if (debug)
                cerr << id << "(" << prolog << ") " << "Template String: '" << template_str << "'" << endl;
            if (debug)
                cerr << id << "(" << prolog << ") " << " Replace String: '" << replace_str << "'" << endl;
            if (debug)
                cerr << id << "(" << prolog << ") " << "       Expected: '" << baseline_str << "'" << endl;

            unsigned int replace_count = BESUtil::replace_all(source_str, template_str, replace_str);
            bool result_matched = source_str == baseline_str;
            if (debug)
                cerr << id << "(" << prolog << ") " << "     Result(" << replace_count << "): '" << source_str << "'"
                     << endl;

            std::stringstream info_msg;
            info_msg << id << "(" << prolog << ") " << "The filtered string "
                     << (result_matched ? "MATCHED " : "DID NOT MATCH ") << "the baseline: " << endl;
            if (debug)
                cerr << info_msg.str();
            CPPUNIT_ASSERT_MESSAGE(info_msg.str(), result_matched);

        } catch (BESError &be) {
            std::stringstream msg;
            msg << prolog << "Caught BESError. Message: " << be.get_verbose_message() << " ";
            msg << be.get_file() << " " << be.get_line();
            if (debug)
                cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }
    }

    /**
     * Test replace_all() string function
     */
    void replace_all_test_01() {
        if (debug)
            cerr << prolog << "BEGIN" << endl;
        string source_str =
            "aabaabaabbbbaaaaaabbabbbaaabbbaaaaabbabaabbabbaaabbaabbaaabejklgvaxxxaccaxxxacccaccaaacaabo";
        string template_str = "aab";
        string replace_str = "###";
        string baseline_str =
            "#########bbbaaaa###babbba###bbaaa###bab###babba###b###ba###ejklgvaxxxaccaxxxacccaccaaac###o";

        replace_all_worker(prolog, source_str, template_str, replace_str, baseline_str);

        if (debug)
            cerr << prolog << "END" << endl;
    }

    void replace_all_test_02() {
        if (debug)
            cerr << prolog << "BEGIN" << endl;
        string source_str =
            "#########bbbaaaa###babbba###bbaaa###bab###babba###b###ba###ejklgvaxxxaccaxxxacccaccaaac###o";
        string template_str = "#";
        string replace_str = "";
        string baseline_str = "bbbaaaababbbabbaaababbabbabbaejklgvaxxxaccaxxxacccaccaaaco";

        replace_all_worker(prolog, source_str, template_str, replace_str, baseline_str);

        if (debug)
            cerr << prolog << "END" << endl;
    }

    void replace_all_test_03() {
        if (debug)
            cerr << prolog << "BEGIN" << endl;
        string source_str = "The quick brown fox jumped over the lazy dog.";
        string template_str = "quick brown fox";
        string replace_str = "grasshopper";
        string baseline_str = "The grasshopper jumped over the lazy dog.";

        replace_all_worker(prolog, source_str, template_str, replace_str, baseline_str);

        if (debug)
            cerr << prolog << "END" << endl;
    }

    void replace_all_test_04() {
        if (debug)
            cerr << prolog << "BEGIN" << endl;
        string source_str = "The quick brown fox jumped over the lazy dog.";
        string template_str = "quick brown fox";
        string replace_str = "reckless skateboarder";
        string baseline_str = "The reckless skateboarder jumped over the lazy dog.";

        replace_all_worker(prolog, source_str, template_str, replace_str, baseline_str);

        if (debug)
            cerr << prolog << "END" << endl;
    }

    // string_to_file(const string &filename, const string &content)
    void string_to_file_and_file_to_string_test() {
        BESUtil::string_to_file("test.txt", "This is a test");
        string info = BESUtil::file_to_string("test.txt");
        CPPUNIT_ASSERT(info == "This is a test");
        system("rm test.txt"); // cleanup
    }

    void string_to_file_test_empty_name() {
        CPPUNIT_ASSERT_THROW_MESSAGE("Expected an exception", BESUtil::string_to_file("", "This is a test"),
                                     BESInternalError);
    }

    void string_to_file_test_not_allowed() {
        CPPUNIT_ASSERT_THROW_MESSAGE("Expected an exception", BESUtil::string_to_file("/", "This is a test"),
                                     BESInternalError);
    }

    void file_to_string_test_empty_name() {
        CPPUNIT_ASSERT_THROW_MESSAGE("Expected an exception", BESUtil::file_to_string(""), BESInternalError);
    }

    void file_to_string_test_not_allowed() {
        CPPUNIT_ASSERT_THROW_MESSAGE("Expected an exception", BESUtil::file_to_string("/tmp/no_such_file"),
                                     BESInternalError);
    }

    // BESUtil::make_temp_file(const string &temp_file_dir, string &temp_file_name)
    void make_temp_file_test() {
        string temp_file_name;
        BESUtil::make_temp_file("/tmp", temp_file_name);
        CPPUNIT_ASSERT(temp_file_name.find("/tmp/") == 0);
        CPPUNIT_ASSERT(temp_file_name.find("bes_util_") == 5);
        CPPUNIT_ASSERT(temp_file_name.length() == 20);
        CPPUNIT_ASSERT_MESSAGE("Temporary file should exist", access(temp_file_name.c_str(), F_OK) != -1);
        system(("rm " + temp_file_name).c_str()); // cleanup
    }

    // Test that when the temp file is written to, the int file descriptor is still valid and
    // does not move from the start of the file. jhrg 3/9/23
    void make_temp_file_write_data_test() {
        string temp_file_name;
        int fd = BESUtil::make_temp_file("/tmp", temp_file_name);
        CPPUNIT_ASSERT_MESSAGE("Temporary file should exist", access(temp_file_name.c_str(), F_OK) != -1);
        BESUtil::string_to_file(temp_file_name, "This is a test");
        CPPUNIT_ASSERT_MESSAGE("Temporary file should exist", fcntl(fd, F_GETFD) != -1);
        unsigned long pos = lseek(fd, 0, SEEK_CUR);
        CPPUNIT_ASSERT_MESSAGE("The fd should be at the start of the file", pos == 0);
        system(("rm " + temp_file_name).c_str()); // cleanup
    }

    // Test that we can write and then read. jhrg 3/9/23
    void make_temp_file_read_data_test() {
        string temp_file_name;
        int fd = BESUtil::make_temp_file("/tmp", temp_file_name);
        CPPUNIT_ASSERT_MESSAGE("Temporary file should exist", access(temp_file_name.c_str(), F_OK) != -1);
        string data = "This is a test";
        BESUtil::string_to_file(temp_file_name, data);
        CPPUNIT_ASSERT_MESSAGE("The fd should be open", fcntl(fd, F_GETFD) != -1);
        unsigned long pos = lseek(fd, 0, SEEK_CUR);
        CPPUNIT_ASSERT_MESSAGE("The fd should be at the start of the file", pos == 0);
        char *buf = new char[data.length() + 1];
        memset(buf, 0, data.length() + 1);
        read(fd, buf, data.length());
        string read_data = buf;
        CPPUNIT_ASSERT(read_data == data);
        system(("rm " + temp_file_name).c_str()); // cleanup
    }

    void make_temp_file_write_data_with_truncate_test() {
        string temp_file_name;
        int fd = BESUtil::make_temp_file("/tmp", temp_file_name);
        CPPUNIT_ASSERT_MESSAGE("Temporary file should exist", access(temp_file_name.c_str(), F_OK) != -1);
        string data = "This is a test";
        BESUtil::string_to_file(temp_file_name, data);
        string new_data = "test test test tes test";
        BESUtil::string_to_file(temp_file_name, new_data);

        // After the second write, which truncates the file, the fd should (still) be at the start of the file
        CPPUNIT_ASSERT_MESSAGE("The fd should be open", fcntl(fd, F_GETFD) != -1);
        unsigned long pos = lseek(fd, 0, SEEK_CUR);
        CPPUNIT_ASSERT_MESSAGE("The fd should be at the start of the file", pos == 0);

        // And the new info should be in the file
        CPPUNIT_ASSERT(new_data == BESUtil::file_to_string(temp_file_name));
        system(("rm " + temp_file_name).c_str()); // cleanup
    }

    void mkdir_p_simple_test() {
        string dir = "/tmp/bes_util_test";
        // initial condition: bes_util_test does not exist
        CPPUNIT_ASSERT_MESSAGE("bes_util_test should not exist", access(dir.c_str(), F_OK) == -1);
        BESUtil::mkdir_p(dir, 0775);
        CPPUNIT_ASSERT_MESSAGE("Directory should exist", access(dir.c_str(), F_OK) != -1);
        system(("rm -rf " + dir).c_str()); // cleanup
    }

    void mkdir_p_w_missing_parent_test() {
        string dir = "/tmp/bes_util_test/parent";
        // initial condition: neither bes_util_test nor parent exists
        CPPUNIT_ASSERT_MESSAGE("Neither bes_util_test nor parent should exist",
                               access("/tmp/bes_util_test", F_OK) == -1 && access(dir.c_str(), F_OK) == -1);
        BESUtil::mkdir_p(dir, 0775);
        CPPUNIT_ASSERT_MESSAGE("Directory should exist", access(dir.c_str(), F_OK) != -1);
        system("rm -rf /tmp/bes_util_test"); // cleanup
    }

    void mkdir_p_exists_test() {
        string dir = "/tmp";
        // Unlike mkdir(2), mkdir_p returns 0 (success) if the directory exists.
        int status = BESUtil::mkdir_p(dir, 0775);
        CPPUNIT_ASSERT_MESSAGE("Call to make an existing dir should fail", status == 0);
    }

    void mkdir_p_exists_trailing_slash_test() {
        string dir = "/tmp/";
        // Unlike mkdir(2), mkdir_p returns 0 (success) if the directory exists.
        int status = BESUtil::mkdir_p(dir, 0775);
        CPPUNIT_ASSERT_MESSAGE("Call to make an existing dir should fail", status == 0);
    }

    void mkdir_p_w_missing_parent_trailing_slash_test() {
        string dir = "/tmp/bes_util_test/parent/";
        // initial condition: neither bes_util_test nor parent exists
        CPPUNIT_ASSERT_MESSAGE("Neither bes_util_test nor parent should exist",
                               access("/tmp/bes_util_test", F_OK) == -1 && access(dir.c_str(), F_OK) == -1);
        BESUtil::mkdir_p(dir, 0775);
        CPPUNIT_ASSERT_MESSAGE("Directory should exist", access(dir.c_str(), F_OK) != -1);
        system("rm -rf /tmp/bes_util_test"); // cleanup
    }

    void mkdir_p_not_allowed_test() {
        string dir = "/dev/mkdir_p_not_allowed";
        // initial conditions: /dev/mkdir_p_not_allowed does not exist and we cannot write to /dev
        CPPUNIT_ASSERT_MESSAGE("/dev/mkdir_p_not_allowed should not exist", access(dir.c_str(), F_OK) == -1);
        CPPUNIT_ASSERT_MESSAGE("/dev not writable by this process", access("/dev", W_OK) == -1);
        int status = BESUtil::mkdir_p(dir, 0775);
        CPPUNIT_ASSERT_MESSAGE("mkdir_p() should return an error", status == -1);
        DBG(cerr << "errno: " << strerror(errno) << " (" << errno << ")" << endl);
        CPPUNIT_ASSERT_MESSAGE("Error code should be EACCES or EPERM", errno == EACCES || errno == EPERM);
        CPPUNIT_ASSERT_MESSAGE("Directory should not exist because we are not allowed to write there",
                               access(dir.c_str(), F_OK) == -1);
    }

    void mkdir_p_empty_path_test() {
        string dir = "";
        int status = BESUtil::mkdir_p(dir, 0775);
        CPPUNIT_ASSERT_MESSAGE("An empty path will succeed", status == 0);
    }

    CPPUNIT_TEST_SUITE(utilT);

    CPPUNIT_TEST(mkdir_p_simple_test);
    CPPUNIT_TEST(mkdir_p_w_missing_parent_test);
    CPPUNIT_TEST(mkdir_p_exists_test);
    CPPUNIT_TEST(mkdir_p_exists_trailing_slash_test);
    CPPUNIT_TEST(mkdir_p_w_missing_parent_trailing_slash_test);
    CPPUNIT_TEST(mkdir_p_not_allowed_test);
    CPPUNIT_TEST(mkdir_p_empty_path_test);

    CPPUNIT_TEST(make_temp_file_test);
    CPPUNIT_TEST(make_temp_file_write_data_test);
    CPPUNIT_TEST(make_temp_file_read_data_test);
    CPPUNIT_TEST(make_temp_file_write_data_with_truncate_test);

    CPPUNIT_TEST(string_to_file_and_file_to_string_test);
    CPPUNIT_TEST(string_to_file_test_empty_name);
    CPPUNIT_TEST(string_to_file_test_not_allowed);
    CPPUNIT_TEST(file_to_string_test_empty_name);
    CPPUNIT_TEST(file_to_string_test_not_allowed);

    CPPUNIT_TEST(test_assemblePath_1);
    CPPUNIT_TEST(test_assemblePath_2);
    CPPUNIT_TEST(test_assemblePath_3);
    CPPUNIT_TEST(test_assemblePath_4);
    CPPUNIT_TEST(test_assemblePath_5);
    CPPUNIT_TEST(test_assemblePath_6);
    CPPUNIT_TEST_FAIL(test_assemblePath_7);
    CPPUNIT_TEST(test_assemblePath_8);
    CPPUNIT_TEST(test_assemblePath_9);
    CPPUNIT_TEST(test_assemblePath_10);

    CPPUNIT_TEST(test_unescape);

    CPPUNIT_TEST(test_removeLeadingAndTrailingBlanks_1);
    CPPUNIT_TEST(test_removeLeadingAndTrailingBlanks_2);
    CPPUNIT_TEST(test_removeLeadingAndTrailingBlanks_3);
    CPPUNIT_TEST(test_removeLeadingAndTrailingBlanks_4);
    CPPUNIT_TEST(test_removeLeadingAndTrailingBlanks_5);
    CPPUNIT_TEST(test_removeLeadingAndTrailingBlanks_6);
    CPPUNIT_TEST(test_removeLeadingAndTrailingBlanks_7);
    CPPUNIT_TEST(test_removeLeadingAndTrailingBlanks_8);
    CPPUNIT_TEST(test_removeLeadingAndTrailingBlanks_9);
    CPPUNIT_TEST(test_removeLeadingAndTrailingBlanks_10);
    CPPUNIT_TEST(test_removeLeadingAndTrailingBlanks_11);

    CPPUNIT_TEST(test_explode_1);
    CPPUNIT_TEST(test_explode_2);
    CPPUNIT_TEST(test_explode_3);
    CPPUNIT_TEST(test_explode_4);

    CPPUNIT_TEST(test_implode_1);
    CPPUNIT_TEST(test_implode_2);
    CPPUNIT_TEST(test_implode_3);

    CPPUNIT_TEST(test_trim_if_trailing_slash_1);
    CPPUNIT_TEST(test_trim_if_trailing_slash_2);
    CPPUNIT_TEST(test_trim_if_trailing_slash_3);
    CPPUNIT_TEST(test_trim_if_trailing_slash_4);
    CPPUNIT_TEST(test_trim_if_trailing_slash_5);

    CPPUNIT_TEST(test_trim_if_surrounding_quotes_1);
    CPPUNIT_TEST(test_trim_if_surrounding_quotes_2);
    CPPUNIT_TEST(test_trim_if_surrounding_quotes_3);
    CPPUNIT_TEST(test_trim_if_surrounding_quotes_4);
    CPPUNIT_TEST(test_trim_if_surrounding_quotes_5);

    CPPUNIT_TEST(replace_all_test_01);
    CPPUNIT_TEST(replace_all_test_02);
    CPPUNIT_TEST(replace_all_test_03);
    CPPUNIT_TEST(replace_all_test_04);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(utilT);

int main(int argc, char *argv[]) {
    int option_char;
    while ((option_char = getopt(argc, argv, "dDh")) != EOF)
        switch (option_char) {
        case 'd':
            debug = true; // debug is a static global
            break;
        case 'D':
            Debug = true; // debug is a static global
            break;
        case 'h': { // help - show test names
            cerr << "Usage: utilT has the following tests:" << endl;
            const std::vector<Test *> &tests = utilT::suite()->getTests();
            unsigned long prefix_len = utilT::suite()->getName().append("::").size();
            for (auto &test : tests) {
                cerr << test->getName().replace(0, prefix_len, "") << endl;
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
    if (0 == argc) {
        // run them all
        wasSuccessful = runner.run("");
    } else {
        int i = 0;
        while (i < argc) {
            if (debug)
                cerr << "Running " << argv[i] << endl;
            wasSuccessful = wasSuccessful && runner.run(utilT::suite()->getName().append("::").append(argv[i]));
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
