// complete_catalog_lister.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2018 OPeNDAP, Inc.
// Author: James Gallagher jgallagher@opendap.org>
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

#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <sstream>

#include <sys/stat.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

// #include <GetOpt.h>

#include "BESCatalog.h"
#include "BESCatalogDirectory.h"
#include "BESCatalogEntry.h"
#include "BESCatalogList.h"
#include "BESCatalogResponseHandler.h"
#include "BESDataHandlerInterface.h"
#include "BESDataNames.h"
#include "BESDefaultModule.h"
#include "BESError.h"
#include "BESNames.h"
#include "BESXMLInfo.h"
#include "TheBESKeys.h"

#include "../../../dispatch/unit-tests/test_config.h"
#include "../../../dispatch/unit-tests/test_utils.h"

static bool debug = false;
#undef DBG
#define DBG(x)                                                                                                         \
    do {                                                                                                               \
        if (debug)                                                                                                     \
            (x);                                                                                                       \
    } while (false);

using namespace CppUnit;

using std::cerr;
using std::cout;
using std::endl;
using std::ostringstream;

const string root_dir = "/catalog_test";

class complete_catalog_lister : public TestFixture {
private:
public:
    complete_catalog_lister() {}
    ~complete_catalog_lister() {}

    void setUp() {
        string bes_conf = (string)TEST_SRC_DIR + "/bes.conf";
        TheBESKeys::ConfigFile = bes_conf;
        TheBESKeys::TheKeys()->set_key("BES.Data.RootDirectory=/dev/null");
        TheBESKeys::TheKeys()->set_key("BES.Info.Buffered=no");
        TheBESKeys::TheKeys()->set_key("BES.Info.Type=xml");
        try {
            BESDefaultModule::initialize(0, 0);
        } catch (BESError &e) {
            cerr << e.get_message() << endl;
            throw e;
        }
    }

    void tearDown() {}

    CPPUNIT_TEST_SUITE(complete_catalog_lister);

    CPPUNIT_TEST(root_dir_test1);

    CPPUNIT_TEST_SUITE_END();

    // This test should be broken up into smaller pieces. jhrg 8/23/17
    void root_dir_test1() {
        TheBESKeys::TheKeys()->set_key(
            string("BES.Catalog.default.RootDirectory=").append(TEST_SRC_DIR).append(root_dir));
        TheBESKeys::TheKeys()->set_key("BES.Catalog.default.TypeMatch=conf:conf&;");
#if 0
        TheBESKeys::TheKeys()->set_key("BES.Catalog.default.Include=.*file.*$;");
#endif
        TheBESKeys::TheKeys()->set_key("BES.Catalog.default.Exclude=.*\\.ini$;");

        BESCatalogList::TheCatalogList()->add_catalog(new BESCatalogDirectory("default"));

        BESCatalog *catobj = BESCatalogList::TheCatalogList()->find_catalog("default");
        cerr << "The default catalog: ";
        catobj->dump(cerr);

        cerr << "Number of catalogs: " << BESCatalogList::TheCatalogList()->num_catalogs() << endl;
#if 0
        BESCatalogEntry *entry = BESCatalogList::TheCatalogList()->show_catalogs(0);

        cerr << "The root node of the BES catalogs: ";
        entry->dump(cerr);
#endif

        string pathname = "/";
        BESCatalogEntry *entry = catobj->show_catalog(pathname, 0);
        cerr << "The root node of the default catalog: ";
        entry->dump(cerr);

        // Use this to find child nodes and build the tree
        BESCatalogEntry::catalog_citer i = entry->get_beginning_entry();
        BESCatalogEntry::catalog_citer e = entry->get_ending_entry();
        for (; i != e; ++i) {
            BESCatalogEntry *e = (*i).second;
            if (e->is_collection()) {
                cerr << "Found collection: " << e->get_name() << endl;
                (void)catobj->show_catalog(pathname.append(e->get_name()), e);
            } else {
                cerr << "Leaf: " << e->get_name() << ", dumping:" << endl;
                e->dump(cerr);
            }
        }

        cerr << "After traversal, the default catalog:";
        entry->dump(cerr);
    }

#if 0
        DBG(cerr << "now try it with BESCatalogResponseHandler" << endl);
        try {
            BESDataHandlerInterface dhi;
            dhi.data[CATALOG_OR_INFO] = CATALOG_RESPONSE;
            BESCatalogResponseHandler handler("catalog");
            handler.execute(dhi);

            BESInfo *info = dynamic_cast<BESInfo *>(handler.get_response_object());
            CPPUNIT_ASSERT(info);

            ostringstream strm;
            info->print(strm);
            string strm_s = strm.str();
            string str = remove(strm_s, "lastModified", 0);
            str = remove(str, "size", 0);

            string cat_root = read_test_baseline(string(TEST_SRC_DIR) + "/catalog_test_baselines/cat_root.txt");

            DBG(cerr << "baseline: " << cat_root << endl);
            DBG(cerr << "response: " << str << endl);

            CPPUNIT_ASSERT(str == cat_root);
        }
        catch (BESError &e) {
            DBG(cerr << e.get_message() << endl);
            CPPUNIT_FAIL("Failed to show catalogs");
        }

        DBG(cerr << "add good catalog" << endl);
        var = (string) "BES.Catalog.other.RootDirectory=" + TEST_SRC_DIR + root_dir;
        TheBESKeys::TheKeys()->set_key(var);
        var = (string) "BES.Catalog.other.TypeMatch=info:info&;";
        TheBESKeys::TheKeys()->set_key(var);
        var = (string) "BES.Catalog.other.Include=.*file.*$;";
        TheBESKeys::TheKeys()->set_key(var);
        var = (string) "BES.Catalog.other.Exclude=\\..*;README;";
        TheBESKeys::TheKeys()->set_key(var);
        BESCatalog *other = 0;
        try {
            other = new BESCatalogDirectory("other");
            BESCatalogList::TheCatalogList()->add_catalog(other);
            CPPUNIT_ASSERT("Succeeded in adding other catalog");
        }
        catch (BESError &e) {
            DBG(cerr << e.get_message() << endl);
            CPPUNIT_FAIL("Failed to add catalog");
        }
        catobj = BESCatalogList::TheCatalogList()->find_catalog("other");
        CPPUNIT_ASSERT(catobj);
        CPPUNIT_ASSERT(catobj == other);
        numcats = BESCatalogList::TheCatalogList()->num_catalogs();
        CPPUNIT_ASSERT(numcats == 2);

        DBG(cerr << "*****************************************" << endl);
        DBG(cerr << "now try it with BESCatalogResponseHandler" << endl);
        try {
            BESDataHandlerInterface dhi;
            dhi.data[CATALOG_OR_INFO] = CATALOG_RESPONSE;
            BESCatalogResponseHandler handler("catalog");
            handler.execute(dhi);
            BESInfo *info = dynamic_cast<BESInfo *>(handler.get_response_object());
            CPPUNIT_ASSERT(info);
            ostringstream strm;
            info->print(strm);
            string strm_s = strm.str();
            string str = remove(strm_s, "lastModified", 0);
            str = remove(str, "size", 0);

            string two_response = read_test_baseline(string(TEST_SRC_DIR) + "/catalog_test_baselines/two_response.txt");
            DBG(cerr << "baseline: " << two_response << endl);
            DBG(cerr << "response: " << str << endl);

            CPPUNIT_ASSERT(str == two_response);
        }
        catch (BESError &e) {
            DBG(cerr << e.get_message() << endl);
            CPPUNIT_FAIL("Failed to show catalogs");
        }

        DBG(cerr << "*****************************************" << endl);
        DBG(cerr << "BESCatalogResponseHandler with catalog response" << endl);
        try {
            BESDataHandlerInterface dhi;
            dhi.data[CONTAINER] = "other";
            dhi.data[CATALOG_OR_INFO] = CATALOG_RESPONSE;
            BESCatalogResponseHandler handler("catalog");
            handler.execute(dhi);
            BESInfo *info = dynamic_cast<BESInfo *>(handler.get_response_object());
            CPPUNIT_ASSERT(info);
            ostringstream strm;
            info->print(strm);
            string strm_s = strm.str();
            string str = remove(strm_s, "lastModified", 0);
            str = remove(str, "size", 0);

            string other_root = read_test_baseline(string(TEST_SRC_DIR) + "/catalog_test_baselines/other_root.txt");
            DBG(cerr << "baseline: " << other_root << endl);
            DBG(cerr << "response: " << str << endl);

            CPPUNIT_ASSERT(str == other_root);
        }
        catch (BESError &e) {
            DBG(cerr << e.get_message() << endl);
            CPPUNIT_FAIL("Failed to show catalogs");
        }

        DBG(cerr << "*****************************************" << endl);
        DBG(cerr << "BESCatalogResponseHandler with info response" << endl);
        try {
            BESDataHandlerInterface dhi;
            dhi.data[CONTAINER] = "other";
            dhi.data[CATALOG_OR_INFO] = SHOW_INFO_RESPONSE;
            BESCatalogResponseHandler handler("catalog");
            handler.execute(dhi);
            BESInfo *info = dynamic_cast<BESInfo *>(handler.get_response_object());
            CPPUNIT_ASSERT(info);
            ostringstream strm;
            info->print(strm);
            string strm_s = strm.str();
            string str = remove(strm_s, "lastModified", 0);
            str = remove(str, "size", 0);

            string other_root_info = read_test_baseline(string(TEST_SRC_DIR) + "/catalog_test_baselines/other_root_info.txt");
            DBG(cerr << "baseline: " << other_root_info << endl);
            DBG(cerr << "response: " << str << endl);

            CPPUNIT_ASSERT(str == other_root_info);
        }
        catch (BESError &e) {
            DBG(cerr << e.get_message() << endl);
            CPPUNIT_FAIL("Failed to show catalogs");
        }

        DBG(cerr << "*****************************************" << endl);
        DBG(cerr << "BESCatalogResponseHandler specific node" << endl);
        try {
            BESDataHandlerInterface dhi;
            dhi.data[CONTAINER] = "other/file1";
            dhi.data[CATALOG_OR_INFO] = CATALOG_RESPONSE;
            BESCatalogResponseHandler handler("catalog");
            handler.execute(dhi);
            BESInfo *info = dynamic_cast<BESInfo *>(handler.get_response_object());
            CPPUNIT_ASSERT(info);
            ostringstream strm;
            info->print(strm);
            string strm_s = strm.str();
            string str = remove(strm_s, "count", 0);
            str = remove(str, "lastModified", 0);
            str = remove(str, "size", 0);

            string spec_node = read_test_baseline(string(TEST_SRC_DIR) + "/catalog_test_baselines/spec_node.txt");
            DBG(cerr << "baseline: " << spec_node << endl);
            DBG(cerr << "response: " << str << endl);

            CPPUNIT_ASSERT(str == spec_node);
        }
        catch (BESError &e) {
            DBG(cerr << e.get_message() << endl);
            CPPUNIT_FAIL("Failed to show catalogs");
        }

        DBG(cerr << "*****************************************" << endl);
        DBG(cerr << "BESCatalogResponseHandler specific node info" << endl);
        try {
            BESDataHandlerInterface dhi;
            dhi.data[CONTAINER] = "other/file1";
            dhi.data[CATALOG_OR_INFO] = SHOW_INFO_RESPONSE;
            BESCatalogResponseHandler handler("catalog");
            handler.execute(dhi);
            BESInfo *info = dynamic_cast<BESInfo *>(handler.get_response_object());
            CPPUNIT_ASSERT(info);
            ostringstream strm;
            info->print(strm);
            string strm_s = strm.str();
            string str = remove(strm_s, "count", 0);
            str = remove(str, "lastModified", 0);
            str = remove(str, "size", 0);

            string spec_info = read_test_baseline(string(TEST_SRC_DIR) + "/catalog_test_baselines/spec_info.txt");
            DBG(cerr << "baseline: " << spec_info << endl);
            DBG(cerr << "response: " << str << endl);

            CPPUNIT_ASSERT(str == spec_info);
        }
        catch (BESError &e) {
            DBG(cerr << e.get_message() << endl);
            CPPUNIT_FAIL("Failed to show catalogs");
        }

        DBG(cerr << "*****************************************" << endl);
        DBG(cerr << "BESCatalogResponseHandler specific default" << endl);
        try {
            BESDataHandlerInterface dhi;
            dhi.data[CONTAINER] = "file2";
            dhi.data[CATALOG_OR_INFO] = CATALOG_RESPONSE;
            BESCatalogResponseHandler handler("catalog");
            handler.execute(dhi);
            BESInfo *info = dynamic_cast<BESInfo *>(handler.get_response_object());
            CPPUNIT_ASSERT(info);
            ostringstream strm;
            info->print(strm);
            string strm_s = strm.str();
            string str = remove(strm_s, "lastModified", 0);
            str = remove(str, "size", 0);

            string default_node = read_test_baseline(string(TEST_SRC_DIR) + "/catalog_test_baselines/default_node.txt");
            DBG(cerr << "baseline: " << default_node << endl);
            DBG(cerr << "response: " << str << endl);

            CPPUNIT_ASSERT(str == default_node);
        }
        catch (BESError &e) {
            DBG(cerr << e.get_message() << endl);
            CPPUNIT_FAIL("Failed to show catalogs");
        }

        DBG(cerr << "*****************************************" << endl);
        DBG(cerr << "BESCatalogResponseHandler not there" << endl);
        try {
            BESDataHandlerInterface dhi;
            dhi.data[CONTAINER] = "file/doesnt/exist";
            dhi.data[CATALOG_OR_INFO] = SHOW_INFO_RESPONSE;
            BESCatalogResponseHandler handler("catalog");
            handler.execute(dhi);
            CPPUNIT_FAIL("file not found error");
        }
        catch (BESError &e) {
            DBG(cerr << e.get_message() << endl);
        }

        DBG(cerr << "*****************************************" << endl);
        DBG(cerr << "BESCatalogResponseHandler not there" << endl);
        try {
            BESDataHandlerInterface dhi;
            dhi.data[CONTAINER] = "other/notexist";
            dhi.data[CATALOG_OR_INFO] = SHOW_INFO_RESPONSE;
            BESCatalogResponseHandler handler("catalog");
            handler.execute(dhi);
            CPPUNIT_FAIL("file not found error");
        }
        catch (BESError &e) {
            DBG(cerr << e.get_message() << endl);
        }

        DBG(cerr << "*****************************************" << endl);
        DBG(cerr << "Returning from catT::run" << endl);
    }
#endif
};

CPPUNIT_TEST_SUITE_REGISTRATION(complete_catalog_lister);

int main(int argc, char *argv[]) {
    int option_char;
    while ((option_char = getopt(argc, argv, "dh")) != EOF)
        switch (option_char) {
        case 'd':
            debug = 1; // debug is a static global
            break;
        case 'h': { // help - show test names
            cerr << "Usage: catT has the following tests:" << endl;
            const std::vector<Test *> &tests = complete_catalog_lister::suite()->getTests();
            unsigned int prefix_len = complete_catalog_lister::suite()->getName().append("::").size();
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
            test = complete_catalog_lister::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);

            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
