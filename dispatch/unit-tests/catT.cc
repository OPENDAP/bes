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

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

using namespace CppUnit;

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <sstream>

using std::cerr;
using std::cout;
using std::endl;
using std::ostringstream;


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
#include <test_config.h>
#include <GetOpt.h>

static bool debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

string empty_response =
    "BESCatalogEntry::dump - ()\n\
    name: /\n\
    catalog: \n\
    size: \n\
    modification date: \n\
    modification time: \n\
    services: none\n\
    metadata: none\n\
    is collection? no\n\
    count: 0\n\
";

string one_response =
    "BESCatalogEntry::dump - ()\n\
    name: /\n\
    catalog: \n\
    size:\n\
    modification date:\n\
    modification time:\n\
    services: none\n\
    metadata: none\n\
    is collection? yes\n\
    count: 1\n\
        BESCatalogEntry::dump - ()\n\
            name: /\n\
            catalog: default\n\
            size:\n\
            modification date:\n\
            modification time:\n\
            services: none\n\
            metadata: none\n\
            is collection? yes\n\
            count: 1\n\
                BESCatalogEntry::dump - ()\n\
                    name: bes.conf\n\
                    catalog: default\n\
                    size:\n\
                    modification date:\n\
                    modification time:\n\
                    services: none\n\
                    metadata: none\n\
                    is collection? no\n\
                    count: 0\n\
";

string cat_root =
    "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\
<response xmlns=\"http://xml.opendap.org/ns/bes/1.0#\">\n\
    <showCatalog>\n\
        <dataset catalog=\"default\" count=\"1\" lastModified=\"\" name=\"/\" node=\"true\" size=\"\">\n\
            <dataset catalog=\"default\" lastModified=\"\" name=\"bes.conf\" node=\"false\" size=\"\"/>\n\
        </dataset>\n\
    </showCatalog>\n\
</response>\n";

string two_response =
    "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\
<response xmlns=\"http://xml.opendap.org/ns/bes/1.0#\">\n\
    <showCatalog>\n\
        <dataset catalog=\"default\" count=\"2\" lastModified=\"\" name=\"/\" node=\"true\" size=\"\">\n\
            <dataset catalog=\"other\" count=\"12\" lastModified=\"\" name=\"other/\" node=\"true\" size=\"\"/>\n\
            <dataset catalog=\"default\" lastModified=\"\" name=\"bes.conf\" node=\"false\" size=\"\"/>\n\
        </dataset>\n\
    </showCatalog>\n\
</response>\n";

string other_root =
    "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\
<response xmlns=\"http://xml.opendap.org/ns/bes/1.0#\">\n\
    <showCatalog>\n\
        <dataset catalog=\"other\" count=\"12\" lastModified=\"\" name=\"other/\" node=\"true\" size=\"\">\n\
            <dataset catalog=\"other\" lastModified=\"\" name=\"other/bad_keys1.ini\" node=\"false\" size=\"\"/>\n\
            <dataset catalog=\"other\" lastModified=\"\" name=\"other/defT.ini\" node=\"false\" size=\"\"/>\n\
            <dataset catalog=\"other\" lastModified=\"\" name=\"other/empty.ini\" node=\"false\" size=\"\"/>\n\
            <dataset catalog=\"other\" lastModified=\"\" name=\"other/info_test.ini\" node=\"false\" size=\"\"/>\n\
            <dataset catalog=\"other\" lastModified=\"\" name=\"other/keys_test.ini\" node=\"false\" size=\"\"/>\n\
            <dataset catalog=\"other\" lastModified=\"\" name=\"other/keys_test_include.ini\" node=\"false\" size=\"\"/>\n\
            <dataset catalog=\"other\" lastModified=\"\" name=\"other/keys_test_m1.ini\" node=\"false\" size=\"\"/>\n\
            <dataset catalog=\"other\" lastModified=\"\" name=\"other/keys_test_m2.ini\" node=\"false\" size=\"\"/>\n\
            <dataset catalog=\"other\" lastModified=\"\" name=\"other/keys_test_m3.ini\" node=\"false\" size=\"\"/>\n\
            <dataset catalog=\"other\" lastModified=\"\" name=\"other/persistence_cgi_test.ini\" node=\"false\" size=\"\"/>\n\
            <dataset catalog=\"other\" lastModified=\"\" name=\"other/persistence_file_test.ini\" node=\"false\" size=\"\"/>\n\
            <dataset catalog=\"other\" lastModified=\"\" name=\"other/persistence_mysql_test.ini\" node=\"false\" size=\"\"/>\n\
        </dataset>\n\
    </showCatalog>\n\
</response>\n";

string other_root_info =
    "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\
<response xmlns=\"http://xml.opendap.org/ns/bes/1.0#\">\n\
    <showInfo>\n\
        <dataset catalog=\"other\" count=\"12\" lastModified=\"\" name=\"other/\" node=\"true\" size=\"\"/>\n\
    </showInfo>\n\
</response>\n";

string spec_node =
    "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\
<response xmlns=\"http://xml.opendap.org/ns/bes/1.0#\">\n\
    <showCatalog>\n\
        <dataset catalog=\"other\" lastModified=\"\" name=\"other/keys_test_m3.ini\" node=\"false\" size=\"\"/>\n\
    </showCatalog>\n\
</response>\n";

string spec_info =
    "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\
<response xmlns=\"http://xml.opendap.org/ns/bes/1.0#\">\n\
    <showInfo>\n\
        <dataset catalog=\"other\" lastModified=\"\" name=\"other/keys_test_m3.ini\" node=\"false\" size=\"\"/>\n\
    </showInfo>\n\
</response>\n";

string default_node =
    "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\
<response xmlns=\"http://xml.opendap.org/ns/bes/1.0#\">\n\
    <showCatalog>\n\
        <dataset catalog=\"default\" lastModified=\"\" name=\"bes.conf\" node=\"false\" size=\"\"/>\n\
    </showCatalog>\n\
</response>\n";

class catT: public TestFixture {
private:

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

    CPPUNIT_TEST( do_test );

    CPPUNIT_TEST_SUITE_END()
    ;

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

    void do_test()
    {
        cerr << "*****************************************" << endl;
        cerr << "Entered catT::run" << endl;

        cerr << "*****************************************" << endl;
        cerr << "set the default catalog and test" << endl;
        TheBESKeys::TheKeys()->set_key("BES.Catalog.Default=default");
        string defcat = BESCatalogList::TheCatalogList()->default_catalog();
        CPPUNIT_ASSERT( defcat == "default" );

        cerr << "*****************************************" << endl;
        cerr << "num catalogs should be zero" << endl;
        int numcats = BESCatalogList::TheCatalogList()->num_catalogs();
        CPPUNIT_ASSERT( numcats == 0 );

        cerr << "*****************************************" << endl;
        cerr << "show empty catalog list" << endl;
        try {
            BESDataHandlerInterface dhi;
            BESCatalogEntry *entry = 0;
            entry = BESCatalogList::TheCatalogList()->show_catalogs(dhi, 0);
            ostringstream strm;
            entry->dump(strm);
            string str = strm.str();
            str = remove_ptr(str);
            CPPUNIT_ASSERT( str == empty_response );
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT( !"Failed to show catalogs" );
        }

        cerr << "*****************************************" << endl;
        cerr << "no default catalog, use catalog response" << endl;
        try {
            BESDataHandlerInterface dhi;
            dhi.data[CATALOG_OR_INFO] = CATALOG_RESPONSE;
            BESCatalogResponseHandler handler("catalog");
            handler.execute(dhi);
            CPPUNIT_ASSERT( !"Should have failed, no default catalog" );
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
        }

        cerr << "*****************************************" << endl;
        cerr << "manipulate non-existant catalog" << endl;
        BESCatalog *catobj = BESCatalogList::TheCatalogList()->find_catalog("dummy");
        CPPUNIT_ASSERT( catobj == 0 );

        CPPUNIT_ASSERT( BESCatalogList::TheCatalogList()->add_catalog( 0 ) == false );
        CPPUNIT_ASSERT( BESCatalogList::TheCatalogList()->ref_catalog( "dummy" ) == false );
        CPPUNIT_ASSERT( BESCatalogList::TheCatalogList()->deref_catalog( "dummy" ) == false );

        cerr << "*****************************************" << endl;
        cerr << "add a catalog with no settings" << endl;
        try {
            BESCatalogList::TheCatalogList()->add_catalog(new BESCatalogDirectory("catalog"));
            CPPUNIT_ASSERT( !"Succeeded in adding catalog, should not have" );
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
        }

        string var = (string) "BES.Catalog.default.RootDirectory=" + TEST_SRC_DIR + "/";
        TheBESKeys::TheKeys()->set_key(var);
        try {
            BESCatalogList::TheCatalogList()->add_catalog(new BESCatalogDirectory("catalog"));
            CPPUNIT_ASSERT( !"Succeeded in adding catalog, should not have" );
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
        }

        cerr << "*****************************************" << endl;
        cerr << "add good catalog" << endl;
        var = (string) "BES.Catalog.default.TypeMatch=conf:conf&;";
        TheBESKeys::TheKeys()->set_key(var);
        var = (string) "BES.Catalog.default.Include=.*\\.conf$;";
        TheBESKeys::TheKeys()->set_key(var);
        var = (string) "BES.Catalog.default.Exclude=\\..*;cache.*;testdir;";
        TheBESKeys::TheKeys()->set_key(var);
        try {
            BESCatalogList::TheCatalogList()->add_catalog(new BESCatalogDirectory("default"));
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT( !"Failed to add catalog" );
        }
        catobj = BESCatalogList::TheCatalogList()->find_catalog("default");
        CPPUNIT_ASSERT( catobj );
        numcats = BESCatalogList::TheCatalogList()->num_catalogs();
        CPPUNIT_ASSERT( numcats == 1 );

        cerr << "*****************************************" << endl;
        cerr << "show catalog list" << endl;
        try {
            BESDataHandlerInterface dhi;
            BESCatalogEntry *entry = 0;
            entry = BESCatalogList::TheCatalogList()->show_catalogs(dhi, 0);
            ostringstream strm;
            entry->dump(strm);
            string str = strm.str();
            str = remove_ptr(str);
            str = remove_stuff(str);
            //cerr << "Catalog list str: " << str << endl;
            CPPUNIT_ASSERT( str == one_response );
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT( !"Failed to show catalogs" );
        }

        cerr << "*****************************************" << endl;
        cerr << "now try it with BESCatalogResponseHandler" << endl;
        try {
            BESDataHandlerInterface dhi;
            dhi.data[CATALOG_OR_INFO] = CATALOG_RESPONSE;
            BESCatalogResponseHandler handler("catalog");
            handler.execute(dhi);
            BESInfo *info = dynamic_cast<BESInfo *>(handler.get_response_object());
            CPPUNIT_ASSERT( info );
            ostringstream strm;
            info->print(strm);
            string strm_s = strm.str();
            string str = remove(strm_s, "lastModified", 0);
            str = remove(str, "size", 0);
            CPPUNIT_ASSERT( str == cat_root );
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT( !"Failed to show catalogs" );
        }

        cerr << "*****************************************" << endl;
        cerr << "add good catalog" << endl;
        var = (string) "BES.Catalog.other.RootDirectory=" + TEST_SRC_DIR + "/";
        TheBESKeys::TheKeys()->set_key(var);
        var = (string) "BES.Catalog.other.TypeMatch=info:info&;";
        TheBESKeys::TheKeys()->set_key(var);
        var = (string) "BES.Catalog.other.Include=.*\\.ini$;";
        TheBESKeys::TheKeys()->set_key(var);
        var = (string) "BES.Catalog.other.Exclude=\\..*;cache.*;testdir;";
        TheBESKeys::TheKeys()->set_key(var);
        BESCatalog *other = 0;
        try {
            other = new BESCatalogDirectory("other");
            BESCatalogList::TheCatalogList()->add_catalog(other);
            CPPUNIT_ASSERT( "Succeeded in adding other catalog" );
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT( !"Failed to add catalog" );
        }
        catobj = BESCatalogList::TheCatalogList()->find_catalog("other");
        CPPUNIT_ASSERT( catobj );
        CPPUNIT_ASSERT( catobj == other );
        numcats = BESCatalogList::TheCatalogList()->num_catalogs();
        CPPUNIT_ASSERT( numcats == 2 );

        cerr << "*****************************************" << endl;
        cerr << "now try it with BESCatalogResponseHandler" << endl;
        try {
            BESDataHandlerInterface dhi;
            dhi.data[CATALOG_OR_INFO] = CATALOG_RESPONSE;
            BESCatalogResponseHandler handler("catalog");
            handler.execute(dhi);
            BESInfo *info = dynamic_cast<BESInfo *>(handler.get_response_object());
            CPPUNIT_ASSERT( info );
            ostringstream strm;
            info->print(strm);
            string strm_s = strm.str();
            string str = remove(strm_s, "lastModified", 0);
            str = remove(str, "size", 0);
            // cerr << "Catalog list str: " << str << endl;
            CPPUNIT_ASSERT( str == two_response );
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT( !"Failed to show catalogs" );
        }

        cerr << "*****************************************" << endl;
        cerr << "BESCatalogResponseHandler with catalog response" << endl;
        try {
            BESDataHandlerInterface dhi;
            dhi.data[CONTAINER] = "other";
            dhi.data[CATALOG_OR_INFO] = CATALOG_RESPONSE;
            BESCatalogResponseHandler handler("catalog");
            handler.execute(dhi);
            BESInfo *info = dynamic_cast<BESInfo *>(handler.get_response_object());
            CPPUNIT_ASSERT( info );
            ostringstream strm;
            info->print(strm);
            string strm_s = strm.str();
            string str = remove(strm_s, "lastModified", 0);
            str = remove(str, "size", 0);
            // cerr << str << endl ;
            // cerr << other_root << endl ;
            CPPUNIT_ASSERT( str == other_root );
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT( !"Failed to show catalogs" );
        }

        cerr << "*****************************************" << endl;
        cerr << "BESCatalogResponseHandler with info response" << endl;
        try {
            BESDataHandlerInterface dhi;
            dhi.data[CONTAINER] = "other";
            dhi.data[CATALOG_OR_INFO] = SHOW_INFO_RESPONSE;
            BESCatalogResponseHandler handler("catalog");
            handler.execute(dhi);
            BESInfo *info = dynamic_cast<BESInfo *>(handler.get_response_object());
            CPPUNIT_ASSERT( info );
            ostringstream strm;
            info->print(strm);
            string strm_s = strm.str();
            string str = remove(strm_s, "lastModified", 0);
            str = remove(str, "size", 0);
            CPPUNIT_ASSERT( str == other_root_info );
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT( !"Failed to show catalogs" );
        }

        cerr << "*****************************************" << endl;
        cerr << "BESCatalogResponseHandler specific node" << endl;
        try {
            BESDataHandlerInterface dhi;
            dhi.data[CONTAINER] = "other/keys_test_m3.ini";
            dhi.data[CATALOG_OR_INFO] = CATALOG_RESPONSE;
            BESCatalogResponseHandler handler("catalog");
            handler.execute(dhi);
            BESInfo *info = dynamic_cast<BESInfo *>(handler.get_response_object());
            CPPUNIT_ASSERT( info );
            ostringstream strm;
            info->print(strm);
            string strm_s = strm.str();
            string str = remove(strm_s, "count", 0);
            str = remove(str, "lastModified", 0);
            str = remove(str, "size", 0);
            CPPUNIT_ASSERT( str == spec_node );
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT( !"Failed to show catalogs" );
        }

        cerr << "*****************************************" << endl;
        cerr << "BESCatalogResponseHandler specific node info" << endl;
        try {
            BESDataHandlerInterface dhi;
            dhi.data[CONTAINER] = "other/keys_test_m3.ini";
            dhi.data[CATALOG_OR_INFO] = SHOW_INFO_RESPONSE;
            BESCatalogResponseHandler handler("catalog");
            handler.execute(dhi);
            BESInfo *info = dynamic_cast<BESInfo *>(handler.get_response_object());
            CPPUNIT_ASSERT( info );
            ostringstream strm;
            info->print(strm);
            string strm_s = strm.str();
            string str = remove(strm_s, "count", 0);
            str = remove(str, "lastModified", 0);
            str = remove(str, "size", 0);
            CPPUNIT_ASSERT( str == spec_info );
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT( !"Failed to show catalogs" );
        }

        cerr << "*****************************************" << endl;
        cerr << "BESCatalogResponseHandler specific default" << endl;
        try {
            BESDataHandlerInterface dhi;
            dhi.data[CONTAINER] = "bes.conf";
            dhi.data[CATALOG_OR_INFO] = CATALOG_RESPONSE;
            BESCatalogResponseHandler handler("catalog");
            handler.execute(dhi);
            BESInfo *info = dynamic_cast<BESInfo *>(handler.get_response_object());
            CPPUNIT_ASSERT( info );
            ostringstream strm;
            info->print(strm);
            string strm_s = strm.str();
            string str = remove(strm_s, "lastModified", 0);
            str = remove(str, "size", 0);
            // cerr << str << endl ;
            // cerr << default_node << endl ;
            CPPUNIT_ASSERT( str == default_node );
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT( !"Failed to show catalogs" );
        }

        cerr << "*****************************************" << endl;
        cerr << "BESCatalogResponseHandler not there" << endl;
        try {
            BESDataHandlerInterface dhi;
            dhi.data[CONTAINER] = "file/doesnt/exist";
            dhi.data[CATALOG_OR_INFO] = SHOW_INFO_RESPONSE;
            BESCatalogResponseHandler handler("catalog");
            handler.execute(dhi);
            CPPUNIT_ASSERT( !"file not found error" );
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
        }

        cerr << "*****************************************" << endl;
        cerr << "BESCatalogResponseHandler not there" << endl;
        try {
            BESDataHandlerInterface dhi;
            dhi.data[CONTAINER] = "other/notexist";
            dhi.data[CATALOG_OR_INFO] = SHOW_INFO_RESPONSE;
            BESCatalogResponseHandler handler("catalog");
            handler.execute(dhi);
            CPPUNIT_ASSERT( !"file not found error" );
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
        }

        cerr << "*****************************************" << endl;
        cerr << "Returning from catT::run" << endl;
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( catT );

int main(int argc, char*argv[]) {

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
            test = catT::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}

