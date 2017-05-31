// convertTypeT.cc

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
using namespace std;

#include "config.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <fcntl.h>

#include <string>
#include <iostream>
#include <sstream>

using std::cout;
using std::endl;
using std::string;
using std::ostringstream;

#include "PPTStreamBuf.h"
#include "PPTProtocol.h"
#include <getopt.h>

static bool debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

string result =
    (string) "00001f4" + "d"
        + "<1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567"
        + "0000070" + "d"
        + "890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890>"
        + "0000000" + "d";

class sbT: public TestFixture {
private:

public:
    sbT()
    {
    }
    ~sbT()
    {
    }

    void setUp()
    {
    }

    void tearDown()
    {
    }

CPPUNIT_TEST_SUITE( sbT );

    CPPUNIT_TEST( do_test );

    CPPUNIT_TEST_SUITE_END()
    ;

    void do_test()
    {
        cout << endl << "*****************************************" << endl;
        cout << "Entered sbT::run" << endl;

        int fd = open("./sbT.out", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        PPTStreamBuf fds(fd, 500);
        std::streambuf *holder;
        holder = cout.rdbuf();
        cout.rdbuf(&fds);
        for (int u = 0; u < 51; u++) {
            cout << "<1234567890>";
        }
        fds.finish();
        cout.rdbuf(holder);
        close(fd);

        string str;
        int bytesRead = 0;
        fd = open("./sbT.out", O_RDONLY, S_IRUSR);
        char buffer[4096];
        while ((bytesRead = read(fd, (char *) buffer, 4096)) > 0) {
            buffer[bytesRead] = '\0';
            str += string(buffer);
        }
        close(fd);
        cout << "****" << endl << str << endl << "****" << endl;
        CPPUNIT_ASSERT( str == result );

        CPPUNIT_ASSERT( true );

        cout << endl << "*****************************************" << endl;
        cout << "Leaving sbT::run" << endl;
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION( sbT );

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
            cerr << "Usage: sbT has the following tests:" << endl;
            const std::vector<Test*> &tests = sbT::suite()->getTests();
            unsigned int prefix_len = sbT::suite()->getName().append("::").length();
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
            test = sbT::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}

