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
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "BESContainer.h"
#include "BESContainerStorageVolatile.h"
#include "BESError.h"
#include "BESTextInfo.h"
#include "TheBESKeys.h"

#include <libdap/debug.h>
#include <unistd.h>

#include "test_config.h"

using namespace CppUnit;
using namespace std;

static bool debug = false;
static bool debug_2 = false;

// This value must match the value in the persistence_cgi_test.ini
// keys file.
const string cache_dir = "."; // "./cache";

#undef DBG
#define DBG(x)                                                                                                         \
    do {                                                                                                               \
        if (debug)                                                                                                     \
            (x);                                                                                                       \
    } while (false);
#undef DBG2
#define DBG2(x)                                                                                                        \
    do {                                                                                                               \
        if (debug_2)                                                                                                   \
            (x);                                                                                                       \
    } while (false);

class pvolT : public TestFixture {
private:
    BESContainerStorageVolatile *cpv;

    string real1, real2, real3, real4, real5;

public:
    pvolT()
        : cpv(0), real1(cache_dir + "/real1"), real2(cache_dir + "/real2"), real3(cache_dir + "/real3"),
          real4(cache_dir + "/real4"), real5(cache_dir + "/real5") {
        string bes_conf = (string)TEST_SRC_DIR + "/persistence_cgi_test.ini";
        TheBESKeys::ConfigFile = bes_conf;

        // This cannot be constructed unless the Keys file can be read.
        cpv = new BESContainerStorageVolatile("volatile");

        ofstream of1(real1.c_str(), ios::trunc);
        of1 << "real1" << endl;

        ofstream of2(real2.c_str(), ios::trunc);
        of2 << "real2" << endl;

        ofstream of3(real3.c_str(), ios::trunc);
        of3 << "real3" << endl;

        ofstream of4(real4.c_str(), ios::trunc);
        of4 << "real4" << endl;

        ofstream of5(real5.c_str(), ios::trunc);
        of5 << "real5" << endl;
    }

    ~pvolT() {
        delete cpv;

        remove(real1.c_str());
        remove(real2.c_str());
        remove(real3.c_str());
        remove(real4.c_str());
        remove(real5.c_str());
    }

    void setUp() {}

    void tearDown() {}

    CPPUNIT_TEST_SUITE(pvolT);

    CPPUNIT_TEST(add_container_test);
    CPPUNIT_TEST(add_overlapping_container);
    CPPUNIT_TEST(test_look_for_containers);
    CPPUNIT_TEST(test_del_container);

    CPPUNIT_TEST_SUITE_END();

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
        } catch (BESError &e) {
            CPPUNIT_FAIL("Failed to add elements: " + e.get_message());
        }

        CPPUNIT_ASSERT(true);

        DBG(cerr << "add_container_test END" << endl);
    }

    void add_overlapping_container() {
        add_container_test();

        DBG(cerr << "add_overlapping_container BEGIN" << endl);

        try {
            cpv->add_container("sym1", "real1", "type1");
            CPPUNIT_FAIL("Successfully added sym1 again");
        } catch (BESError &e) {
            DBG(cerr << "Unable to add sym1 again, good: " << e.get_message() << endl);
            CPPUNIT_ASSERT("unable to add sym1 again, good");
        }

        DBG(cerr << "add_overlapping_container END" << endl);
    }

    void test_look_for_containers() {
        add_container_test();

        DBG(cerr << "test_look_for_containers BEGIN" << endl);

        for (int i = 1; i < 6; i++) {
            ostringstream s;
            s << "sym" << i;
            ostringstream r;
            r << cache_dir + "/real" << i;
            ostringstream c;
            c << "type" << i;

            DBG(cerr << "    looking for " << s.str() << endl);

            BESContainer *d = cpv->look_for(s.str());
            CPPUNIT_ASSERT(d);
            CPPUNIT_ASSERT(d->get_real_name() == r.str());
            CPPUNIT_ASSERT(d->get_container_type() == c.str());
        }

        DBG(cerr << "test_look_for_containers END" << endl);
    }

    void test_del_container() {
        add_container_test();

        DBG(cerr << "test_del_container BEGIN" << endl);

        CPPUNIT_ASSERT(cpv->del_container("sym1"));

        {
            BESContainer *d = cpv->look_for("sym1");
            CPPUNIT_ASSERT(!d);
        }

        CPPUNIT_ASSERT(cpv->del_container("sym5"));

        {
            BESContainer *d = cpv->look_for("sym5");
            CPPUNIT_ASSERT(!d);
        }

        CPPUNIT_ASSERT(!cpv->del_container("nosym"));

        DBG(cerr << "test_del_container END" << endl);

        // Double check the correct containers are really gone
        {
            for (int i = 2; i < 5; i++) {
                ostringstream s;
                s << "sym" << i;
                ostringstream r;
                r << cache_dir + "/real" << i;
                ostringstream c;
                c << "type" << i;

                DBG(cerr << "    looking for " << s.str() << endl);

                BESContainer *d = cpv->look_for(s.str());
                CPPUNIT_ASSERT(d);
                CPPUNIT_ASSERT(d->get_real_name() == r.str());
                CPPUNIT_ASSERT(d->get_container_type() == c.str());
            }
        }
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(pvolT);

int main(int argc, char *argv[]) {
    int option_char;
    while ((option_char = getopt(argc, argv, "dDh")) != -1)
        switch (option_char) {
        case 'd':
            debug = 1; // debug is a static global
            break;
        case 'D':
            debug_2 = 1;
            break;
        case 'h': { // help - show test names
            cerr << "Usage: pvolT has the following tests:" << endl;
            const std::vector<Test *> &tests = pvolT::suite()->getTests();
            unsigned int prefix_len = pvolT::suite()->getName().append("::").size();
            for (std::vector<Test *>::const_iterator i = tests.begin(), e = tests.end(); i != e; ++i) {
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
    } else {
        int i = 0;
        while (i < argc) {
            if (debug)
                cerr << "Running " << argv[i] << endl;
            test = pvolT::suite()->getName().append("::").append(argv[i++]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
