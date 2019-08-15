// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the OPeNDAP Back-End Server (BES)
// and embodies a whitelist of remote system that may be
// accessed by the server as part of it's routine operation.

// Copyright (c) 2018 OPeNDAP, Inc.
// Author: Nathan D. Potter <ndp@opendap.org>
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
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <string>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <streambuf>

#include <GetOpt.h>
#include <BESCatalog.h>
#include <BESCatalogDirectory.h>
#include <BESCatalogList.h>
#include <BESCatalogUtils.h>

#include <TheBESKeys.h>
#include <BESDebug.h>

#include "test_config.h"
#include "WhiteList.h"

using namespace std;
using namespace CppUnit;

static bool debug = false;
static bool bes_debug = false;

const string catalog_root_dir = BESUtil::assemblePath(TEST_SRC_DIR,"catalog_test");

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

class RemoteAccessTest: public TestFixture {
private:

    void show_file(string filename)
    {
        ifstream t(filename.c_str());

        if (t.is_open()) {
            string file_content((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());
            t.close();
            cout << endl << "##################################################################" << endl;
            cout << "file: " << filename << endl;
            cout << ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . " << endl;
            cout << file_content << endl;
            cout << ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . " << endl;
        }
    }

public:
    RemoteAccessTest()
    {
    }

    ~RemoteAccessTest()
    {
    }

    void setUp()
    {
        if (bes_debug) BESDebug::SetUp("cerr,all");

        string bes_conf = BESUtil::assemblePath(TEST_SRC_DIR,"remote_access_test.ini");

        TheBESKeys::ConfigFile = bes_conf;

        TheBESKeys::TheKeys()->set_key("BES.Catalog.catalog.RootDirectory",catalog_root_dir);

        try {
            BESCatalogList *tcl = BESCatalogList::TheCatalogList();
            if (!tcl->ref_catalog(BES_DEFAULT_CATALOG)) {
                tcl->add_catalog(new BESCatalogDirectory(BES_DEFAULT_CATALOG));
            }
        }
        catch (BESError &be) {
            cerr << endl << endl << "setUp() - OUCH! Could not initialize the BES Catalog! Message:  " << be.get_message() << endl;
        }

        if (bes_debug) show_file(bes_conf);
    }

    void tearDown()
    {
        // BESCatalogList::TheCatalogList()->deref_catalog(BES_DEFAULT_CATALOG);
    }

    CPPUNIT_TEST_SUITE( RemoteAccessTest );

    CPPUNIT_TEST(do_test);

    CPPUNIT_TEST_SUITE_END();

    bool can_access(string url)
    {
        if (debug) cout << "Checking remote access permission for url: '" << url << "' result: ";
        bool result = bes::WhiteList::get_white_list()->is_white_listed(url);
        if (debug) cout << (result ? "true" : "false") << endl;
        return result;
    }

    void do_test()
    {
        string catalog_root_url = "file://" + catalog_root_dir;

        CPPUNIT_ASSERT(!can_access("http://google.com"));

        CPPUNIT_ASSERT(can_access("http://test.opendap.org/opendap/data/nc/fnoc1.nc"));

        CPPUNIT_ASSERT(can_access("https://s3.amazonaws.com/somewhereovertherainbow/data/nc/fnoc1.nc"));
        CPPUNIT_ASSERT(!can_access("http://s3.amazonaws.com/somewhereovertherainbow/data/nc/fnoc1.nc"));

        CPPUNIT_ASSERT(can_access("http://thredds.ucar.edu/thredds/dodsC/data/nc/fnoc1.nc"));
        CPPUNIT_ASSERT(!can_access("https://thredds.ucar.edu/thredds/dodsC/data/nc/fnoc1.nc"));

        CPPUNIT_ASSERT(
            can_access("http://cloudydap.opendap.org/opendap/Arch-2/ebs/samples/3A-MO.GPM.GMI.GRID2014R1.20140601-S000000-E235959.06.V03A.h5"));

        CPPUNIT_ASSERT(!can_access("file:///foo"));

        CPPUNIT_ASSERT(can_access(BESUtil::assemblePath(catalog_root_url, "nc/fnoc1.nc")));

        CPPUNIT_ASSERT(!can_access("file://tmp/data/nc/fnoc1.nc"));
        CPPUNIT_ASSERT(!can_access("file:///etc/password"));
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(RemoteAccessTest);

int main(int argc, char*argv[])
{
    GetOpt getopt(argc, argv, "dhb");
    char option_char;
    while ((option_char = getopt()) != EOF)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;
        case 'b':
            bes_debug = 1;  // debug is a static global
            break;
        case 'h': {     // help - show test names
            cerr << "Usage: plistT has the following tests:" << endl;
            const vector<Test*> &tests = RemoteAccessTest::suite()->getTests();
            unsigned int prefix_len = RemoteAccessTest::suite()->getName().append("::").length();
            for (vector<Test*>::const_iterator i = tests.begin(), e = tests.end(); i != e; ++i) {
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
            test = RemoteAccessTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}

