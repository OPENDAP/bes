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
#include <sstream>

// Include necessary BES headers
#include "BESContainerStorageFile.h"
#include "BESContainer.h"
#include "BESError.h"
#include "BESTextInfo.h"
#include "TheBESKeys.h"
#include <test_config.h> // Assumed to define TEST_SRC_DIR

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

    CPPUNIT_TEST_SUITE_END();

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
     * @brief Tests finding existing containers using look_for (using std::ostringstream).
     */
    void testLookFor_FindsExistingContainers() {

        DBG(cout << "Running testLookFor_FindsExistingContainers" << endl);

        BESContainerStorageFile cpf("File1"); // Assumes File1 is valid (tested above)

        for (int i = 1; i < 6; ++i) {
            std::ostringstream ss_sym, ss_real, ss_type;

            ss_sym << "sym" << i;
            string sym_key = ss_sym.str();

            ss_real << "real" << i;
            string expected_real_name = ss_real.str();

            ss_type << "type" << i;
            string expected_type = ss_type.str();

            DBG(cout << "    looking for " << sym_key << endl);

            std::unique_ptr<BESContainer> d(cpf.look_for(sym_key));

            std::string find_msg = "Should find container for key: " + sym_key;
            CPPUNIT_ASSERT_MESSAGE(find_msg, d != nullptr);

            if (d) {
                std::string real_name_msg = "Real name mismatch for key: " + sym_key;
                std::string type_msg = "Container type mismatch for key: " + sym_key;

                CPPUNIT_ASSERT_EQUAL_MESSAGE(real_name_msg, expected_real_name, d->get_real_name());
                CPPUNIT_ASSERT_EQUAL_MESSAGE(type_msg, expected_type, d->get_container_type());
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
        std::unique_ptr<BESContainer> d(cpf.look_for("notthere"));
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


CPPUNIT_TEST_SUITE_REGISTRATION(connT);

int main(int argc, char *argv[]) {
    const bool success = bes_run_tests<connT>(argc, argv, "bes,container");

    // Return 0 for success (true from bes_run_tests), 1 for failure (false).
    return success ? 0 : 1;
}
