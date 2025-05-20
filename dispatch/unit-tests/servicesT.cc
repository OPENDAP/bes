// servicesT.C

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

#include <iostream>
#include <sstream>
#include <list>

using std::cerr;
using std::cout;
using std::endl;
using std::ostringstream;
using std::string;
using std::map;
using std::list;

#include "BESServiceRegistry.h"
#include "BESError.h"
#include "BESXMLInfo.h"
#include "BESDataNames.h"
#include <unistd.h>

static bool debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

string dump1 =
    "BESServiceRegistry::dump - (X)\n\
    registered services\n\
        cedar\n\
            flat\n\
                description: CEDAR flat format data response\n\
                formats:\n\
                    cedar\n\
            stream\n\
                description: CEDAR stream .cbf data file\n\
                formats:\n\
                    cedar\n\
            tab\n\
                description: CEDAR tab separated data response\n\
                formats:\n\
                    cedar\n\
        dap\n\
            ascii\n\
                description: OPeNDAP ASCII data\n\
                formats:\n\
                    dap2\n\
            das\n\
                description: OPeNDAP Data Attributes\n\
                formats:\n\
                    dap2\n\
            dds\n\
                description: OPeNDAP Data Description\n\
                formats:\n\
                    dap2\n\
            dods\n\
                description: OPeNDAP Data Object\n\
                formats:\n\
                    dap2\n\
                    netcdf\n\
            html_form\n\
                description: OPeNDAP Form for access\n\
                formats:\n\
                    dap2\n\
            info_page\n\
                description: OPeNDAP Data Information\n\
                formats:\n\
                    dap2\n\
    services provided by handler\n\
        cedar: cedar, dap\n\
        nc: dap\n\
";

string dump2 =
    "BESServiceRegistry::dump - (X)\n\
    registered services\n\
        cedar\n\
            flat\n\
                description: CEDAR flat format data response\n\
                formats:\n\
                    cedar\n\
            stream\n\
                description: CEDAR stream .cbf data file\n\
                formats:\n\
                    cedar\n\
            tab\n\
                description: CEDAR tab separated data response\n\
                formats:\n\
                    cedar\n\
    services provided by handler\n\
        cedar: cedar\n\
        nc\n\
";

string show1 =
    "<showServices>\n\
        <serviceDescription name=\"cedar\">\n\
            <command name=\"flat\">\n\
                <description>CEDAR flat format data response</description>\n\
                <format name=\"cedar\"/>\n\
            </command>\n\
            <command name=\"stream\">\n\
                <description>CEDAR stream .cbf data file</description>\n\
                <format name=\"cedar\"/>\n\
            </command>\n\
            <command name=\"tab\">\n\
                <description>CEDAR tab separated data response</description>\n\
                <format name=\"cedar\"/>\n\
            </command>\n\
        </serviceDescription>\n\
        <serviceDescription name=\"dap\">\n\
            <command name=\"ascii\">\n\
                <description>OPeNDAP ASCII data</description>\n\
                <format name=\"dap2\"/>\n\
            </command>\n\
            <command name=\"das\">\n\
                <description>OPeNDAP Data Attributes</description>\n\
                <format name=\"dap2\"/>\n\
            </command>\n\
            <command name=\"dds\">\n\
                <description>OPeNDAP Data Description</description>\n\
                <format name=\"dap2\"/>\n\
            </command>\n\
            <command name=\"dods\">\n\
                <description>OPeNDAP Data Object</description>\n\
                <format name=\"dap2\"/>\n\
                <format name=\"netcdf\"/>\n\
            </command>\n\
            <command name=\"html_form\">\n\
                <description>OPeNDAP Form for access</description>\n\
                <format name=\"dap2\"/>\n\
            </command>\n\
            <command name=\"info_page\">\n\
                <description>OPeNDAP Data Information</description>\n\
                <format name=\"dap2\"/>\n\
            </command>\n\
        </serviceDescription>\n\
    </showServices>\n\
</response>\n\
";

string show2 =
    "<showServices>\n\
        <serviceDescription name=\"cedar\">\n\
            <command name=\"flat\">\n\
                <description>CEDAR flat format data response</description>\n\
                <format name=\"cedar\"/>\n\
            </command>\n\
            <command name=\"stream\">\n\
                <description>CEDAR stream .cbf data file</description>\n\
                <format name=\"cedar\"/>\n\
            </command>\n\
            <command name=\"tab\">\n\
                <description>CEDAR tab separated data response</description>\n\
                <format name=\"cedar\"/>\n\
            </command>\n\
        </serviceDescription>\n\
    </showServices>\n\
</response>\n\
";

class servicesT: public TestFixture {
private:

public:
    servicesT()
    {
    }
    ~servicesT()
    {
    }

    void setUp()
    {
    }

    void tearDown()
    {
    }

CPPUNIT_TEST_SUITE( servicesT );

    CPPUNIT_TEST( do_test );

    CPPUNIT_TEST_SUITE_END()
    ;

    void do_test()
    {
        cout << "*****************************************" << endl;
        cout << "Entered servicesT::run" << endl;

        cout << "*****************************************" << endl;
        cout << "create the registry" << endl;
        BESServiceRegistry *registry = BESServiceRegistry::TheRegistry();
        CPPUNIT_ASSERT( registry );

        try {
            cout << "*****************************************" << endl;
            cout << "add dap service with das, dds, dods" << endl;
            registry->add_service("dap");
            map<string, string> cmds;
            registry->add_to_service("dap", "das", "OPeNDAP Data Attributes", "dap2");
            registry->add_to_service("dap", "dds", "OPeNDAP Data Description", "dap2");
            registry->add_to_service("dap", "dods", "OPeNDAP Data Object", "dap2");
            CPPUNIT_ASSERT( registry->service_available( "dap" ) );
            CPPUNIT_ASSERT( registry->service_available( "dap", "das" ) );
            CPPUNIT_ASSERT( registry->service_available( "dap", "dds" ) );
            CPPUNIT_ASSERT( registry->service_available( "dap", "dods" ) );
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT( !"failed to add service" );
        }

        try {
            cout << "*****************************************" << endl;
            cout << "try to add duplicate dap service with das, dds, dods" << endl;
            registry->add_to_service("dap", "das", "OPeNDAP Data Attributes", "dap2");
            CPPUNIT_ASSERT( !"succeeded, should have failed" );
        }
        catch (BESError &e) {
        }

        try {
            cout << "*****************************************" << endl;
            cout << "add to the dap service" << endl;
            registry->add_to_service("dap", "ascii", "OPeNDAP ASCII data", "dap2");
            registry->add_to_service("dap", "info_page", "OPeNDAP Data Information", "dap2");
            registry->add_to_service("dap", "html_form", "OPeNDAP Form for access", "dap2");
            CPPUNIT_ASSERT( registry->service_available( "dap", "ascii" ) );
            CPPUNIT_ASSERT(registry->service_available("dap", "info_page"));
            CPPUNIT_ASSERT(registry->service_available("dap", "html_form"));
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT( !"failed to add to the dap service" );
        }

        try {
            cout << "*****************************************" << endl;
            cout << "try to add duplicate cmd to dap service" << endl;
            registry->add_to_service("dap", "ascii", "OPeNDAP ASCII data", "dap2");
            CPPUNIT_ASSERT( !"succeeded, should have failed" );
        }
        catch (BESError &e) {
        }

        try {
            cout << "*****************************************" << endl;
            cout << "try to add cedar services" << endl;
            registry->add_service("cedar");
            registry->add_to_service("cedar", "flat", "CEDAR flat format data response", "cedar");
            registry->add_to_service("cedar", "tab", "CEDAR tab separated data response", "cedar");
            registry->add_to_service("cedar", "stream", "CEDAR stream .cbf data file", "cedar");
            CPPUNIT_ASSERT(registry->service_available("cedar"));
            CPPUNIT_ASSERT(registry->service_available("cedar", "flat"));
            CPPUNIT_ASSERT(registry->service_available("cedar", "tab"));
            CPPUNIT_ASSERT(registry->service_available("cedar", "stream"));
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT( !"failed to add cedar services" );
        }

        try {
            cout << "*****************************************" << endl;
            cout << "try to add a format to the dap data response" << endl;
            registry->add_format("dap", "dods", "netcdf");
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT( !"failed to add format" );
        }

        try {
            cout << "*****************************************" << endl;
            cout << "try to re-add a format to the dap data response" << endl;
            registry->add_format("dap", "dods", "netcdf");
            CPPUNIT_ASSERT( !"success, should have failed" );
        }
        catch (BESError &e) {
        }

        try {
            cout << "*****************************************" << endl;
            cout << "add format to non-existant service" << endl;
            registry->add_format("nogood", "dods", "netcdf");
            CPPUNIT_ASSERT( !"success, should have failed" );
        }
        catch (BESError &e) {
        }

        try {
            cout << "*****************************************" << endl;
            cout << "add format to non-existant cmd" << endl;
            registry->add_format("dap", "nocmd", "netcdf");
            CPPUNIT_ASSERT( !"success, should have failed" );
        }
        catch (BESError &e) {
        }

        CPPUNIT_ASSERT( registry->service_available( "dap" ) );
        CPPUNIT_ASSERT( registry->service_available( "dap", "ascii" ) );
        CPPUNIT_ASSERT( !registry->service_available( "not" ) );
        CPPUNIT_ASSERT( !registry->service_available( "not", "ascii" ) );
        CPPUNIT_ASSERT( !registry->service_available( "not", "nono" ) );
        CPPUNIT_ASSERT( !registry->service_available( "dap", "nono" ) );

        try {
            cout << "*****************************************" << endl;
            cout << "handler services" << endl;
            registry->handles_service("nc", "dap");
            registry->handles_service("cedar", "dap");
            registry->handles_service("cedar", "cedar");
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT( !"handles_service calls failed" );
        }

        try {
            cout << "*****************************************" << endl;
            cout << "dump the registry" << endl;
            ostringstream strm;
            strm << *registry;
            string res = strm.str();
            string::size_type spos = res.find("(0x");
            string::size_type epos = res.find(")", spos);
            CPPUNIT_ASSERT( spos != string::npos );
            CPPUNIT_ASSERT( epos != string::npos );
            res.replace(spos + 1, epos - spos - 1, "X");
            CPPUNIT_ASSERT( res == dump1 );
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT( !"failed to dump the registry" );
        }

        try {
            cout << "*****************************************" << endl;
            cout << "try to add command to service that doesn't exist" << endl;
            registry->add_to_service("notexist", "something", "something description", "something format");
            CPPUNIT_ASSERT( !"success, should have failed" );
        }
        catch (const BESError &e) {
        }

        try {
            cout << "*****************************************" << endl;
            cout << "handle service that does not exist" << endl;
            registry->handles_service("csv", "noexist");
            CPPUNIT_ASSERT( !"success, should have failed" );
        }
        catch (const BESError &e) {
        }


        cout << "*****************************************" << endl;
        cout << "does handle?" << endl;
        CPPUNIT_ASSERT( registry->does_handle_service( "nc", "dap" ) );
        CPPUNIT_ASSERT( registry->does_handle_service( "cedar", "cedar" ) );
        CPPUNIT_ASSERT( !registry->does_handle_service( "nc", "ascii" ) );
        CPPUNIT_ASSERT( !registry->does_handle_service( "noexist", "dap" ) );
        CPPUNIT_ASSERT( !registry->does_handle_service( "nc", "noexist" ) );
        CPPUNIT_ASSERT(!registry->does_handle_service("noexist", "noexist"));

        {
            cout << "*****************************************" << endl;
            cout << "services handled by cedar" << endl;
            list<string> services;
            registry->services_handled("cedar", services);
            map<string, string> baseline;
            baseline["dap"] = "dap";
            baseline["cedar"] = "cedar";
            CPPUNIT_ASSERT( services.size() == baseline.size() );

            list<string>::const_iterator si = services.begin();
            list<string>::const_iterator se = services.end();
            for (; si != se; si++) {
                cout << "    " << (*si) << endl;
                map<string, string>::iterator fi = baseline.find((*si));
                CPPUNIT_ASSERT( fi != baseline.end() );
            }
        }

        {
            cout << "*****************************************" << endl;
            cout << "services handled by nc" << endl;
            list<string> services;
            registry->services_handled("nc", services);
            map<string, string> baseline;
            baseline["dap"] = "dap";
            CPPUNIT_ASSERT( services.size() == baseline.size() );
            list<string>::const_iterator si = services.begin();
            list<string>::const_iterator se = services.end();
            for (; si != se; si++) {
                cout << "    " << (*si) << endl;
                map<string, string>::iterator fi = baseline.find((*si));
                CPPUNIT_ASSERT( fi != baseline.end() );
            }
        }

        try {
            cout << "*****************************************" << endl;
            cout << "show services" << endl;
            BESXMLInfo info;
            BESDataHandlerInterface dhi;
            dhi.data[REQUEST_ID_KEY] = "123456789";
            dhi.data[REQUEST_UUID_KEY] = "0nce-up0n-a-t1m3-1n-th3-w3st";
            info.begin_response("showServices", dhi);
            registry->show_services(info);
            info.end_response();
            ostringstream strm;
            info.print(strm);
            string str = strm.str();

            // we need to remove the first part of the response because the
            // order of the attributes can be different between machines. So
            // drop the <!xml and <response tags
            string::size_type sd = str.find("<showServices>");
            CPPUNIT_ASSERT( sd != string::npos );

            string cmp_str = str.substr(sd);
            cout << "received = " << endl << cmp_str << endl;
            cout << "expecting = " << endl << show1 << endl;

            CPPUNIT_ASSERT( cmp_str == show1 );
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT( !"failed to show services" );
        }

        try {
            cout << "*****************************************" << endl;
            cout << "remove service" << endl;
            registry->remove_service("dap");
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT( !"failed to remove the service" );
        }

        try {
            cout << "*****************************************" << endl;
            cout << "show services" << endl;
            BESXMLInfo info;
            BESDataHandlerInterface dhi;
            dhi.data[REQUEST_ID_KEY] = "123456789";
            info.begin_response("showServices", dhi);
            registry->show_services(info);
            info.end_response();
            ostringstream strm;
            info.print(strm);
            string str = strm.str();
            cout << "received = " << endl << str << endl;
            cout << "expecting = " << endl << show2 << endl;

            // we need to remove the first part of the response because the
            // order of the attributes can be different between machines. So
            // drop the <!xml and <response tags
            string::size_type sd = str.find("<showServices>");
            CPPUNIT_ASSERT( sd != string::npos );
            string cmp_str = str.substr(sd);

            CPPUNIT_ASSERT( cmp_str == show2 );
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT( !"failed to show services" );
        }

        try {
            cout << "*****************************************" << endl;
            cout << "dump the registry" << endl;
            ostringstream strm;
            strm << *registry;
            string res = strm.str();
            string::size_type spos = res.find("(0x");
            string::size_type epos = res.find(")", spos);
            CPPUNIT_ASSERT( spos != string::npos );
            CPPUNIT_ASSERT( epos != string::npos );

            res.replace(spos + 1, epos - spos - 1, "X");
            CPPUNIT_ASSERT( res == dump2 );
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT( !"failed to dump the registry" );
        }

        cout << "*****************************************" << endl;
        cout << "Returning from servicesT::run" << endl;
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( servicesT );

int main(int argc, char*argv[])
{
    int option_char;
    while ((option_char = getopt(argc, argv, "dh")) != EOF)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;
        case 'h': {     // help - show test names
            cerr << "Usage: servicesT has the following tests:" << endl;
            const std::vector<Test*> &tests = servicesT::suite()->getTests();
            unsigned int prefix_len = servicesT::suite()->getName().append("::").size();
            for (std::vector<Test*>::const_iterator i = tests.begin(), e = tests.end(); i != e; ++i) {
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
    }
    else {
        int i = 0;
        while (i < argc) {
            if (debug) cerr << "Running " << argv[i] << endl;
            test = servicesT::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}

