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
#include <memory>

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
#include "CatalogNode.h"
#include "CatalogItem.h"

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

using namespace bes;
using namespace CppUnit;
using namespace std;

const string root_dir = "/catalog_test";

class catT: public TestFixture {
private:
    /**
     * Like remove_attr, remove the value of an attribute. This function
     * looks for quoted values and removes those (while remove_attr()
     * expects values to be separated by a colon).
     *
     * @param str Operate on this string
     * @param attr Look for this attribute
     * @param pos Start working at this position
     * @return The modified string
     */
    string remove(string &str, const string &attr, string::size_type pos)
    {
        string::size_type apos = str.find(attr, pos);
        if (apos == string::npos) return str;

        apos = str.find("\"", apos + 1);
        if (apos == string::npos) return str;

        string::size_type qpos = str.find("\"", apos + 1);
        if (qpos == string::npos) return str;

        // str = str.substr(0, apos + 1) + str.substr(qpos);
        // instead of using substr(), use erase
        str.erase(apos + 1, qpos - apos -1);
        // for the recursive call, start looking past the chars just removed.
        // There's no sense checking from the start of the string.
        return remove(str, attr, apos + 1);
    }

    /**
     * Return the \arg str with all the pointer items removed.
     * Used to remove the pointer information printed by the dump()
     * methods (which are usually different and thus break the
     * baselines).
     *
     * @param str Hack this string
     * @param pos Look starting at this position
     * @return The modified string.
     */
    string remove_ptr(string &str, string::size_type pos = 0)
    {
        string::size_type start = str.find("0x", pos);
        if (start == string::npos) return str;

        string::size_type end = str.find_first_not_of("0123456789abcdef", start + 2);
        if (end == string::npos) return str;

        // Cut out the '0x0000...0000' stuff
        str.erase(start, end - start);
        return remove_ptr(str, start);
    }


    /**
     * Return the \arg str with all attributes' \arg attr values removed. An
     * attribute is a _string_ : _value_. These are often things like dates
     * and sizes that can vary between OSs, machines, et cetera.
     *
     * @param str Hack this string
     * @param attr Look for this attribute
     * @param pos start looking at this position.
     * @return The modified string
     */
    string remove_attr(string &str, const string &attr, string::size_type pos = 0)
    {
        string::size_type apos = str.find(attr, pos);
        if (apos == string::npos) return str;

        string::size_type colon = str.find(":", apos);
        if (colon == string::npos) return str;

        string::size_type end = str.find("\n", colon);
        if (end == string::npos) return str;

        str.erase(colon + 1, end - colon - 1);
        return remove_attr(str, attr, colon + 1);
    }

    /**
     * Remove the _size_, _modification date_ and _time_ 'attributes' from a string
     *
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

    CPPUNIT_TEST(get_node_test);
    CPPUNIT_TEST(get_node_test_2);

    CPPUNIT_TEST(get_site_map_test);

    CPPUNIT_TEST_SUITE_END();

    void default_test()
    {
        DBG(cerr << __func__ << endl);

        TheBESKeys::TheKeys()->set_key("BES.Catalog.Default=default");
        string defcat = BESCatalogList::TheCatalogList()->default_catalog_name();
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
       TheBESKeys::TheKeys()->set_key(string("BES.Catalog.default.RootDirectory=") + TEST_SRC_DIR + root_dir);
       try {
            BESCatalogList::TheCatalogList()->add_catalog(new BESCatalogDirectory("catalog"));
            CPPUNIT_FAIL("Succeeded in adding catalog, should not have");
        }
        catch (BESError &e) {
            DBG(cerr << e.get_message() << endl);
            CPPUNIT_ASSERT("Correctly caught exception");
        }

        TheBESKeys::TheKeys()->set_key("BES.Catalog.default.TypeMatch=conf:.*\\.conf$;");
        TheBESKeys::TheKeys()->set_key("BES.Catalog.default.Include=.*file.*$;");
        TheBESKeys::TheKeys()->set_key("BES.Catalog.default.Exclude=README;");

        try {
            if (!BESCatalogList::TheCatalogList()->ref_catalog("default"))
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

        TheBESKeys::TheKeys()->set_key(string("BES.Catalog.other.RootDirectory=") + TEST_SRC_DIR + root_dir);
        TheBESKeys::TheKeys()->set_key("BES.Catalog.other.TypeMatch=conf:conf&;");
        TheBESKeys::TheKeys()->set_key("BES.Catalog.other.Include=.*file.*$;");
        TheBESKeys::TheKeys()->set_key("BES.Catalog.other.Exclude=README;");

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

    void get_node_test()
    {
        TheBESKeys::TheKeys()->set_key(string("BES.Catalog.default.RootDirectory=") + TEST_SRC_DIR + root_dir);
        TheBESKeys::TheKeys()->set_key("BES.Catalog.default.TypeMatch=conf:.*\\.conf$;");
        TheBESKeys::TheKeys()->set_key("BES.Catalog.default.Include=.*file.*$;");
        TheBESKeys::TheKeys()->set_key("BES.Catalog.default.Exclude=README;");

        auto_ptr<BESCatalog> catalog(0);
        try {
            catalog.reset(new BESCatalogDirectory("default"));
            CPPUNIT_ASSERT(catalog.get());
        }
        catch (BESError &e) {
            DBG(cerr << e.get_message() << endl);
            CPPUNIT_FAIL("Failed to add catalog");
        }

        try {
            auto_ptr<CatalogNode> node(catalog->get_node("/"));

            ostringstream oss;
            node->dump(oss);

            if (node->get_item_count() > 0) {
                int n = 0;
                for (CatalogNode::item_citer i = node->items_begin(), e = node->items_end(); i != e; ++i) {
                    oss << "Item " << n++ << ": " << endl;
                    (*i)->dump(oss);
                }
            }

            string str = oss.str();

            str = remove_ptr(str);
            str = remove_attr(str, "last modified time");

            string baseline = read_test_baseline(string(TEST_SRC_DIR) + "/catalog_test_baselines/get_node_1.txt");

            DBG(cerr << "Baseline: " << baseline << endl);
            DBG(cerr << "response: " << str << endl);

            CPPUNIT_ASSERT(str == baseline);
        }
        catch (BESError &e) {
            DBG(cerr << e.get_message() << endl);
            CPPUNIT_FAIL("Failed to get node listing for '/'");
        }
    }

    void get_node_test_2()
    {
        TheBESKeys::TheKeys()->set_key(string("BES.Catalog.default.RootDirectory=") + TEST_SRC_DIR + root_dir);
        TheBESKeys::TheKeys()->set_key("BES.Catalog.default.TypeMatch=conf:.*\\.conf$;");
        TheBESKeys::TheKeys()->set_key("BES.Catalog.default.Include=.*file.*$;");
        TheBESKeys::TheKeys()->set_key("BES.Catalog.default.Exclude=README;");

        auto_ptr<BESCatalog> catalog(new BESCatalogDirectory("default"));
        CPPUNIT_ASSERT(catalog.get());

        try {
            auto_ptr<CatalogNode> node(catalog->get_node("/child_dir"));

            ostringstream oss;
            node->dump(oss);

            if (node->get_item_count() > 0) {
                int n = 0;
                for (CatalogNode::item_citer i = node->items_begin(), e = node->items_end(); i != e; ++i) {
                    oss << "Item " << n++ << ": " << endl;
                    (*i)->dump(oss);
                }
            }

            string str = oss.str();

            str = remove_ptr(str);
            str = remove_attr(str, "last modified time");

            string baseline = read_test_baseline(string(TEST_SRC_DIR) + "/catalog_test_baselines/get_node_2.txt");

            DBG(cerr << "Baseline: " << baseline << endl);
            DBG(cerr << "response: " << str << endl);

            CPPUNIT_ASSERT(str == baseline);
        }
        catch (BESError &e) {
            DBG(cerr << e.get_message() << endl);
            CPPUNIT_FAIL("Failed to get node listing for '/child_dir'");
        }
    }

    void get_site_map_test()
    {
        TheBESKeys::TheKeys()->set_key(string("BES.Catalog.default.RootDirectory=") + TEST_SRC_DIR + root_dir);
        TheBESKeys::TheKeys()->set_key("BES.Catalog.default.TypeMatch=conf:.*file.*$;");
        TheBESKeys::TheKeys()->set_key("BES.Catalog.default.Include=.*file.*$;");
        TheBESKeys::TheKeys()->set_key("BES.Catalog.default.Exclude=README;");

        auto_ptr<BESCatalog> catalog(new BESCatalogDirectory("default"));
        CPPUNIT_ASSERT(catalog.get());

        try {
            ostringstream oss;

            catalog->get_site_map("https://machine/opendap", oss, "/");

            string baseline = read_test_baseline(string(TEST_SRC_DIR) + "/catalog_test_baselines/get_site_map.txt");

            DBG(cerr << "Baseline: " << baseline << endl);
            DBG(cerr << "response: " << oss.str() << endl);

            CPPUNIT_ASSERT(oss.str() == baseline);
        }
        catch (BESError &e) {
            DBG(cerr << e.get_message() << endl);
            CPPUNIT_FAIL("Failed to get site map");
        }
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

