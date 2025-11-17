// defT.C

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

using namespace CppUnit;

#include <cstdlib>
#include <iostream>
#include <sstream>

using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::stringstream;

#include "BESDefine.h"
#include "BESDefinitionStorageList.h"
#include "BESDefinitionStorageVolatile.h"
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

class defT : public TestFixture {
private:
public:
    defT() {}
    ~defT() {}

    void setUp() {
        string bes_conf = (string)TEST_SRC_DIR + "/defT.ini";
        TheBESKeys::ConfigFile = bes_conf;
    }

    void tearDown() {}

    CPPUNIT_TEST_SUITE(defT);

    CPPUNIT_TEST(do_test);

    CPPUNIT_TEST_SUITE_END();

    void do_test() {
        cout << "*****************************************" << endl;
        cout << "Entered defT::run" << endl;

        BESDefinitionStorageList::TheList()->add_persistence(new BESDefinitionStorageVolatile(DEFAULT));
        BESDefinitionStorage *store = BESDefinitionStorageList::TheList()->find_persistence(DEFAULT);

        cout << "*****************************************" << endl;
        cout << "add d1, d2, d3, d4, d5" << endl;
        for (unsigned int i = 1; i < 6; i++) {
            stringstream name;
            name << "d" << i;
            stringstream agg;
            agg << "d" << i << "agg";
            BESDefine *dd = new BESDefine;
            dd->set_agg_cmd(agg.str());
            cout << "    adding " << name.str() << endl;
            CPPUNIT_ASSERT(store->add_definition(name.str(), dd));
        }

        cout << "*****************************************" << endl;
        cout << "find d1, d2, d3, d4, d5" << endl;
        for (unsigned int i = 1; i < 6; i++) {
            stringstream name;
            name << "d" << i;
            stringstream agg;
            agg << "d" << i << "agg";
            cout << "    looking for " << name.str() << endl;
            BESDefine *dd = store->look_for(name.str());
            CPPUNIT_ASSERT(dd);
            CPPUNIT_ASSERT(dd->get_agg_cmd() == agg.str());
        }

        cout << "*****************************************" << endl;
        cout << "show definitions" << endl;
        {
            BESTextInfo info;
            store->show_definitions(info);
            info.print(cout);
        }

        cout << "*****************************************" << endl;
        cout << "delete d3" << endl;
        {
            CPPUNIT_ASSERT(store->del_definition("d3"));
            BESDefine *dd = store->look_for("d3");
            CPPUNIT_ASSERT(!dd);
        }

        cout << "*****************************************" << endl;
        cout << "delete d1" << endl;
        {
            CPPUNIT_ASSERT(store->del_definition("d1"));
            BESDefine *dd = store->look_for("d1");
            CPPUNIT_ASSERT(!dd);
        }

        cout << "*****************************************" << endl;
        cout << "delete d5" << endl;
        {
            CPPUNIT_ASSERT(store->del_definition("d5"));
            BESDefine *dd = store->look_for("d5");
            CPPUNIT_ASSERT(!dd);
        }

        cout << "*****************************************" << endl;
        cout << "find d2 and d4" << endl;
        {
            BESDefine *dd = store->look_for("d2");
            CPPUNIT_ASSERT(dd);

            dd = store->look_for("d4");
            CPPUNIT_ASSERT(dd);
        }

        cout << "*****************************************" << endl;
        cout << "delete all definitions" << endl;
        store->del_definitions();

        cout << "*****************************************" << endl;
        cout << "find definitions d1, d2, d3, d4, d5" << endl;
        for (unsigned int i = 1; i < 6; i++) {
            stringstream name;
            name << "d" << i;
            stringstream agg;
            agg << "d" << i << "agg";
            cout << "    looking for " << name.str() << endl;
            BESDefine *dd = store->look_for(name.str());
            CPPUNIT_ASSERT(!dd);
        }

        cout << "*****************************************" << endl;
        cout << "Returning from defT::run" << endl;
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(defT);

int main(int argc, char *argv[]) {
    int option_char;
    while ((option_char = getopt(argc, argv, "dh")) != EOF)
        switch (option_char) {
        case 'd':
            debug = 1; // debug is a static global
            break;
        case 'h': { // help - show test names
            cerr << "Usage: defT has the following tests:" << endl;
            const std::vector<Test *> &tests = defT::suite()->getTests();
            unsigned int prefix_len = defT::suite()->getName().append("::").size();
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
            test = defT::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
