// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the OPeNDAP Back-End Server (BES)
// and creqates an allowed hosts list od systems that may be
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

#include "config.h"

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <memory>
#include <string>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <streambuf>

#include <GetOpt.h>

#include "BESCatalog.h"
#include "BESCatalogDirectory.h"
#include "BESCatalogList.h"
#include "BESCatalogUtils.h"
#include "TheBESKeys.h"
#include "BESDebug.h"

#include "test_config.h"
#include "AllowedHosts.h"

using namespace std;
using namespace CppUnit;

static bool debug = false;
static bool bes_debug = false;

#define prolog std::string("AllowedHostsTest::").append(__func__).append("() - ")

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

class AllowedHostsTest: public TestFixture {
private:

    string d_catalog_root_dir;

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
    AllowedHostsTest()
    {
        d_catalog_root_dir = TEST_DATA_DIR;
    }

    ~AllowedHostsTest()
    {
    }

    void setUp()
    {
        if(debug) cerr << endl;
        if (bes_debug) BESDebug::SetUp("cerr,all");

        string bes_conf = BESUtil::assemblePath(TEST_SRC_DIR,"allowed_hosts_test.ini");

        TheBESKeys::ConfigFile = bes_conf;

        TheBESKeys::TheKeys()->set_key("BES.Catalog.catalog.RootDirectory",d_catalog_root_dir);

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

    bool can_access(const string &prlg, shared_ptr<http::url> &target_url)
    {
        if (debug) cout << prlg << "(" << prolog << ") Checking remote access permission for url: '" << target_url->str() << "' result: ";
        if(bes_debug) cerr << endl;
        bool result = http::AllowedHosts::theHosts()->is_allowed(target_url);
        if(bes_debug) cerr << prlg << "(" << prolog << ") is_allowed() returned: ";
        if (debug) cout << (result ? "true" : "false") << endl;
        return result;
    }

    void do_http_test()
    {
        shared_ptr<http::url> target_url;

        try {
            target_url = shared_ptr<http::url>(new http::url("http://google.com"));
            CPPUNIT_ASSERT(!can_access(prolog, target_url));

            target_url = shared_ptr<http::url>(new http::url("http://test.opendap.org/opendap/data/nc/fnoc1.nc"));
            CPPUNIT_ASSERT(can_access(prolog, target_url));

            target_url = shared_ptr<http::url>(new http::url("http://test.opendap.wrong.org/opendap/data/nc/fnoc1.nc"));
            CPPUNIT_ASSERT(!can_access(prolog, target_url));

            target_url = shared_ptr<http::url>(new http::url("https://s3.amazonaws.com/somewhereovertherainbow/data/nc/fnoc1.nc"));
            CPPUNIT_ASSERT(can_access(prolog, target_url));

            target_url = shared_ptr<http::url>(new http::url("http://s3.amazonaws.com/somewhereovertherainbow/data/nc/fnoc1.nc"));
            CPPUNIT_ASSERT(!can_access(prolog, target_url));

            target_url = shared_ptr<http::url>(new http::url("http://thredds.ucar.edu/thredds/dodsC/data/nc/fnoc1.nc"));
            CPPUNIT_ASSERT(can_access(prolog, target_url));

            target_url = shared_ptr<http::url>(new http::url("https://thredds.ucar.edu/thredds/dodsC/data/nc/fnoc1.nc"));
            CPPUNIT_ASSERT(can_access(prolog, target_url));

            string turl_str = "http://cloudydap.opendap.org/opendap/Arch-2/ebs/samples/3A-MO.GPM.GMI.GRID2014R1.20140601-S000000-E235959.06.V03A.h5";
            target_url = shared_ptr<http::url>(new http::url(turl_str));
            CPPUNIT_ASSERT(can_access(prolog, target_url));

            string ghrc_uat_s3 = "https://ghrcwuat-protected.s3.us-west-2.amazonaws.com/rss/rssmif16d__7/f16_ssmis_20200512v7.nc?A-userid=hyrax&X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=ASIASF4AWSMOOMOO0200901%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20200901T153129Z&X-Amz-Expires=86400&X-Amz-Security-Token=FwoGZXAWSMOOMOOMOOMOOKHf6ew%3D%3D&X-Amz-SignedHeaders=host&X-Amz-Signature=10e80b9876AWSMOOMOOMOOOO9f26174bca4fab";
            target_url = shared_ptr<http::url>(new http::url(ghrc_uat_s3));
            CPPUNIT_ASSERT(can_access(prolog, target_url));

        }
        catch(BESError &berr){
            stringstream msg;
            msg << "Caught BESError! " << berr.get_verbose_message();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch(std::exception &e){
            stringstream msg;
            msg << "Caught std::exception! " << e.what();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }


    }


    void do_file_test()
    {
        string catalog_root_url = "file://" + d_catalog_root_dir;
        string turl_str;

        // Rejected Does not exist in catalog_root
        shared_ptr<http::url> file_url;

        try {
            // AN absolute path to a node outside of the BES.Catalog.catalog.RootDirectory tree must be rejected,
            // access is not allowed.
            file_url = shared_ptr<http::url>(new http::url("file:///foo"));
            if(debug) cerr << prolog << "Checking url: " << file_url->str() << endl;
            CPPUNIT_ASSERT(!can_access(prolog, file_url));

            // THis is a relative url for an existing file in the BES.Catalog.catalog.RootDirectory, access is allowed.
            file_url = shared_ptr<http::url>(new http::url("file://csv/temperature.csv"));
            if(debug) cerr << prolog << "Checking url: " << file_url->str() << endl;
            CPPUNIT_ASSERT(can_access(prolog, file_url));

            // This is a fully qualified path to an existing file that in the BES.Catalog.catalog.RootDirectory tree
            // so access is allowed.
            turl_str = BESUtil::assemblePath(catalog_root_url, "csv/temperature.csv");
            file_url = shared_ptr<http::url>(new http::url(turl_str));
            if(debug) cerr << prolog << "Checking url: " << file_url->str() << endl;
            CPPUNIT_ASSERT(can_access(prolog, file_url));

            // this is a relative path to nothing, it will fail because the test needs to check if the server
            // has read permission for the file ala lstat() - access is not allowed.
            file_url = shared_ptr<http::url>(new http::url("file://tmp/data/nc/fnoc1.nc"));
            if(debug) cerr << prolog << "Checking url: " << file_url->str() << endl;
            CPPUNIT_ASSERT(!can_access(prolog, file_url));

            // This absolute path is outside of the BES.Catalog.catalog.RootDirectory tree and must be rejected,
            // access is not allowed
            file_url = shared_ptr<http::url>(new http::url("file:///etc/password"));
            if(debug) cerr << prolog << "Checking url: " << file_url->str() << endl;
            CPPUNIT_ASSERT(!can_access(prolog, file_url));
        }
        catch(BESError &berr){
            stringstream msg;
            msg << "Caught BESError! " << berr.get_verbose_message();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch(std::exception &e){
            stringstream msg;
            msg << "Caught std::exception! " << e.what();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }

    }

    void bad_url_test(){

        try {
            auto target_url = shared_ptr<http::url>(new http::url());
            can_access(prolog, target_url);
            CPPUNIT_FAIL("Bad (empty) URL should have thrown a BESInternalError!");
        }
        catch(BESInternalError &berr){
            CPPUNIT_ASSERT("Bad (empty) URL Failed as expected.");
            if(debug) cerr << endl;
        }
        try {
            auto target_url = shared_ptr<http::url>(new http::url("s3://cloudydap/airs/somefile.nc"));
            can_access(prolog, target_url);
            CPPUNIT_FAIL("Bad (s3://) URL should have thrown a BESInternalError!");
        }
        catch(BESInternalError &berr){
            CPPUNIT_ASSERT("Bad (s3://) URL Failed as expected.");
            if(debug) cerr << endl;
        }
        try {
            auto target_url = shared_ptr<http::url>(new http::url("dods://localhost:8080/opendap/data/nc/fnoc1.nc"));
            can_access(prolog, target_url);
            CPPUNIT_FAIL("Bad (dods://) URL should have thrown a BESInternalError!");
        }
        catch(BESInternalError &berr){
            CPPUNIT_ASSERT("Bad (dods://) URL Failed as expected.");
            if(debug) cerr << endl;
        }
    }

    void trusted_url_test() {

        // There is no such thing as a "trusted" file URL other than only allowing access to things
        // that are within the BES.Catalog.catalog.RootDirectory tree.
        auto target_url = shared_ptr<http::url>(new http::url("file:///etc/password", true));
        if(debug) cerr << prolog << "Checking url: " << target_url->str() << endl;
        CPPUNIT_ASSERT(!can_access(prolog, target_url));

        // Marking this url trusted=false (default) will still allow access because it comes from an allowed host.
        target_url = shared_ptr<http::url>(new http::url("http://test.opendap.org/opendap/data/nc/fnoc1.nc",false));
        CPPUNIT_ASSERT(can_access(prolog, target_url));

        // Marking this url trusted=false (default) will block access because it does not comes from an allowed host.
        target_url = shared_ptr<http::url>(new http::url("http://test.opendap.WRONG.org/opendap/data/nc/fnoc1.nc",false));
        CPPUNIT_ASSERT(!can_access(prolog, target_url));

        // Marking this url trusted=true (default) will allow access because it's "trusted".
        target_url = shared_ptr<http::url>(new http::url("http://test.opendap.WRONG.org/opendap/data/nc/fnoc1.nc",true));
        CPPUNIT_ASSERT(can_access(prolog, target_url));

    }

    CPPUNIT_TEST_SUITE( AllowedHostsTest );

        CPPUNIT_TEST(do_http_test);
        CPPUNIT_TEST(do_file_test);
        CPPUNIT_TEST(bad_url_test);
        CPPUNIT_TEST(trusted_url_test);

    CPPUNIT_TEST_SUITE_END();


};

CPPUNIT_TEST_SUITE_REGISTRATION(AllowedHostsTest);

int main(int argc, char*argv[])
{
    GetOpt getopt(argc, argv, "dhb");
    int option_char;
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
            const vector<Test*> &tests = AllowedHostsTest::suite()->getTests();
            unsigned int prefix_len = AllowedHostsTest::suite()->getName().append("::").length();
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
            test = AllowedHostsTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}

