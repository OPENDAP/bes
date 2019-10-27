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

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

using namespace CppUnit;

#include <iostream>
#include <cstdlib>

using std::cerr;
using std::cout;
using std::endl;

#include "BESContainerStorageList.h"
#include "BESContainerStorageFile.h"
#include "BESContainer.h"
#include "BESError.h"
#include "BESTextInfo.h"
#include "TheBESKeys.h"
#include <test_config.h>
#include <GetOpt.h>

static bool debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

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

    GetOpt getopt(argc, argv, "dh");
    int option_char;
    while ((option_char = getopt()) != EOF)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;
        case 'h': {     // help - show test names
            cerr << "Usage: plistT has the following tests:" << endl;
            const std::vector<Test*> &tests = plistT::suite()->getTests();
            unsigned int prefix_len = plistT::suite()->getName().append("::").length();
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
            test = plistT::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}

