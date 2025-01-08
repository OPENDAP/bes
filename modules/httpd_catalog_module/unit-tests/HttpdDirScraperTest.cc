// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES component of the Hyrax Data Server.

// Copyright (c) 2018 OPeNDAP, Inc.
// Author: Nathan Potter <ndp@opendap.org>
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

#include <memory>
#include <cstdio>
#include <cstring>
#include <iostream>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <unistd.h>
#include <libdap/util.h>

#include <BESError.h>
#include <BESDebug.h>
#include <BESUtil.h>
#include <BESCatalogList.h>
#include <TheBESKeys.h>
#include <CatalogNode.h>
#include <CatalogItem.h>
#include "test_config.h"

#include "RemoteResource.h"
#include "HttpNames.h"
#include "../HttpdDirScraper.h"
#include "../HttpdCatalogNames.h"


using namespace std;

static bool debug = false;
static bool Debug = false;
static bool bes_debug = false;

#undef DBG
#define DBG(x) do { if (debug) x; } while(false)

#define prolog std::string("HttpdDirScraperTest::").append(__func__).append("() - ")

namespace httpd_catalog {

class HttpdDirScraperTest: public CppUnit::TestFixture {
private:

    void show_file(string filename)
    {
        ifstream t(filename.c_str());

        if (t.is_open()) {
            string file_content((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());
            t.close();
            cerr << endl << prolog << "BEGIN  file: " << filename << endl;
            cerr << file_content << endl;
            cerr << prolog << "END file: " << filename << endl;
        }
        else {
            cerr << "FAILED TO OPEN FILE: " << filename << endl;
        }
    }

    std::string get_file_as_string(string filename)
    {
        ifstream t(filename.c_str());

        if (t.is_open()) {
            string file_content((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());
            t.close();
            if(Debug) cerr << endl << prolog << "BEGIN file: " << filename << endl;
            if(Debug) cerr << prolog << file_content << endl;
            if(Debug) cerr << prolog << "END file: " << filename << endl;
            return file_content;
        }
        else {
            cerr << prolog << "FAILED TO OPEN FILE: " << filename << endl;
            CPPUNIT_ASSERT(false);
            return "";
        }
    }

    /**
     *
     */
    string get_data_file_url(string name){

        string data_file = BESUtil::pathConcat(d_data_dir,name);
        if(debug) cerr << prolog << "data_file: " << data_file << endl;
        if(Debug) show_file(data_file);

        string data_file_url = "file://" + data_file;
        if(debug) cerr << prolog << "data_file_url: " << data_file_url << endl;

        return data_file_url;
    }


public:
    string d_data_dir;
    // Called once before everything gets tested
    HttpdDirScraperTest()
    {
        d_data_dir = BESUtil::assemblePath(TEST_DATA_DIR,"httpd_dirs");
        if(debug) cerr << "data_dir: " << d_data_dir << endl;
    }

    // Called at the end of the test
    ~HttpdDirScraperTest()
    {
    }

    // Called before each test
    void setUp()
    {
        if(debug) cerr << endl;
        if(Debug) cerr << prolog << "BEGIN" << endl;
        string bes_conf = BESUtil::assemblePath(TEST_BUILD_DIR,"bes.conf");
        if(Debug) cerr << prolog << "Using BES configuration: " << bes_conf << endl;

        TheBESKeys::ConfigFile = bes_conf;

        if (bes_debug) BESDebug::SetUp(string("cerr,http,").append(MODULE));

        if (bes_debug) show_file(bes_conf);

#if 0
        if(purge_cache){
            if(Debug) cerr << prolog << "Purging cache!" << endl;
            string cache_dir;
            bool found;
            TheBESKeys::TheKeys()->get_value(HTTP_CACHE_DIR_KEY,cache_dir,found);
            if(found){
                if(Debug) cerr << prolog << HTTP_CACHE_DIR_KEY << ": " <<  cache_dir << endl;
                if(Debug) cerr << prolog << "Purging " << cache_dir << endl;
                string cmd = "exec rm -r "+ BESUtil::assemblePath(cache_dir,"/*");
                system(cmd.c_str());
            }
        }
#endif

        if(Debug) cerr << "setUp() - END" << endl;
    }

    // Called after each test
    void tearDown()
    {
    }


/*##################################################################################################*/
/* TESTS BEGIN */


    void get_remote_node_test() {

        // note: the following path must end with "/" in order for the scraper to think
        // it's a catalog/directory link and not an item or file
        string url = "http://test.opendap.org/data/httpd_catalog/";
        HttpdDirScraper hds;
        bes::CatalogNode *node = nullptr;
        unsigned int expected_node_count = 2;
        unsigned int expected_leaf_count = 2;

        try {
            if(debug) cerr << prolog << "Scraping '" << url << "'" << endl;

            node = hds.get_node(url,"/data/httpd_catalog/");
            if(debug) {

                if(debug) cerr << prolog << "Found " <<  node->get_node_count() << " nodes. " <<
                        "Expected: " << expected_node_count << endl;
                unsigned long i = 0;
                auto it = node->nodes_begin();
                while(it != node->nodes_end()){
                    bes::CatalogItem *node = *it++;
                    cerr << prolog << "    Node["<< i << "]: " << node->get_name() << endl;
                }

                cerr << prolog << "Found " << node->get_leaf_count() << " leaves. " <<
                     "Expected: " << expected_leaf_count << endl;
                it = node->leaves_begin();
                i = 0;
                while(it != node->leaves_end()){
                    bes::CatalogItem *leaf = *it++;
                    cerr << prolog << "    Leaf["<< i << "]: " << leaf->get_name() << endl;
                }
            }

            // Node items...
            CPPUNIT_ASSERT(node->get_node_count() == expected_node_count);
            bes::CatalogNode::item_iter it = node->nodes_begin();
            bes::CatalogItem *first_node = *it++;
            if(debug) cerr << prolog << "first_node: " << first_node->get_name() << endl;
            CPPUNIT_ASSERT(first_node->get_name() == "subdir1");

            bes::CatalogItem *second_node = *it;
            if(debug) cerr << prolog << "second_node: " << second_node->get_name() << endl;
            CPPUNIT_ASSERT(second_node->get_name() == "subdir2");

            // Leaf items...
            CPPUNIT_ASSERT(node->get_leaf_count() == expected_leaf_count);
            it = node->leaves_begin();
            bes::CatalogItem *first_leaf = *it++;
            if(debug) cerr << prolog << "first_leaf: " << first_leaf->get_name() << endl;
            CPPUNIT_ASSERT(first_leaf->get_name() == "READTHIS");

            bes::CatalogItem *second_leaf = *it;
            if(debug) cerr << prolog << "second_leaf: " << second_leaf->get_name() << endl;
            CPPUNIT_ASSERT(second_leaf->get_name() == "fnoc1.nc");

        }
        catch (BESError &besE){
            cerr << "Caught BESError! message: " << besE.get_verbose_message() << " type: " << besE.get_bes_error_type() << endl;
        }
        catch (libdap::Error &le){
            cerr << "Caught libdap::Error! message: " << le.get_error_message() << " code: "<< le.get_error_code() << endl;
        }
        delete node;
    }

    void get_file_node_test() {

        // note: the following path must end with "/" in order for the scraper to think
        // it's a catalog/directory link and not an item or file (even though it is a file...)
        string url = get_data_file_url("too.data.http_catalog/");

        HttpdDirScraper hds;
        bes::CatalogNode *node = 0;
        try {
            if(debug) cerr << prolog << "Scraping '" << url << "'" << endl;
            node = hds.get_node(url,"/data/httpd_catalog/");
            if(debug) cerr << prolog << "Found " <<  node->get_leaf_count() << " leaves and " << node->get_node_count() << " nodes." << endl;

            // Node items...
            CPPUNIT_ASSERT(node->get_node_count() == 2);
            bes::CatalogNode::item_iter it = node->nodes_begin();
            bes::CatalogItem *first_node = *it++;
            if(debug) cerr << prolog << "first_node: " << first_node->get_name() << endl;
            CPPUNIT_ASSERT(first_node->get_name() == "subdir1");

            bes::CatalogItem *second_node = *it;
            if(debug) cerr << prolog << "second_node: " << second_node->get_name() << endl;
            CPPUNIT_ASSERT(second_node->get_name() == "subdir2");

            // Leaf items...
            CPPUNIT_ASSERT(node->get_leaf_count() == 2);
            it = node->leaves_begin();
            bes::CatalogItem *first_leaf = *it++;
            if(debug) cerr << prolog << "first_leaf: " << first_leaf->get_name() << endl;
            CPPUNIT_ASSERT(first_leaf->get_name() == "READTHIS");

            bes::CatalogItem *second_leaf = *it;
            if(debug) cerr << prolog << "second_leaf: " << second_leaf->get_name() << endl;
            CPPUNIT_ASSERT(second_leaf->get_name() == "fnoc1.nc");

        }
        catch (BESError &besE){
            cerr << "Caught BESError! message: " << besE.get_verbose_message() << " type: " << besE.get_bes_error_type() << endl;
        }
        catch (libdap::Error &le){
            cerr << "Caught libdap::Error! message: " << le.get_error_message() << " code: "<< le.get_error_code() << endl;
        }
        delete node;
    }


/* TESTS END */
/*##################################################################################################*/


    CPPUNIT_TEST_SUITE( HttpdDirScraperTest );

    CPPUNIT_TEST(get_remote_node_test);
    CPPUNIT_TEST(get_file_node_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(HttpdDirScraperTest);

} // namespace httpd_catalog

int main(int argc, char*argv[])
{
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    int option_char;
    while ((option_char = getopt(argc, argv, "dbD")) != -1)
        switch (option_char) {
        case 'd':
            debug = true;  // debug is a static global
            cerr << "debug enabled" << endl;
            break;
        case 'D':
            Debug = true;  // Debug is a static global
            cerr << "Debug enabled" << endl;
            break;
        case 'b':
            bes_debug = true;  // debug is a static global
            cerr << "bes_debug enabled" << endl;
            break;
#if 0
            case 'P':
            purge_cache = true;  // debug is a static global
            cerr << "purge_cache enabled" << endl;
            break;
#endif
        default:
            break;
        }

    argc -= optind;
    argv += optind;

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
            test = httpd_catalog::HttpdDirScraperTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
