// infoT.C

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
#include <iostream>
#include <sstream>

using std::cerr;
using std::cout;
using std::endl;
using std::map;
using std::ostringstream;
using std::string;

#include "BESDataHandlerInterface.h"
#include "BESHTMLInfo.h"
#include "BESInfoList.h"
#include "BESInfoNames.h"
#include "BESTextInfo.h"
#include "BESXMLInfo.h"
#include "TheBESKeys.h"
#include <test_config.h>
#include <unistd.h>

static bool debug = false;

#undef DBG
#define DBG(x)                                                                                                         \
    do {                                                                                                               \
        if (debug)                                                                                                     \
            (x);                                                                                                       \
    } while (false);

string txt_baseline = "tag1: tag1 data\n\
tag2\n\
    tag3: tag3 data\n\
        attr_name: \"attr_val\"\n";

string html_baseline = "<HTML>\n\
    <HEAD>\n\
        <TITLE>testHTMLResponse</TITLE>\n\
    </HEAD>\n\
    <BODY>\n\
        tag1: tag1 data<BR />\n\
        tag2<BR />\n\
            tag3: tag3 data<BR />\n\
                attr_name: \"attr_val\"<BR />\n\
    </BODY>\n\
</HTML>\n";

string xml_baseline = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\
<response xmlns=\"http://xml.opendap.org/ns/bes/1.0#\">\n\
    <testXMLResponse>\n\
        <tag1>tag1 data</tag1>\n\
        <tag2>\n\
            <tag3 attr_name=\"&quot;attr_val&quot;\">tag3 data</tag3>\n\
        </tag2>\n\
    </testXMLResponse>\n\
</response>\n";

class infoT : public TestFixture {
private:
public:
    infoT() {}
    ~infoT() {}

    void setUp() {
        string bes_conf = (string)TEST_SRC_DIR + "/info_test.ini";
        TheBESKeys::ConfigFile = bes_conf;
    }

    void tearDown() {}

    CPPUNIT_TEST_SUITE(infoT);

    CPPUNIT_TEST(do_test);

    CPPUNIT_TEST_SUITE_END();

    void do_test() {
        cout << "*****************************************" << endl;
        cout << "Entered infoT::run" << endl;

        cout << "*****************************************" << endl;
        cout << "add info builders to info list" << endl;
        CPPUNIT_ASSERT(BESInfoList::TheList()->add_info_builder(BES_TEXT_INFO, BESTextInfo::BuildTextInfo));
        CPPUNIT_ASSERT(BESInfoList::TheList()->add_info_builder(BES_HTML_INFO, BESHTMLInfo::BuildHTMLInfo));
        CPPUNIT_ASSERT(BESInfoList::TheList()->add_info_builder(BES_XML_INFO, BESXMLInfo::BuildXMLInfo));

        map<string, string, std::less<>> attrs;
        attrs["attr_name"] = "\"attr_val\"";

        cout << "*****************************************" << endl;
        cout << "Set Info type to txt" << endl;
        TheBESKeys::TheKeys()->set_key("BES.Info.Type", "txt");
        BESInfo *info = BESInfoList::TheList()->build_info();
        auto t_info = dynamic_cast<BESTextInfo *>(info);
        CPPUNIT_ASSERT(t_info);

        BESDataHandlerInterface dhi;
        t_info->begin_response("testTextResponse", dhi);
        t_info->add_tag("tag1", "tag1 data");
        t_info->begin_tag("tag2");
        t_info->add_tag("tag3", "tag3 data", &attrs);
        t_info->end_tag("tag2");
        t_info->end_response();
        ostringstream tstrm;
        t_info->print(tstrm);
        CPPUNIT_ASSERT(tstrm.str() == txt_baseline);

        cout << "*****************************************" << endl;
        cout << "Set Info type to html" << endl;
        TheBESKeys::TheKeys()->set_key("BES.Info.Type", "html");
        info = BESInfoList::TheList()->build_info();
        auto h_info = dynamic_cast<BESHTMLInfo *>(info);
        CPPUNIT_ASSERT(h_info);

        h_info->begin_response("testHTMLResponse", dhi);
        h_info->add_tag("tag1", "tag1 data");
        h_info->begin_tag("tag2");
        h_info->add_tag("tag3", "tag3 data", &attrs);
        h_info->end_tag("tag2");
        h_info->end_response();
        ostringstream hstrm;
        h_info->print(hstrm);
        CPPUNIT_ASSERT(hstrm.str() == html_baseline);

        cout << "*****************************************" << endl;
        cout << "Set Info type to xml" << endl;
        TheBESKeys::TheKeys()->set_key("BES.Info.Type", "xml");
        info = BESInfoList::TheList()->build_info();
        BESXMLInfo *x_info = dynamic_cast<BESXMLInfo *>(info);
        CPPUNIT_ASSERT(x_info);

        x_info->begin_response("testXMLResponse", dhi);
        x_info->add_tag("tag1", "tag1 data");
        x_info->begin_tag("tag2");
        x_info->add_tag("tag3", "tag3 data", &attrs);
        x_info->end_tag("tag2");
        x_info->end_response();
        ostringstream xstrm;
        x_info->print(xstrm);
        CPPUNIT_ASSERT(xstrm.str() == xml_baseline);

        cout << "*****************************************" << endl;
        cout << "Returning from infoT::run" << endl;
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(infoT);

int main(int argc, char *argv[]) {
    int option_char;
    while ((option_char = getopt(argc, argv, "dh")) != EOF)
        switch (option_char) {
        case 'd':
            debug = 1; // debug is a static global
            break;
        case 'h': { // help - show test names
            cerr << "Usage: infoT has the following tests:" << endl;
            const std::vector<Test *> &tests = infoT::suite()->getTests();
            unsigned int prefix_len = infoT::suite()->getName().append("::").size();
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
            test = infoT::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
