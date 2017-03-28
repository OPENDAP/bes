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
    BESContainerStorageVolatile *cpv;

public:
    pvolT() : cpv(0)
    {
        string bes_conf = (string) TEST_SRC_DIR + "/persistence_cgi_test.ini";
        TheBESKeys::ConfigFile = bes_conf;

        // This cannot be constructed unless the Keys file can be read.
        cpv = new BESContainerStorageVolatile("volatile");

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
    }

    ~pvolT()
    {
        delete cpv;

        remove( "./real1" );
        remove( "./real2" );
        remove( "./real3" );
        remove( "./real4" );
        remove( "./real5" );
    }

    void setUp()
    {
    }

    void tearDown()
    {
    }

    // The rest of the tests depend on these settings; call this 'test' from
    // them to set up the those tests.
    //
    // NB: Each test appears to get it's own instance of pvolT (which is not
    // how I understood the cppunit tests to work... jhrg 3/27/17)
    void add_container_test() {
        DBG(cerr << "add_container_test BEGIN" << endl);

        try {
            cpv->add_container("sym1", "real1", "type1");
            cpv->add_container("sym2", "real2", "type2");
            cpv->add_container("sym3", "real3", "type3");
            cpv->add_container("sym4", "real4", "type4");
            cpv->add_container("sym5", "real5", "type5");
        }
        catch (BESError &e) {
            CPPUNIT_FAIL( "Failed to add elements: " + e.get_message());
        }

        CPPUNIT_ASSERT( true );

        DBG(cerr << "add_container_test END" << endl);
    }

    void add_overlapping_container() {
        add_container_test();

        DBG(cerr << "add_overlapping_container BEGIN" << endl);

        try {
            cpv->add_container("sym1", "real1", "type1");
            CPPUNIT_FAIL( "Successfully added sym1 again" );
        }
        catch (BESError &e) {
            DBG(cerr << "Unable to add sym1 again, good: " << e.get_message() << endl);
            CPPUNIT_ASSERT( "unable to add sym1 again, good" );
        }

        DBG(cerr << "add_overlapping_container END" << endl);
    }

    void test_look_for_containers() {
        add_container_test();

        DBG(cerr << "test_look_for_containers BEGIN" << endl);

        char s[10];
        char r[10];
        char c[10];
        for (int i = 1; i < 6; i++) {
            sprintf(s, "sym%d", i);
            sprintf(r, "./real%d", i);
            sprintf(c, "type%d", i);

            DBG(cerr << "    looking for " << s << endl);

            BESContainer *d = cpv->look_for(s);
            CPPUNIT_ASSERT( d );
            CPPUNIT_ASSERT( d->get_real_name() == r );
            CPPUNIT_ASSERT( d->get_container_type() == c );
        }

        DBG(cerr << "test_look_for_containers END" << endl);
    }

    void test_del_container() {
        add_container_test();

        DBG(cerr << "test_del_container BEGIN" << endl);

        CPPUNIT_ASSERT( cpv->del_container( "sym1" ) );

        {
            BESContainer *d = cpv->look_for("sym1");
            CPPUNIT_ASSERT( !d );
        }

        CPPUNIT_ASSERT( cpv->del_container( "sym5" ) );

        {
            BESContainer *d = cpv->look_for("sym5");
            CPPUNIT_ASSERT( !d );
        }

        CPPUNIT_ASSERT( !cpv->del_container( "nosym" ) );

        DBG(cerr << "test_del_container END" << endl);

        // Double check the correct containers are really gone
        {
            char s[10];
            char r[10];
            char c[10];
            for (int i = 2; i < 5; i++) {
                sprintf(s, "sym%d", i);
                sprintf(r, "./real%d", i);
                sprintf(c, "type%d", i);

                DBG(cerr << "    looking for " << s << endl);

                BESContainer *d = cpv->look_for(s);
                CPPUNIT_ASSERT( d );
                CPPUNIT_ASSERT( d->get_real_name() == r );
                CPPUNIT_ASSERT( d->get_container_type() == c );
            }
        }
    }

    CPPUNIT_TEST_SUITE( pvolT );

    CPPUNIT_TEST( add_container_test );
    CPPUNIT_TEST( add_overlapping_container );
    CPPUNIT_TEST( test_look_for_containers );
    CPPUNIT_TEST( test_del_container );

    CPPUNIT_TEST_SUITE_END();
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
