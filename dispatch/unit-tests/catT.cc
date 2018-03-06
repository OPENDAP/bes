// catT.C

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

#include "config.h"

#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <sstream>

#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <GetOpt.h>

#include "BESCatalogList.h"
#include "BESCatalog.h"
#include "BESCatalogEntry.h"
#include "BESCatalogDirectory.h"
#include "BESError.h"
#include "TheBESKeys.h"
#include "BESXMLInfo.h"
#include "BESNames.h"
#include "BESDataNames.h"
#include "BESDataHandlerInterface.h"
#include "BESCatalogResponseHandler.h"
#include "BESDefaultModule.h"

#include "test_config.h"
#include "test_utils.h"

static bool debug = false;
#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

using namespace CppUnit;

using std::cerr;
using std::cout;
using std::endl;
using std::ostringstream;

const string root_dir = "/catalog_test";

class catT: public TestFixture {
private:
    string remove(string &str, string attr, string::size_type s)
    {
        string::size_type apos = str.find(attr, s);
        if (apos == string::npos) return str;
        apos = str.find("\"", apos + 1);
        if (apos == string::npos) return str;
        string::size_type qpos = str.find("\"", apos + 1);
        if (qpos == string::npos) return str;
        str = str.substr(0, apos + 1) + str.substr(qpos);
        return remove(str, attr, qpos + 1);
    }

    // I think this removes all text enclosed in parentheses. jhrg 2.25.18
    string remove_ptr(string &str, string::size_type pos = 0)
    {
        string ret;
        string::size_type lparen = str.find("(", pos);
        if (lparen != string::npos) {
            string::size_type rparen = str.find(")", lparen);
            if (rparen != string::npos) {
                ret = str.substr(0, lparen + 1) + str.substr(rparen);
                ret = remove_ptr(ret, rparen + 1);
            }
            else {
                ret = str;
            }
        }
        else {
            ret = str;
        }
        return ret;
    }

    string remove_attr(string &str, string &attr, string::size_type pos = 0)
    {
        string ret;
        string::size_type apos = str.find(attr, pos);
        if (apos != string::npos) {
            string::size_type colon = str.find(":", apos);
            if (colon != string::npos) {
                string::size_type end = str.find("\n", colon);
                if (end != string::npos) {
                    ret = str.substr(0, colon + 1) + str.substr(end);
                    ret = remove_attr(ret, attr, end);
                }
                else {
                    ret = str;
                }
            }
            else {
                ret = str;
            }
        }
        else {
            ret = str;
        }
        return ret;
    }

    /**
     * Remove the size, mod date and mod time 'attributes' from a string
     * @param str
     * @return The pruned string
     */
    string remove_stuff(string &str)
    {
        string ret;
        string attr = "size";
        ret = remove_attr(str, attr);
        attr = "modification date";
        ret = remove_attr(ret, attr);
        attr = "modification time";
        ret = remove_attr(ret, attr);
        return ret;
    }

public:
    catT()
    {
    }
    ~catT()
    {
    }

    void setUp()
    {
        string bes_conf = (string) TEST_SRC_DIR + "/bes.conf";
        TheBESKeys::ConfigFile = bes_conf;
        TheBESKeys::TheKeys()->set_key("BES.Data.RootDirectory=/dev/null");
        TheBESKeys::TheKeys()->set_key("BES.Info.Buffered=no");
        TheBESKeys::TheKeys()->set_key("BES.Info.Type=xml");
        try {
            BESDefaultModule::initialize(0, 0);
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            throw e;
        }
    }

    void tearDown()
    {
    }

    CPPUNIT_TEST_SUITE( catT );

    CPPUNIT_TEST(default_test);
    CPPUNIT_TEST(no_default_test);
    CPPUNIT_TEST(root_dir_test1);


    CPPUNIT_TEST_SUITE_END();


    void default_test()
    {
        DBG(cerr << __func__ << endl);

        TheBESKeys::TheKeys()->set_key("BES.Catalog.Default=default");
        string defcat = BESCatalogList::TheCatalogList()->default_catalog();
        CPPUNIT_ASSERT(defcat == "default");

        int numcats = BESCatalogList::TheCatalogList()->num_catalogs();
        CPPUNIT_ASSERT(numcats == 0);

        try {
            // show_catalogs(...) the false value for show_default will suppress
            // showing anything for the default catalog. Since there's no other
            // catalog, there's nothing to show. jhrg 2.25.18
            BESCatalogEntry *entry = 0;
            entry = BESCatalogList::TheCatalogList()->show_catalogs(0, false);
            ostringstream strm;
            entry->dump(strm);
            DBG(cerr << "Entry before remove_ptr: ");
            DBG(entry->dump(cerr));
            DBG(cerr <<endl);

            string str = strm.str();
            str = remove_ptr(str);
            string empty_response = read_test_baseline(string(TEST_SRC_DIR) + "/catalog_test_baselines/empty_response.txt");

            DBG(cerr << "baseline: " << empty_response << endl);
            DBG(cerr << "response: " << str << endl);

            CPPUNIT_ASSERT(str == empty_response);
        }
        catch (BESError &e) {
            DBG(cerr << e.get_message() << endl);
            CPPUNIT_FAIL("Failed to show catalogs");
        }
    }

    // This is really three different tests. jhrg 2.25.18
    void no_default_test() {
        DBG(cerr << __func__ << endl);
        try {
            BESDataHandlerInterface dhi;
            dhi.data[CATALOG_OR_INFO] = CATALOG_RESPONSE;
            BESCatalogResponseHandler handler("catalog");
            handler.execute(dhi);
            CPPUNIT_FAIL("Should have failed, no default catalog");
        }
        catch (BESError &e) {
            DBG(cerr << e.get_message() << endl);
            CPPUNIT_ASSERT("Correctly caught exception");
        }

        DBG(cerr << "manipulate non-existent catalog" << endl);
        BESCatalog *catobj = BESCatalogList::TheCatalogList()->find_catalog("dummy");
        CPPUNIT_ASSERT(catobj == 0);

        CPPUNIT_ASSERT(BESCatalogList::TheCatalogList()->add_catalog(0) == false);
        CPPUNIT_ASSERT(BESCatalogList::TheCatalogList()->ref_catalog("dummy") == false);
        CPPUNIT_ASSERT(BESCatalogList::TheCatalogList()->deref_catalog("dummy") == false);

        DBG(cerr << "add a catalog with no settings" << endl);
        try {
            BESCatalogList::TheCatalogList()->add_catalog(new BESCatalogDirectory("catalog"));
            CPPUNIT_FAIL("Succeeded in adding catalog, should not have");
        }
        catch (BESError &e) {
            DBG(cerr << e.get_message() << endl);
            CPPUNIT_ASSERT("Correctly caught exception");
        }
    }

    // This test should be broken up into smaller pieces. jhrg 8/23/17
    void root_dir_test1() {
        string var = (string) "BES.Catalog.default.RootDirectory=" + TEST_SRC_DIR + root_dir;
        TheBESKeys::TheKeys()->set_key(var);
        try {
            BESCatalogList::TheCatalogList()->add_catalog(new BESCatalogDirectory("catalog"));
            CPPUNIT_FAIL("Succeeded in adding catalog, should not have");
        }
        catch (BESError &e) {
            DBG(cerr << e.get_message() << endl);
            CPPUNIT_ASSERT("Correctly caught exception");
        }

        DBG(cerr << "add good catalog" << endl);
        var = (string) "BES.Catalog.default.TypeMatch=conf:conf&;";
        TheBESKeys::TheKeys()->set_key(var);
        var = (string) "BES.Catalog.default.Include=.*file.*$;";
        TheBESKeys::TheKeys()->set_key(var);
        var = (string) "BES.Catalog.default.Exclude=README;";
        TheBESKeys::TheKeys()->set_key(var);

        try {
            BESCatalogList::TheCatalogList()->add_catalog(new BESCatalogDirectory("default"));
        }
        catch (BESError &e) {
            DBG(cerr << e.get_message() << endl);
            CPPUNIT_FAIL("Failed to add catalog");
        }

        BESCatalog *catobj = BESCatalogList::TheCatalogList()->find_catalog("default");
        CPPUNIT_ASSERT(catobj);
        int numcats = BESCatalogList::TheCatalogList()->num_catalogs();
        CPPUNIT_ASSERT(numcats == 1);

        try {
            BESDataHandlerInterface dhi;
            BESCatalogEntry *entry = BESCatalogList::TheCatalogList()->show_catalogs(0);
            ostringstream strm;
            entry->dump(strm);
            string str = strm.str();
            str = remove_ptr(str);
            str = remove_stuff(str);

            string one_response = read_test_baseline(string(TEST_SRC_DIR) + "/catalog_test_baselines/one_response.txt");

            DBG(cerr << "baseline: " << one_response << endl);
            DBG(cerr << "response: " << str << endl);

            CPPUNIT_ASSERT(str == one_response);
        }
        catch (BESError &e) {
            DBG(cerr << e.get_message() << endl);
            CPPUNIT_FAIL("Failed to show catalogs");
        }

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

    void get_node_test() {
#if 0
        DBG(cerr << "add good catalog" << endl);
        var = (string) "BES.Catalog.default.TypeMatch=conf:conf&;";
        TheBESKeys::TheKeys()->set_key(var);
        var = (string) "BES.Catalog.default.Include=.*file.*$;";
        TheBESKeys::TheKeys()->set_key(var);
        var = (string) "BES.Catalog.default.Exclude=README;";
        TheBESKeys::TheKeys()->set_key(var);

        try {
            BESCatalogList::TheCatalogList()->add_catalog(new BESCatalogDirectory("default"));
        }
        catch (BESError &e) {
            DBG(cerr << e.get_message() << endl);
            CPPUNIT_FAIL("Failed to add catalog");
        }

        BESCatalog *catobj = BESCatalogList::TheCatalogList()->find_catalog("default");
        CPPUNIT_ASSERT(catobj);
        int numcats = BESCatalogList::TheCatalogList()->num_catalogs();
        CPPUNIT_ASSERT(numcats == 1);

        try {
            BESDataHandlerInterface dhi;
            BESCatalogEntry *entry = BESCatalogList::TheCatalogList()->show_catalogs(0);
            ostringstream strm;
            entry->dump(strm);
            string str = strm.str();
            str = remove_ptr(str);
            str = remove_stuff(str);

            string one_response = read_test_baseline(string(TEST_SRC_DIR) + "/catalog_test_baselines/one_response.txt");

            DBG(cerr << "baseline: " << one_response << endl);
            DBG(cerr << "response: " << str << endl);

            CPPUNIT_ASSERT(str == one_response);
        }
        catch (BESError &e) {
            DBG(cerr << e.get_message() << endl);
            CPPUNIT_FAIL("Failed to show catalogs");
        }
#endif
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(catT);

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
            cerr << "Usage: catT has the following tests:" << endl;
            const std::vector<Test*> &tests = catT::suite()->getTests();
            unsigned int prefix_len = catT::suite()->getName().append("::").length();
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
            test = catT::suite()->getName().append("::").append(argv[i++]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}

