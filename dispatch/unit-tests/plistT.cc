// plistT.C

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

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>

// Application headers.
#include "BESContainer.h"
#include "BESContainerStorageFile.h"
#include "BESContainerStorageList.h"
#include "BESError.h"
#include "BESTextInfo.h"
#include "TheBESKeys.h"
#include "test_config.h"

// Include the header-only test runner and its debug macros.
#include "modules/common/run_tests_cppunit.h"

class PlistT : public CppUnit::TestFixture {
    BESContainerStorageList *csl_ = BESContainerStorageList::TheList();

public:
    PlistT() = default;
    ~PlistT() override = default;

    // Configure common state before each test.
    void setUp() override {
        std::string bes_conf = std::string(TEST_SRC_DIR) + "/persistence_file_test.ini";
        TheBESKeys::ConfigFile = bes_conf;
    }

    void tearDown() override {}

    // Test: Adding persistence objects works for File1 and File2.
    void testAddPersistence() {
        const auto keys = TheBESKeys::TheKeys();
        keys->set_key("BES.Container.Persistence.File.File1=" + std::string(TEST_SRC_DIR) + "/persistence_file1.txt");
        keys->set_key("BES.Container.Persistence.File.File2=" + std::string(TEST_SRC_DIR) + "/persistence_file2.txt");

        // auto* cpl = BESContainerStorageList::TheList();

        auto *cpf1 = new BESContainerStorageFile("File1");
        CPPUNIT_ASSERT(csl_->add_persistence(cpf1));

        auto *cpf2 = new BESContainerStorageFile("File2");
        CPPUNIT_ASSERT(csl_->add_persistence(cpf2));
    }

    // Test: Attempting to add a duplicate persistence (File2) fails.
    void testDuplicateAddPersistence() {
        // auto* cpl = BESContainerStorageList::TheList();

        const auto duplicate_cpf = std::make_unique<BESContainerStorageFile>("File1");
        CPPUNIT_ASSERT_MESSAGE("Should not be able to add a duplicate storage",
                               !csl_->add_persistence(duplicate_cpf.get()));
    }

    // Test: Existing containers (sym1 to sym10) are found with the expected properties.
    void testLookupExistingContainers() {
        // auto* cpl = BESContainerStorageList::TheList();

        for (int i = 1; i <= 10; ++i) {
            const std::string sym = "sym" + std::to_string(i);
            const std::string expected_real = "real" + std::to_string(i);
            const std::string expected_type = "type" + std::to_string(i);
            DBG(std::cerr << "Looking for container: " << sym << '\n');
            try {
                std::unique_ptr<BESContainer> container(csl_->look_for(sym));
                CPPUNIT_ASSERT(container != nullptr);
                CPPUNIT_ASSERT_EQUAL(expected_real, container->get_real_name());
                CPPUNIT_ASSERT_EQUAL(expected_type, container->get_container_type());
            } catch (BESError &e) {
                CPPUNIT_FAIL("Exception while looking for " + sym);
            }
        }
    }

    // Test: Looking up a non-existent container ("notthere") behaves as expected.
    void testLookupNonExistentContainer() {
        // auto* cpl = BESContainerStorageList::TheList();
        try {
            std::unique_ptr<BESContainer> container(csl_->look_for("notthere"));
            CPPUNIT_FAIL("Expected exception for non-existent container");
        } catch (BESError &e) {
            DBG(std::cerr << "Found expected exception for non-existent container: " << e.get_message() << '\n');
        }
    }

    // Test: Showing containers does not throw errors.
    void testShowContainers() {
        // auto* cpl = BESContainerStorageList::TheList();
        BESTextInfo info;
        CPPUNIT_ASSERT_NO_THROW(csl_->show_containers(info));
        CPPUNIT_ASSERT_NO_THROW(info.print(std::cerr));
    }

    // Test: Removing persistence for "File1" succeeds.
    void testRemovePersistence() {
        // auto* cpl = BESContainerStorageList::TheList();
        CPPUNIT_ASSERT(csl_->deref_persistence("File1"));

        try {
            std::unique_ptr<BESContainer> container(csl_->look_for("sym2"));
            CPPUNIT_FAIL("Expected exception after removal");
        } catch (BESError &e) {
            DBG(std::cerr << "Found expected exception after removal: " << e.get_message() << '\n');
        }

        try {
            std::unique_ptr<BESContainer> container7(csl_->look_for("sym7"));
            CPPUNIT_ASSERT(container7 != nullptr);
            CPPUNIT_ASSERT_EQUAL(std::string("real7"), container7->get_real_name());
            CPPUNIT_ASSERT_EQUAL(std::string("type7"), container7->get_container_type());
        } catch (BESError &e) {
            CPPUNIT_FAIL("Exception while looking for container sym7");
        }
    }

    CPPUNIT_TEST_SUITE(PlistT);
    CPPUNIT_TEST(testAddPersistence);
    CPPUNIT_TEST(testDuplicateAddPersistence);
    CPPUNIT_TEST(testLookupExistingContainers);
    CPPUNIT_TEST(testLookupNonExistentContainer);
    CPPUNIT_TEST(testShowContainers);
    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(PlistT);

// The main function now leverages the header-only test runner template.
int main(const int argc, char *argv[]) { return bes_run_tests<PlistT>(argc, argv, "dDh") ? 0 : 1; }

#if 0
#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

using namespace CppUnit;

#include <cstdlib>
#include <iostream>

using std::cerr;
using std::cout;
using std::endl;
using std::string;

#include "BESContainer.h"
#include "BESContainerStorageFile.h"
#include "BESContainerStorageList.h"
#include "BESError.h"
#include "BESTextInfo.h"
#include "TheBESKeys.h"
#include "test_config.h"
#include <unistd.h>

static bool debug = false;

#undef DBG
#define DBG(x)                                                                                                         \
    do {                                                                                                               \
        if (debug)                                                                                                     \
            (x);                                                                                                       \
    } while (false);

class plistT: public TestFixture {
private:

public:
    plistT()
    {
    }
    ~plistT()
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

    CPPUNIT_TEST_SUITE( plistT );

    CPPUNIT_TEST( do_test );

    CPPUNIT_TEST_SUITE_END();

    void do_test()
    {
        TheBESKeys *keys = TheBESKeys::TheKeys();
        keys->set_key((string) "BES.Container.Persistence.File.File1=" + TEST_SRC_DIR + "/persistence_file1.txt");
        keys->set_key((string) "BES.Container.Persistence.File.File2=" + TEST_SRC_DIR + "/persistence_file2.txt");

        cout << "*****************************************" << endl;
        cout << "Entered plistT::run" << endl;

        cout << "*****************************************" << endl;
        cout << "Create the BESContainerPersistentList" << endl;
        BESContainerStorageList *cpl = BESContainerStorageList::TheList();

        cout << "*****************************************" << endl;
        cout << "Add BESContainerStorageFile for File1 and File2" << endl;
        BESContainerStorageFile *cpf;
        cpf = new BESContainerStorageFile("File1");
        CPPUNIT_ASSERT( cpl->add_persistence( cpf ) );
        cpf = new BESContainerStorageFile("File2");
        CPPUNIT_ASSERT( cpl->add_persistence( cpf ) );

        cout << "*****************************************" << endl;
        cout << "Try to add File2 again" << endl;
        cpf = new BESContainerStorageFile("File2");
        CPPUNIT_ASSERT( cpl->add_persistence( cpf ) == false );

        cout << "*****************************************" << endl;
        cout << "look for containers" << endl;
        char s[10];
        char r[10];
        char c[10];
        for (int i = 1; i < 11; i++) {
            sprintf(s, "sym%d", i);
            sprintf(r, "real%d", i);
            sprintf(c, "type%d", i);
            cout << "    looking for " << s << endl;
            try {
                BESContainer *d = cpl->look_for(s);
                CPPUNIT_ASSERT( d );
                CPPUNIT_ASSERT( d->get_real_name() == r );
                CPPUNIT_ASSERT( d->get_container_type() == c );
            }
            catch (BESError &e) {
                CPPUNIT_ASSERT( !"couldn't find" );
            }
        }

        cout << "*****************************************" << endl;
        cout << "looking for non-existant notthere" << endl;
        try {
            BESContainer *dnot = cpl->look_for("notthere");
            CPPUNIT_ASSERT( !dnot );
        }
        catch (BESError &e) {
            cout << "didn't find notthere, good" << endl;
            cout << e.get_message() << endl;
        }

        cout << "*****************************************" << endl;
        cout << "show containers" << endl;
        BESTextInfo info;
        cpl->show_containers(info);
        info.print(cout);

        cout << "*****************************************" << endl;
        cout << "remove File1" << endl;
        CPPUNIT_ASSERT( cpl->deref_persistence( "File1" ) == true );

        cout << "*****************************************" << endl;
        cout << "looking for sym2" << endl;
        try {
            BESContainer *d2 = cpl->look_for("sym2");
            CPPUNIT_ASSERT( !d2 );
        }
        catch (BESError &e) {
            cout << "couldn't find sym2, good" << endl;
            cout << e.get_message() << endl;
        }

        cout << "*****************************************" << endl;
        cout << "looking for sym7" << endl;
        try {
            BESContainer *d7 = cpl->look_for("sym7");
            CPPUNIT_ASSERT( d7 );
            CPPUNIT_ASSERT( d7->get_real_name() == "real7" );
            CPPUNIT_ASSERT( d7->get_container_type() == "type7" );
        }
        catch (BESError &e) {
            CPPUNIT_ASSERT( !"couldn't find sym7" );
        }

        cout << "*****************************************" << endl;
        cout << "Returning from plistT::run" << endl;
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( plistT );

int main(int argc, char*argv[])
{
    int option_char;
    while ((option_char = getopt(argc, argv, "dh")) != EOF)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;
        case 'h': {     // help - show test names
            cerr << "Usage: plistT has the following tests:" << endl;
            const std::vector<Test*> &tests = plistT::suite()->getTests();
            unsigned int prefix_len = plistT::suite()->getName().append("::").size();
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
            test = plistT::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
#endif