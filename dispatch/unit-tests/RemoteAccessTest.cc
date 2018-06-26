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

#include <string>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <streambuf>


using std::cerr;
using std::cout;
using std::endl;

#include "RemoteAccess.h"
#include <TheBESKeys.h>
#include <test_config.h>
#include <GetOpt.h>

static bool debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

class plistT: public TestFixture {
private:

    void show_file(std::string filename){
        cout << endl << "##################################################################" << endl;
        cout << "file: " << filename << endl;
        cout << ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . " << endl;
        std::ifstream t(filename);
        std::string file_content((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
        cout << file_content << endl;
        cout << ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . " << endl;

    }
public:
    plistT()
    {
    }
    ~plistT()
    {
    }

    void setUp()
    {

//        Gateway.Whitelist=http://localhost
//        Gateway.Whitelist+=http://test.opendap.org/opendap/
//        Gateway.Whitelist+=http://cloudydap.opendap.org/opendap/
//        Gateway.Whitelist+=http://thredds.ucar.edu/thredds/
//        Gateway.Whitelist+=https://s3.amazonaws.com/somewhereovertherainbow/

        std::string bes_conf = (std::string) TEST_SRC_DIR + "/remote_access_test.ini";
        TheBESKeys::ConfigFile = bes_conf;

        if(debug) show_file(bes_conf);
    }

    void tearDown()
    {
    }

    CPPUNIT_TEST_SUITE( plistT );

    CPPUNIT_TEST( do_test );

    CPPUNIT_TEST_SUITE_END();


    bool can_access(std::string url){
        if(debug) cout << "Checking remote access permission for url: '" << url << "' result: ";
        bool result =  bes::RemoteAccess::Is_Whitelisted(url);
        if(debug) cout << (result?"true":"false") << endl;
        return result;
    }

    void do_test()
    {
        // bes::RemoteAccess::Initialize();

        CPPUNIT_ASSERT( !can_access("http://google.com") );

        CPPUNIT_ASSERT( can_access("http://test.opendap.org/opendap/data/nc/fnoc1.nc") );

        CPPUNIT_ASSERT( can_access("https://s3.amazonaws.com/somewhereovertherainbow/data/nc/fnoc1.nc") );
        CPPUNIT_ASSERT( !can_access("http://s3.amazonaws.com/somewhereovertherainbow/data/nc/fnoc1.nc") );

        CPPUNIT_ASSERT( can_access("http://thredds.ucar.edu/thredds/dodsC/data/nc/fnoc1.nc") );
        CPPUNIT_ASSERT( !can_access("https://thredds.ucar.edu/thredds/dodsC/data/nc/fnoc1.nc") );

        CPPUNIT_ASSERT( can_access("http://cloudydap.opendap.org/opendap/Arch-2/ebs/samples/3A-MO.GPM.GMI.GRID2014R1.20140601-S000000-E235959.06.V03A.h5") );






    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( plistT );

int main(int argc, char*argv[])
{

    GetOpt getopt(argc, argv, "dh");
    char option_char;
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

