// fsT.C

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
#include <fstream>
#include <iostream>
#include <map>
#include <string>

using std::cerr;
using std::cout;
using std::endl;
using std::ifstream;
using std::make_pair;
using std::map;
using std::string;

#include "BESFSDir.h"
#include "BESFSFile.h"
#include "test_config.h"
#include <unistd.h>

static bool debug = false;

#undef DBG
#define DBG(x)                                                                                                         \
    do {                                                                                                               \
        if (debug)                                                                                                     \
            (x);                                                                                                       \
    } while (false);

class fsT : public TestFixture {
private:
    map<string, string> fileList;

public:
    fsT() {}
    ~fsT() {}

    void setUp() {
        fileList.insert(make_pair("agglistT.cc", "agglistT.cc"));
        fileList.insert(make_pair("bz2T.cc", "bz2T.cc"));
        fileList.insert(make_pair("cacheT.cc", "cacheT.cc"));
        fileList.insert(make_pair("catT.cc", "catT.cc"));
        fileList.insert(make_pair("checkT.cc", "checkT.cc"));
        fileList.insert(make_pair("constraintT.cc", "constraintT.cc"));
        fileList.insert(make_pair("containerT.cc", "containerT.cc"));
        fileList.insert(make_pair("debugT.cc", "debugT.cc"));
        fileList.insert(make_pair("defT.cc", "defT.cc"));
        fileList.insert(make_pair("encodeT.cc", "encodeT.cc"));
        fileList.insert(make_pair("fsT.cc", "fsT.cc"));
        fileList.insert(make_pair("gzT.cc", "gzT.cc"));
        fileList.insert(make_pair("infoT.cc", "infoT.cc"));
        fileList.insert(make_pair("initT.cc", "initT.cc"));
        fileList.insert(make_pair("keysT.cc", "keysT.cc"));
        fileList.insert(make_pair("lockT.cc", "lockT.cc"));
        fileList.insert(make_pair("pfileT.cc", "pfileT.cc"));
        fileList.insert(make_pair("plistT.cc", "plistT.cc"));
        fileList.insert(make_pair("pvolT.cc", "pvolT.cc"));
        fileList.insert(make_pair("regexT.cc", "regexT.cc"));
        fileList.insert(make_pair("replistT.cc", "replistT.cc"));
        fileList.insert(make_pair("reqhandlerT.cc", "reqhandlerT.cc"));
        fileList.insert(make_pair("reqlistT.cc", "reqlistT.cc"));
        fileList.insert(make_pair("resplistT.cc", "resplistT.cc"));
        fileList.insert(make_pair("scrubT.cc", "scrubT.cc"));
        fileList.insert(make_pair("servicesT.cc", "servicesT.cc"));
        fileList.insert(make_pair("uncompressT.cc", "uncompressT.cc"));
        fileList.insert(make_pair("urlT.cc", "urlT.cc"));
        fileList.insert(make_pair("utilT.cc", "utilT.cc"));
        fileList.insert(make_pair("zT.cc", "zT.cc"));
    }

    void tearDown() {}

    CPPUNIT_TEST_SUITE(fsT);

    CPPUNIT_TEST(do_test);

    CPPUNIT_TEST_SUITE_END();

    void do_test() {
        cout << "*****************************************" << endl;
        cout << "Entered fsT::run" << endl;

        BESFSDir fsd("./", ".*T.cc$");
        BESFSDir::fileIterator i = fsd.beginOfFileList();
        BESFSDir::fileIterator e = fsd.endOfFileList();
        for (; i != e; i++) {
            map<string, string>::iterator f = fileList.find((*i).getFileName());
            CPPUNIT_ASSERT(f != fileList.end());
            CPPUNIT_ASSERT((*i).getDirName() == "./");
        }

        {
            BESFSFile fsf("/some/path/to/a/file/with/extension/fnoc1.nc");
            CPPUNIT_ASSERT(fsf.getFullPath() == "/some/path/to/a/file/with/extension/fnoc1.nc");
            CPPUNIT_ASSERT(fsf.getDirName() == "/some/path/to/a/file/with/extension");
            CPPUNIT_ASSERT(fsf.getBaseName() == "fnoc1");
            CPPUNIT_ASSERT(fsf.getFileName() == "fnoc1.nc");
            CPPUNIT_ASSERT(fsf.getExtension() == "nc");
            string reason;
            CPPUNIT_ASSERT(fsf.exists(reason) == false);
            CPPUNIT_ASSERT(fsf.isReadable(reason) == false);
            CPPUNIT_ASSERT(fsf.isWritable(reason) == false);
            CPPUNIT_ASSERT(fsf.isExecutable(reason) == false);
            CPPUNIT_ASSERT(fsf.hasDotDot() == false);
        }

        {
            BESFSFile fsf("./fsT");
            CPPUNIT_ASSERT(fsf.getFullPath() == "./fsT");
            CPPUNIT_ASSERT(fsf.getDirName() == ".");
            CPPUNIT_ASSERT(fsf.getBaseName() == "fsT");
            CPPUNIT_ASSERT(fsf.getExtension().empty());
            string reason;
            CPPUNIT_ASSERT(fsf.exists(reason) == true);
            CPPUNIT_ASSERT(fsf.isReadable(reason) == true);
            CPPUNIT_ASSERT(fsf.isWritable(reason) == true);
            CPPUNIT_ASSERT(fsf.isExecutable(reason) == true);
            CPPUNIT_ASSERT(fsf.hasDotDot() == false);
        }

        {
            BESFSFile fsf("/some/dir/../parent/dir/fnoc1.nc");
            CPPUNIT_ASSERT(fsf.getFullPath() == "/some/dir/../parent/dir/fnoc1.nc");
            CPPUNIT_ASSERT(fsf.getDirName() == "/some/dir/../parent/dir");
            CPPUNIT_ASSERT(fsf.getBaseName() == "fnoc1");
            CPPUNIT_ASSERT(fsf.getFileName() == "fnoc1.nc");
            CPPUNIT_ASSERT(fsf.getExtension() == "nc");
            string reason;
            CPPUNIT_ASSERT(fsf.exists(reason) == false);
            CPPUNIT_ASSERT(fsf.isReadable(reason) == false);
            CPPUNIT_ASSERT(fsf.isWritable(reason) == false);
            CPPUNIT_ASSERT(fsf.isExecutable(reason) == false);
            CPPUNIT_ASSERT(fsf.hasDotDot() == true);
        }

        cout << "*****************************************" << endl;
        cout << "Returning from fsT::run" << endl;
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(fsT);

int main(int argc, char *argv[]) {
    int option_char;
    while ((option_char = getopt(argc, argv, "dh")) != EOF)
        switch (option_char) {
        case 'd':
            debug = 1; // debug is a static global
            break;
        case 'h': { // help - show test names
            cerr << "Usage: fsT has the following tests:" << endl;
            const std::vector<Test *> &tests = fsT::suite()->getTests();
            unsigned int prefix_len = fsT::suite()->getName().append("::").size();
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
            test = fsT::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
