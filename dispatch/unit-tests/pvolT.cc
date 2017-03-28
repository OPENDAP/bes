// pvolT.cc

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
using std::ofstream;
using std::ios;

#include "BESContainerStorageVolatile.h"
#include "BESContainer.h"
#include "TheBESKeys.h"
#include "BESError.h"
#include "BESTextInfo.h"

#include <GetOpt.h>
#include <debug.h>

#include "test_config.h"

static bool debug = false;
static bool debug_2 = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);
#undef DBG2
#define DBG2(x) do { if (debug_2) (x); } while(false);

class pvolT: public TestFixture {
private:

public:
    pvolT()
    {
        string bes_conf = (string) TEST_SRC_DIR + "/persistence_cgi_test.ini";
        TheBESKeys::ConfigFile = bes_conf;
    }
    ~pvolT()
    {
    }

    void setUp()
    {
#if 1
        ofstream real1( "./real1", ios::trunc );
        real1 << "real1" << endl;

        ofstream real2( "./real2", ios::trunc );
        real2 << "real2" << endl;

        ofstream real3( "./real3", ios::trunc );
        real3 << "real3" << endl;

        ofstream real4( "./real4", ios::trunc );
        real4 << "real4" << endl;

        ofstream real5( "./real5", ios::trunc );
        real5 << "real5" << endl;
#endif
    }

    void tearDown()
    {
#if 1
        remove( "./real1" );
        remove( "./real2" );
        remove( "./real3" );
        remove( "./real4" );
        remove( "./real5" );
#endif
    }

    CPPUNIT_TEST_SUITE( pvolT );

    CPPUNIT_TEST( do_test );

    CPPUNIT_TEST_SUITE_END();

    void do_test()
    {
        cout << "*****************************************" << endl;
        cout << "Entered pvolT::run" << endl;

        BESContainerStorageVolatile cpv("volatile");

        cout << "*****************************************" << endl;
        cout << "Create volatile and add five elements" << endl;
        try {
            cpv.add_container("sym1", "real1", "type1");
            cpv.add_container("sym2", "real2", "type2");
            cpv.add_container("sym3", "real3", "type3");
            cpv.add_container("sym4", "real4", "type4");
            cpv.add_container("sym5", "real5", "type5");
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT( ! "failed to add elements" );
        }

        cout << "*****************************************" << endl;
        cout << "show containers" << endl;
        BESTextInfo info;
        cpv.show_containers(info);
        info.print(cout);

        cout << "*****************************************" << endl;
        cout << "try to add sym1 again" << endl;
        try {
            cpv.add_container("sym1", "real1", "type1");
            CPPUNIT_FAIL( "Successfully added sym1 again" );
        }
        catch (BESError &e) {
            cout << "unable to add sym1 again, good" << endl;
            cout << e.get_message() << endl;
        }

        cout << "*****************************************" << endl;
        cout << "Look for containers" << endl;
        {
            char s[10];
            char r[10];
            char c[10];
            for (int i = 1; i < 6; i++) {
                sprintf(s, "sym%d", i);
                sprintf(r, "./real%d", i);
                sprintf(c, "type%d", i);
                cout << "    looking for " << s << endl;
                BESContainer *d = cpv.look_for(s);
                CPPUNIT_ASSERT( d );
                CPPUNIT_ASSERT( d->get_real_name() == r );
                CPPUNIT_ASSERT( d->get_container_type() == c );
            }
        }

        cout << "*****************************************" << endl;
        cout << "remove sym1" << endl;
        CPPUNIT_ASSERT( cpv.del_container( "sym1" ) );

        {
            cout << "*****************************************" << endl;
            cout << "find sym1" << endl;
            BESContainer *d = cpv.look_for("sym1");
            CPPUNIT_ASSERT( !d );
        }

        cout << "*****************************************" << endl;
        cout << "remove sym5" << endl;
        CPPUNIT_ASSERT( cpv.del_container( "sym5" ) );

        {
            cout << "*****************************************" << endl;
            cout << "find sym5" << endl;
            BESContainer *d = cpv.look_for("sym5");
            CPPUNIT_ASSERT( !d );
        }

        cout << "*****************************************" << endl;
        cout << "remove nosym" << endl;
        CPPUNIT_ASSERT( !cpv.del_container( "nosym" ) );

        cout << "*****************************************" << endl;
        cout << "Look for containers" << endl;
        {
            char s[10];
            char r[10];
            char c[10];
            for (int i = 2; i < 5; i++) {
                sprintf(s, "sym%d", i);
                sprintf(r, "./real%d", i);
                sprintf(c, "type%d", i);
                cout << "    looking for " << s << endl;
                BESContainer *d = cpv.look_for(s);
                CPPUNIT_ASSERT( d );
                CPPUNIT_ASSERT( d->get_real_name() == r );
                CPPUNIT_ASSERT( d->get_container_type() == c );
            }
        }

        cout << "*****************************************" << endl;
        cout << "show containers" << endl;
        BESTextInfo info2;
        cpv.show_containers(info2);
        info2.print(cout);

        cout << "*****************************************" << endl;
        cout << "Returning from pvolT::run" << endl;
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( pvolT );

int main(int argc, char*argv[])
{
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    GetOpt getopt(argc, argv, "dD");
    int option_char;
    while ((option_char = getopt()) != -1)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;
        case 'D':
            debug_2 = 1;
            break;
        default:
            break;
        }

    bool wasSuccessful = true;
    string test = "";
    int i = getopt.optind;
    if (i == argc) {
        // run them all
        wasSuccessful = runner.run("");
    }
    else {
        while (i < argc) {
            test = string("pvolT::") + argv[i++];

            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
#if 0
int main(int, char**)
{
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    bool wasSuccessful = runner.run("", false);

    return wasSuccessful ? 0 : 1;
}
#endif
