// pfileT.C

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

// connT.cc - Refactored

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <iostream>
#include <string>   // Include <string> explicitly
#include <memory>   // Although not using unique_ptr for 'd', good practice
#include <vector>   // Needed for TestFactoryRegistry access if done manually
#include <cstdio>   // For sprintf (kept from original)
#include <cstdlib>  // For EXIT_SUCCESS in bes_run_tests

// Include necessary BES headers
#include "BESContainerStorageFile.h"
#include "BESContainer.h"
#include "BESError.h"
#include "BESTextInfo.h"
#include "TheBESKeys.h"
#include <test_config.h> // Assumed to define TEST_SRC_DIR

// *** Include the header defining bes_run_tests ***
// *** Make sure this path is correct for your build system ***
#include "modules/common/run_tests_cppunit.h"
// **************************************************

// Using directives - std namespace
using std::cerr;
using std::cout;
using std::endl;
using std::string;
// Using directives - CppUnit namespace
using CppUnit::TestFixture;

// The global 'debug', 'debug2', DBG, and DBG2 macros are now expected
// to be defined in "bes_run_tests_cppunit.h".

class connT final : public TestFixture {

public:
    connT() = default;  // Use default constructor
    ~connT() override = default; // Use default virtual destructor

    // setUp now configures the necessary keys before each test
    void setUp() override {
        const string bes_conf = string(TEST_SRC_DIR) + "/persistence_file_test.ini";
        TheBESKeys::ConfigFile = bes_conf;

        // Get singleton instance
        auto* keys = TheBESKeys::TheKeys(); // Use auto

        // Set up file paths for various test cases
        // Using string() constructor for clarity, though implicit conversion often works
        keys->set_key(string("BES.Container.Persistence.File.FileNot=") + TEST_SRC_DIR + "/persistence_filenot.txt");
        keys->set_key(string("BES.Container.Persistence.File.FileTooMany=") + TEST_SRC_DIR + "/persistence_file3.txt");
        keys->set_key(string("BES.Container.Persistence.File.FileTooFew=") + TEST_SRC_DIR + "/persistence_file4.txt");
        keys->set_key(string("BES.Container.Persistence.File.File1=") + TEST_SRC_DIR + "/persistence_file1.txt");
        keys->set_key(string("BES.Container.Persistence.File.File2=") + TEST_SRC_DIR + "/persistence_file2.txt");

        // Note: The key "BES.Container.Persistence.File.File" (used in the incomplete test)
        // is intentionally NOT fully set here, simulating the incomplete state.
    }

    // tearDown remains empty
    void tearDown() override {}

    // --- Test Suite Definition ---
    CPPUNIT_TEST_SUITE(connT);

    // Register the new, specific test methods
    CPPUNIT_TEST(testConstructor_IncompleteKeyInfo_Throws);
    CPPUNIT_TEST(testConstructor_NonExistentFile_Throws);
    CPPUNIT_TEST(testConstructor_FileTooManyColumns_Throws);
    CPPUNIT_TEST(testConstructor_FileTooFewColumns_Throws);
    CPPUNIT_TEST(testConstructor_ValidFile_Opens);
    CPPUNIT_TEST(testLookFor_FindsExistingContainers);
    CPPUNIT_TEST(testLookFor_NonExistentKey_ReturnsNull);
    CPPUNIT_TEST(testShowContainers_RunsWithoutError); // Basic check

    // CPPUNIT_TEST( do_test ); // Remove the old combined test

    CPPUNIT_TEST_SUITE_END();

    // --- Individual Test Methods ---

    /**
     * @brief Tests BESContainerStorageFile constructor with incomplete key info.
     */
    void testConstructor_IncompleteKeyInfo_Throws() {
        DBG(cout << "Running testConstructor_IncompleteKeyInfo_Throws" << endl);
        // Expecting BESError because "BES.Container.Persistence.File.File" key is not fully defined in setUp.
        CPPUNIT_ASSERT_THROW_MESSAGE(
            "Should throw BESError for incomplete key 'File'",
            BESContainerStorageFile cpf("File"), // Action that should throw
            BESError                       // Expected exception type
        );
    }

    /**
     * @brief Tests BESContainerStorageFile constructor with a non-existent file path.
     */
    void testConstructor_NonExistentFile_Throws() {
        DBG(cout << "Running testConstructor_NonExistentFile_Throws" << endl);
        // Expecting BESError because the file specified by "FileNot" key doesn't exist.
         CPPUNIT_ASSERT_THROW_MESSAGE(
            "Should throw BESError for non-existent file 'FileNot'",
            BESContainerStorageFile cpf("FileNot"),
            BESError
        );
    }

    /**
     * @brief Tests BESContainerStorageFile constructor with too many values on a line.
     */
    void testConstructor_FileTooManyColumns_Throws() {
         DBG(cout << "Running testConstructor_FileTooManyColumns_Throws" << endl);
         // Expecting BESError due to format error in the file specified by "FileTooMany".
         CPPUNIT_ASSERT_THROW_MESSAGE(
            "Should throw BESError for file with too many columns 'FileTooMany'",
            BESContainerStorageFile cpf("FileTooMany"),
            BESError
        );
    }

    /**
     * @brief Tests BESContainerStorageFile constructor with too few values on a line.
     */
    void testConstructor_FileTooFewColumns_Throws() {
        DBG(cout << "Running testConstructor_FileTooFewColumns_Throws" << endl);
        // Expecting BESError due to format error in the file specified by "FileTooFew".
        CPPUNIT_ASSERT_THROW_MESSAGE(
            "Should throw BESError for file with too few columns 'FileTooFew'",
            BESContainerStorageFile cpf("FileTooFew"),
            BESError
        );
    }

   /**
    * @brief Tests successful construction of BESContainerStorageFile with a valid file.
    */
    void testConstructor_ValidFile_Opens() {
        DBG(cout << "Running testConstructor_ValidFile_Opens" << endl);
        // Expecting no exception for a valid file configuration "File1".
        CPPUNIT_ASSERT_NO_THROW_MESSAGE(
            "Should successfully open valid file 'File1'",
            BESContainerStorageFile cpf("File1")
        );
    }

    /**
     * @brief Tests finding existing containers using look_for.
     */
    void testLookFor_FindsExistingContainers() {
        DBG(cout << "Running testLookFor_FindsExistingContainers" << endl);
        BESContainerStorageFile cpf("File1"); // Assumes File1 is valid (tested above)

        char s[10];
        char r[10];
        char c[10];
        for (int i = 1; i < 6; i++) {
            // Using sprintf as in original, consider std::ostringstream for more C++ style
            sprintf(s, "sym%d", i);
            sprintf(r, "real%d", i);
            sprintf(c, "type%d", i);
            DBG(cout << "    looking for " << s << endl);

            // look_for returns a non-owning raw pointer. Do NOT delete 'd'.
            BESContainer *d = cpf.look_for(s);

            CPPUNIT_ASSERT_MESSAGE(string("Should find container for key: ") + s, d != nullptr);
            if (d) { // Check pointer before dereferencing
                CPPUNIT_ASSERT_EQUAL_MESSAGE(string("Real name mismatch for key: ") + s, string(r), d->get_real_name());
                CPPUNIT_ASSERT_EQUAL_MESSAGE(string("Container type mismatch for key: ") + s, string(c), d->get_container_type());
            }
        }
    }

   /**
    * @brief Tests that look_for returns null for a non-existent key.
    */
    void testLookFor_NonExistentKey_ReturnsNull() {
        DBG(cout << "Running testLookFor_NonExistentKey_ReturnsNull" << endl);
        BESContainerStorageFile cpf("File1"); // Assumes File1 is valid

        DBG(cout << "    looking for notthere" << endl);
        BESContainer *d = cpf.look_for("notthere"); // Non-owning raw pointer

        CPPUNIT_ASSERT_MESSAGE("Should return nullptr for non-existent key 'notthere'", d == nullptr);
    }

    /**
     * @brief Basic test for show_containers - ensures it runs without throwing.
     * Note: This doesn't verify the output content.
     */
    void testShowContainers_RunsWithoutError() {
         DBG(cout << "Running testShowContainers_RunsWithoutError" << endl);
         BESContainerStorageFile cpf("File1"); // Assumes File1 is valid
         BESTextInfo info;

         CPPUNIT_ASSERT_NO_THROW_MESSAGE(
             "show_containers should run without throwing exceptions",
             cpf.show_containers(info)
         );
         // info.print(cout); // Optionally print for manual inspection if needed during debug
    }

}; // End class connT

// --- Test Suite Registration (Unchanged) ---
CPPUNIT_TEST_SUITE_REGISTRATION(connT);

// --- Main Function ---
// Uses the bes_run_tests template function from the included header.
int main(int argc, char *argv[]) {
    // Call the templated test runner function.
    // Pass the test fixture class (connT) as the template parameter.
    // Pass argc and argv directly.
    // Provide relevant BES debug contexts (e.g., "bes,container"). Adjust as needed.
    bool success = bes_run_tests<connT>(argc, argv, "bes,container");

    // Return 0 for success (true from bes_run_tests), 1 for failure (false).
    return success ? 0 : 1;
}
//------------------------------ old
#if 0
#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

using namespace CppUnit;

#include <iostream>
#include <cstdlib>

using std::cerr;
using std::cout;
using std::endl;
using std::string;

#include "BESContainerStorageFile.h"
#include "BESContainer.h"
#include "BESError.h"
#include "BESTextInfo.h"
#include "TheBESKeys.h"
#include <test_config.h>
#include <unistd.h>

static bool debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

class connT: public TestFixture {
private:

public:
    connT()
    {
    }
    ~connT()
    {
    }

    void setUp()
    {
        string bes_conf = (string) TEST_SRC_DIR + "/persistence_file_test.ini";
        TheBESKeys::ConfigFile = bes_conf;
    }

    void tearDown()
    {
    }

    CPPUNIT_TEST_SUITE( connT );

    CPPUNIT_TEST( do_test );

    CPPUNIT_TEST_SUITE_END();

    void do_test()
    {
        TheBESKeys *keys = TheBESKeys::TheKeys();
        keys->set_key((string) "BES.Container.Persistence.File.FileNot=" + TEST_SRC_DIR + "/persistence_filenot.txt");
        keys->set_key((string) "BES.Container.Persistence.File.FileTooMany=" + TEST_SRC_DIR + "/persistence_file3.txt");
        keys->set_key((string) "BES.Container.Persistence.File.FileTooFew=" + TEST_SRC_DIR + "/persistence_file4.txt");
        keys->set_key((string) "BES.Container.Persistence.File.File1=" + TEST_SRC_DIR + "/persistence_file1.txt");
        keys->set_key((string) "BES.Container.Persistence.File.File2=" + TEST_SRC_DIR + "/persistence_file2.txt");

        cout << "*****************************************" << endl;
        cout << "Entered pfileT::run" << endl;

        cout << "*****************************************" << endl;
        cout << "Open File, incomplete key information" << endl;
        try {
            BESContainerStorageFile cpf("File");
            CPPUNIT_ASSERT( !"opened file File, shouldn't have" );
        }
        catch (BESError &ex) {
            cout << "couldn't get File, good, because" << endl;
            cout << ex.get_message() << endl;
        }

        cout << "*****************************************" << endl;
        cout << "Open FileNot, doesn't exist" << endl;
        try {
            BESContainerStorageFile cpf("FileNot");
            CPPUNIT_ASSERT( !"opened file FileNot, shouldn't have" );
        }
        catch (BESError &ex) {
            cout << "couldn't get FileNot, good, because" << endl;
            cout << ex.get_message() << endl;
        }

        cout << "*****************************************" << endl;
        cout << "Open FileTooMany, too many values on line" << endl;
        try {
            BESContainerStorageFile cpf("FileTooMany");
            CPPUNIT_ASSERT( ! "opened file FileTooMany, shouldn't have" );
        }
        catch (BESError &ex) {
            cout << "couldn't get FileTooMany, good, because" << endl;
            cout << ex.get_message() << endl;
        }

        cout << "*****************************************" << endl;
        cout << "Open FileTooFew, too few values on line" << endl;
        try {
            BESContainerStorageFile cpf("FileTooFew");
            CPPUNIT_ASSERT( !"opened file FileTooFew, shouldn't have" );
        }
        catch (BESError &ex) {
            cout << "couldn't get FileTooFew, good, because" << endl;
            cout << ex.get_message() << endl;
        }

        cout << "*****************************************" << endl;
        cout << "Open File1" << endl;
        try {
            BESContainerStorageFile cpf("File1");
        }
        catch (BESError &ex) {
            cerr << ex.get_message() << endl;
            CPPUNIT_ASSERT( !"couldn't open File1" );
        }

        BESContainerStorageFile cpf("File1");
        char s[10];
        char r[10];
        char c[10];
        for (int i = 1; i < 6; i++) {
            sprintf(s, "sym%d", i);
            sprintf(r, "real%d", i);
            sprintf(c, "type%d", i);
            cout << "    looking for " << s << endl;
            BESContainer *d = cpf.look_for(s);
            CPPUNIT_ASSERT( d );
            CPPUNIT_ASSERT( d->get_real_name() == r );
            CPPUNIT_ASSERT( d->get_container_type() == c );
        }

        cout << "    looking for notthere" << endl;
        BESContainer *d = cpf.look_for("notthere");
        CPPUNIT_ASSERT( !d );

        cout << "*****************************************" << endl;
        cout << "show containers" << endl;
        BESTextInfo info;
        cpf.show_containers(info);
        info.print(cout);

        cout << "*****************************************" << endl;
        cout << "Returning from pfileT::run" << endl;
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( connT );

int main(int argc, char*argv[])
{
    int option_char;
    while ((option_char = getopt(argc, argv, "dh")) != EOF)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;
        case 'h': {     // help - show test names
            cerr << "Usage: connT has the following tests:" << endl;
            const std::vector<Test*> &tests = connT::suite()->getTests();
            unsigned int prefix_len = connT::suite()->getName().append("::").size();
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
            test = connT::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
#endif