// constraintT.cc

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
#include <memory>
#include <cstdlib>

using std::cerr;
using std::cout;
using std::endl;
using std::string;

#include "BESFileContainer.h"
#include "BESDataHandlerInterface.h"
#include "BESConstraintFuncs.h"
#include "BESDataNames.h"
#include "TheBESKeys.h"
#include <test_config.h>
#include <unistd.h>

static bool debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

class constraintT : public TestFixture {
private:

public:
    constraintT() {
    }

    ~constraintT() {
    }

    void setUp() {
        string bes_conf = (string) TEST_SRC_DIR + "/empty.ini";
        TheBESKeys::ConfigFile = bes_conf;
    }

    void tearDown() {
    }

    CPPUNIT_TEST_SUITE(constraintT);

        CPPUNIT_TEST(do_test);

    CPPUNIT_TEST_SUITE_END();

    void do_test() {
        cout << "*****************************************" << endl;
        cout << "Running constraintT tests" << endl;
#if 1
        {
            cout << "*****************************************" << endl;
            cout << "Build the data and build the post constraint" << endl;
            BESDataHandlerInterface dhi;
            // BESContainer *d1 = new BESFileContainer("sym1", "real1", "type1");                                                                                                                                                  
            auto d1 = std::make_unique<BESFileContainer>("sym1", "real1", "type1");
            d1->set_constraint("var1");
            // dhi.containers.push_back(d1);                                                                                                                                                                                       
            dhi.containers.push_back(d1.get());

            // BESContainer *d2 = new BESFileContainer("sym2", "real2", "type2");                                                                                                                                                  
            auto d2 = std::make_unique<BESFileContainer>("sym2", "real2", "type2");
            d2->set_constraint("var2");
            // dhi.containers.push_back(d2);                                                                                                                                                                                       
            dhi.containers.push_back(d2.get());

            dhi.first_container();
            BESConstraintFuncs::post_append(dhi);
            dhi.next_container();
            BESConstraintFuncs::post_append(dhi);

            string should_be = "sym1.var1,sym2.var2";
            cout << "    post constraint = " << dhi.data[POST_CONSTRAINT] << endl;
            cout << "    should be = " << should_be << endl;
            CPPUNIT_ASSERT(dhi.data[POST_CONSTRAINT] == should_be);
        }
        {
            cout << "*****************************************" << endl;
            cout << "Only first container has constraint" << endl;
            BESDataHandlerInterface dhi;
            auto d1 = std::make_unique<BESFileContainer>("sym1", "real1", "type1");
            dhi.containers.push_back(d1.get());

            auto d2 = std::make_unique<BESFileContainer>("sym2", "real2", "type2");
            d2->set_constraint("var2");
            dhi.containers.push_back(d2.get());

            dhi.first_container();
            BESConstraintFuncs::post_append(dhi);
            dhi.next_container();
            BESConstraintFuncs::post_append(dhi);

            string should_be = "sym1,sym2.var2";
            cout << "    post constraint = " << dhi.data[POST_CONSTRAINT] << endl;
            cout << "    should be = " << should_be << endl;
            CPPUNIT_ASSERT(dhi.data[POST_CONSTRAINT] == should_be);
        }
        {
            cout << "*****************************************" << endl;
            cout << "Only second container has constraint" << endl;
            BESDataHandlerInterface dhi;
            auto d1 = std::make_unique<BESFileContainer>("sym1", "real1", "type1");
            d1->set_constraint("var1");
            dhi.containers.push_back(d1.get());

            auto d2 = std::make_unique<BESFileContainer>("sym2", "real2", "type2");
            dhi.containers.push_back(d2.get());

            dhi.first_container();
            BESConstraintFuncs::post_append(dhi);
            dhi.next_container();
            BESConstraintFuncs::post_append(dhi);

            string should_be = "sym1.var1,sym2";
            cout << "    post constraint = " << dhi.data[POST_CONSTRAINT] << endl;
            cout << "    should be = " << should_be << endl;
            CPPUNIT_ASSERT(dhi.data[POST_CONSTRAINT] == should_be);
        }
#endif


        cout << "*****************************************" << endl;
        cout << "Done running constraintT tests" << endl;
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(constraintT);

int main(int argc, char *argv[]) {
    int option_char;
    while ((option_char = getopt(argc, argv, "dh")) != EOF)
        switch (option_char) {
            case 'd':
                debug = 1; // debug is a static global
                break;
            case 'h': {
                // help - show test names
                cerr << "Usage: constraintT has the following tests:" << endl;
                const std::vector<Test *> &tests = constraintT::suite()->getTests();
                unsigned int prefix_len = constraintT::suite()->getName().append("::").size();
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
    }
    else {
        int i = 0;
        while (i < argc) {
            if (debug) cerr << "Running " << argv[i] << endl;
            test = constraintT::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
