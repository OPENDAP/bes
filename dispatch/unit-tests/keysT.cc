// keysT.C

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

#include <cstdlib>
#include <iostream>

using std::cerr;
using std::cout;
using std::endl;

#include "TheBESKeys.h"
#include "BESError.h"

#include <GetOpt.h>
#include <debug.h>

#include "test_config.h"

static bool debug = false;
static bool debug_2 = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);
#undef DBG2
#define DBG2(x) do { if (debug_2) (x); } while(false);

using namespace CppUnit;

//string KeyFile;

class keysT: public TestFixture {
private:

public:
    keysT()
    {
    }
    ~keysT()
    {
    }

    void setUp()
    {
    }

    void tearDown()
    {
    }

    CPPUNIT_TEST_SUITE( keysT );

    CPPUNIT_TEST( do_test );

    CPPUNIT_TEST_SUITE_END();

    void do_test()
    {
#if 0
        cout << "*****************************************" << endl;
        cout << "Entered keysT::run" << endl;
        // unused. jhrg 5/24/16 int retVal = 0;

        if (KeyFile != "") {
            TheBESKeys::ConfigFile = KeyFile;
            try {
                TheBESKeys::TheKeys()->dump(cout);
            }
            catch (BESError &e) {
                cout << "unable to create BESKeys:" << endl;
                cout << e.get_message() << endl;
            }
            catch (...) {
                cout << "unable to create BESKeys: unkown exception caught" << endl;
            }
        }
#endif
        cout << "*****************************************" << endl;
        cout << "bad keys, not enough equal signs" << endl;
        string bes_conf = (string) TEST_SRC_DIR + "/bad_keys1.ini";
        TheBESKeys::ConfigFile = bes_conf;
        try {
            TheBESKeys::TheKeys();
            CPPUNIT_FAIL( "loaded keys, should have failed" );
        }
        catch (BESError &e) {
            cout << "unable to create BESKeys, good, because:" << endl;
            cout << e.get_message() << endl;
        }

        cout << "*****************************************" << endl;
        cout << "good keys file, should load" << endl;
        bes_conf = (string) TEST_SRC_DIR + "/keys_test.ini";
        TheBESKeys::ConfigFile = bes_conf;

        cerr << "TheBESKeys::ConfigFile: " << TheBESKeys::ConfigFile << endl;

        try {
            TheBESKeys::TheKeys();
        }
        catch (BESError &e) {
            //cerr << "Error: " << e.get_message() << endl;
            cerr << "TheBESKeys::ConfigFile: " << TheBESKeys::ConfigFile << endl;
            CPPUNIT_FAIL( "Unable to create BESKeys: " + e.get_message());
        }

        cout << "*****************************************" << endl;
        cout << "get keys" << endl;
        bool found = false;
        string ret = "";
        for (int i = 1; i < 5; i++) {
            char key[32];
            sprintf(key, "BES.KEY%d", i);
            char val[32];
            sprintf(val, "val%d", i);
            cout << "looking for " << key << " with value " << val << endl;
            ret = "";
            TheBESKeys::TheKeys()->get_value(key, ret, found);
            CPPUNIT_ASSERT( found );
            CPPUNIT_ASSERT( !ret.empty() );
            CPPUNIT_ASSERT( ret == val );
        }

        cout << "*****************************************" << endl;
        cout << "use get_values to get a single value key" << endl;
        found = false;
        vector<string> vals;
        TheBESKeys::TheKeys()->get_values("BES.KEY1", vals, found);
        CPPUNIT_ASSERT( found );
        CPPUNIT_ASSERT( vals.size() == 1 );
        CPPUNIT_ASSERT( vals[0] == "val1" );

        cout << "*****************************************" << endl;
        cout << "use get_values to get a multi value key" << endl;
        found = false;
        vals.clear();
        TheBESKeys::TheKeys()->get_values("BES.KEY.MULTI", vals, found);
        CPPUNIT_ASSERT( found );
        CPPUNIT_ASSERT( vals.size() == 5 );
        for (int i = 0; i < 5; i++) {
            char val[32];
            sprintf(val, "val_multi_%d", i);
            cout << "looking for value " << val << endl;
            CPPUNIT_ASSERT( vals[i] == val );
        }

        cout << "*****************************************" << endl;
        cout << "use get_value on multi-value key" << endl;
        ret = "";
        found = false;
        try {
            TheBESKeys::TheKeys()->get_value("BES.KEY.MULTI", ret, found);
        }
        catch (BESError &e) {
            cout << "getting single value from multi-value key failed, good" << endl;
            cout << e.get_message() << endl;
        }

        cout << "*****************************************" << endl;
        cout << "look for non existant key" << endl;
        ret = "";
        TheBESKeys::TheKeys()->get_value("BES.NOTFOUND", ret, found);
        CPPUNIT_ASSERT( found == false );
        CPPUNIT_ASSERT( ret.empty() );

        cout << "*****************************************" << endl;
        cout << "look for key with empty value" << endl;
        TheBESKeys::TheKeys()->get_value("BES.KEY5", ret, found);
        CPPUNIT_ASSERT( found );
        CPPUNIT_ASSERT( ret.empty() );

        cout << "*****************************************" << endl;
        cout << "look for key with empty value in included file" << endl;
        TheBESKeys::TheKeys()->get_value("BES.KEY6", ret, found);
        CPPUNIT_ASSERT( found );
        CPPUNIT_ASSERT( ret.empty() );

        cout << "*****************************************" << endl;
        cout << "set bad key, 0 = characters" << endl;
        try {
            TheBESKeys::TheKeys()->set_key("BES.NOEQS");
            CPPUNIT_ASSERT ( !"set_key successful, should have failed" );
        }
        catch (BESError &e) {
            cout << "unable to set the key, good, because:" << endl;
            cout << e.get_message();
        }

        cout << "*****************************************" << endl;
        cout << "set key, 2 = characters" << endl;
        try {
            TheBESKeys::TheKeys()->set_key("BES.2EQS=val1=val2");
            found = false;
            TheBESKeys::TheKeys()->get_value("BES.2EQS", ret, found);
            CPPUNIT_ASSERT( ret == "val1=val2" );
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT( !"failed to set key where value has multiple =" );
        }

        cout << "*****************************************" << endl;
        cout << "set BES.KEY7 to val7" << endl;
        try {
            TheBESKeys::TheKeys()->set_key("BES.KEY7=val7");
            found = false;
            TheBESKeys::TheKeys()->get_value("BES.KEY7", ret, found);
            CPPUNIT_ASSERT( ret == "val7" );
        }
        catch (BESError &e) {
            cerr << e.get_message();
            CPPUNIT_ASSERT( !"unable to set the key" );
        }

        cout << "*****************************************" << endl;
        cout << "set BES.KEY8 to val8" << endl;
        try {
            TheBESKeys::TheKeys()->set_key("BES.KEY8", "val8");
            found = false;
            TheBESKeys::TheKeys()->get_value("BES.KEY8", ret, found);
            CPPUNIT_ASSERT( ret == "val8" );
        }
        catch (BESError &e) {
            cerr << e.get_message();
            CPPUNIT_ASSERT( !"unable to set the key" );
        }

        cout << "*****************************************" << endl;
        cout << "add val_multi_5 to BES.KEY.MULTI" << endl;
        try {
            TheBESKeys::TheKeys()->set_key("BES.KEY.MULTI", "val_multi_5", true);
            found = false;
            vals.clear();
            TheBESKeys::TheKeys()->get_values("BES.KEY.MULTI", vals, found);
            CPPUNIT_ASSERT( found );
            CPPUNIT_ASSERT( vals.size() == 6 );
            for (int i = 0; i < 6; i++) {
                char val[32];
                sprintf(val, "val_multi_%d", i);
                cout << "looking for value " << val << endl;
                CPPUNIT_ASSERT( vals[i] == val );
            }

        }
        catch (BESError &e) {
            cerr << e.get_message();
            CPPUNIT_ASSERT( !"unable to set the key" );
        }

        cout << "*****************************************" << endl;
        cout << "get keys from reg exp include" << endl;
        found = false;
        vals.clear();
        TheBESKeys::TheKeys()->get_values("BES.KEY.MI", vals, found);
        CPPUNIT_ASSERT( found );
        CPPUNIT_ASSERT( vals.size() == 3 );
        CPPUNIT_ASSERT( vals[0] == "val_multi_2" );
        CPPUNIT_ASSERT( vals[1] == "val_multi_1" );
        CPPUNIT_ASSERT( vals[2] == "val_multi_3" );

        cout << "*****************************************" << endl;
        cout << "get keys" << endl;
        for (int i = 1; i < 7; i++) {
            char key[32];
            sprintf(key, "BES.KEY%d", i);
            char val[32];
            if (i == 5 || i == 6)
                sprintf(val, "");
            else
                sprintf(val, "val%d", i);
            cout << "looking for " << key << " with value " << val << endl;
            ret = "";
            TheBESKeys::TheKeys()->get_value(key, ret, found);
            CPPUNIT_ASSERT( found );
            CPPUNIT_ASSERT( ret == val );
        }

        cout << "*****************************************" << endl;
        cout << "Returning from keysT::run" << endl;
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( keysT );

int main(int argc, char*argv[])
{
    GetOpt getopt(argc, argv, "dDh");
    int option_char;
    while ((option_char = getopt()) != -1)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;
        case 'D':
            debug_2 = 1;
            break;

        case 'h': {     // help - show test names
            cerr << "Usage: keysT has the following tests:" << endl;
            const std::vector<Test*> &tests = keysT::suite()->getTests();
            unsigned int prefix_len = keysT::suite()->getName().append("::").length();
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
            test = keysT::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
