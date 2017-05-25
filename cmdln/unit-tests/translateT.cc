// translateT.cc

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

using namespace CppUnit ;

#include <string>
#include <iostream>
#include <sstream>

using std::cout ;
using std::endl ;
using std::string ;
using std::ostringstream ;

#include "CmdTranslation.h"
#include "BESError.h"
#include <test_config.h>
#include <GetOpt.h>

using namespace CppUnit;
using namespace std;

static bool debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);


string d1 = "CmdTranslation::dump\n\
    translations registered\n\
        define\n\
        delete\n\
        get\n\
        set\n\
        set.container\n\
        set.context\n\
        show\n\
        show.catalog\n\
        show.error\n\
        show.info\n" ;

string d2 = "CmdTranslation::dump\n\
    translations registered\n\
        define\n\
        delete\n\
        get\n\
        set\n\
        set.container\n\
        set.context\n\
        show\n\
        show.catalog\n\
        show.error\n\
        show.info\n\
        test\n" ;

string def1 = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\
<request reqID=\"some_unique_value\"><define name=\"d\"><container name=\"c\"/></define></request>\n" ;

string def2 = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\
<request reqID=\"some_unique_value\"><define name=\"d\"><container name=\"c\"><constraint>u</constraint><attributes>u</attributes></container><aggregate handler=\"agg\" cmd=\"u\"/></define></request>\n" ;

string del1 = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\
<request reqID=\"some_unique_value\"><deleteContainer name=\"d\"/></request>\n" ;

string del2 = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\
<request reqID=\"some_unique_value\"><deleteContainer name=\"d\" space=\"catalog\"/></request>\n" ;

string del3 = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\
<request reqID=\"some_unique_value\"><deleteDefinition name=\"d\"/></request>\n" ;

string del4 = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\
<request reqID=\"some_unique_value\"><deleteDefinition name=\"d\" space=\"catalog\"/></request>\n" ;

string del5 = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\
<request reqID=\"some_unique_value\"><deleteContainers/></request>\n" ;

string del6 = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\
<request reqID=\"some_unique_value\"><deleteContainers space=\"catalog\"/></request>\n" ;

string del7 = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\
<request reqID=\"some_unique_value\"><deleteDefinitions/></request>\n" ;

string del8 = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\
<request reqID=\"some_unique_value\"><deleteDefinitions space=\"catalog\"/></request>\n" ;

string get1 = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\
<request reqID=\"some_unique_value\"><get type=\"das\" definition=\"d\"/></request>\n" ;

string get2 = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\
<request reqID=\"some_unique_value\"><get type=\"das\" definition=\"d\" returnAs=\"nc\"/></request>\n" ;

string set1 = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\
<request reqID=\"some_unique_value\"><setContainer name=\"c\" type=\"nc\">data/nc/fnoc1.nc</setContainer></request>\n" ;

string set2 = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\
<request reqID=\"some_unique_value\"><setContainer name=\"c\" space=\"catalog\">data/nc/fnoc1.nc</setContainer></request>\n" ;

string set3 = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\
<request reqID=\"some_unique_value\"><setContext name=\"format\">dap2</setContext></request>\n" ;

string show1 = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\
<request reqID=\"some_unique_value\"><showStatus/></request>\n" ;

string show2 = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\
<request reqID=\"some_unique_value\"><showVersion/></request>\n" ;

string cat1 = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\
<request reqID=\"some_unique_value\"><showCatalog/></request>\n" ;

string cat2 = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\
<request reqID=\"some_unique_value\"><showCatalog node=\"data\"/></request>\n" ;

string info1 = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\
<request reqID=\"some_unique_value\"><showInfo/></request>\n" ;

string info2 = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\
<request reqID=\"some_unique_value\"><showInfo node=\"data\"/></request>\n" ;

bool translate_test( BESTokenizer &/*tokenizer*/, xmlTextWriterPtr /*writer*/ )
{
    return true ;
}

class translateT: public TestFixture {
private:

public:
    translateT() {}
    ~translateT() {}

    void setUp()
    {
    } 

    void tearDown()
    {
    }

    CPPUNIT_TEST_SUITE( translateT ) ;

    CPPUNIT_TEST( do_test ) ;

    CPPUNIT_TEST_SUITE_END() ;

    void do_test()
    {
	cout << endl << "*****************************************" << endl;
	cout << "Entered translateT::run" << endl;

	try
	{
	    CmdTranslation::initialize( 0, 0 ) ;

	    {
		ostringstream dstrm ;
		CmdTranslation::dump( dstrm ) ;
		cout << dstrm.str() << endl ;
		CPPUNIT_ASSERT( dstrm.str() == d1 ) ;
	    }

	    CmdTranslation::add_translation( "test", translate_test ) ;
	    {
		ostringstream dstrm ;
		CmdTranslation::dump( dstrm ) ;
		cout << dstrm.str() << endl ;
		CPPUNIT_ASSERT( dstrm.str() == d2 ) ;
	    }

	    CmdTranslation::remove_translation( "test" ) ;
	    {
		ostringstream dstrm ;
		CmdTranslation::dump( dstrm ) ;
		cout << dstrm.str() << endl ;
		CPPUNIT_ASSERT( dstrm.str() == d1 ) ;
	    }

	    // test each command for accuracy
	    string doc = CmdTranslation::translate( "unknown" ) ;
	    CPPUNIT_ASSERT( doc.empty() ) ;

	    doc = CmdTranslation::translate( "unknown;" ) ;
	    CPPUNIT_ASSERT( doc.empty() ) ;

	    // define
	    doc = CmdTranslation::translate( "define d as c;" ) ;
	    cout << doc << endl ;
	    CPPUNIT_ASSERT( doc == def1 ) ;

	    doc = CmdTranslation::translate( "define d as c with c.constraint=\"u\",c.attributes=\"u\" aggregate using agg by \"u\";" ) ;
	    cout << doc << endl ;
	    CPPUNIT_ASSERT( doc == def2 ) ;

	    // delete
	    doc = CmdTranslation::translate( "delete container d;" ) ;
	    cout << doc << endl ;
	    CPPUNIT_ASSERT( doc == del1 ) ;

	    doc = CmdTranslation::translate( "delete container d from catalog;" ) ;
	    cout << doc << endl ;
	    CPPUNIT_ASSERT( doc == del2 ) ;

	    doc = CmdTranslation::translate( "delete definition d;" ) ;
	    cout << doc << endl ;
	    CPPUNIT_ASSERT( doc == del3 ) ;

	    doc = CmdTranslation::translate( "delete definition d from catalog;" ) ;
	    cout << doc << endl ;
	    CPPUNIT_ASSERT( doc == del4 ) ;

	    doc = CmdTranslation::translate( "delete containers;" ) ;
	    cout << doc << endl ;
	    CPPUNIT_ASSERT( doc == del5 ) ;

	    doc = CmdTranslation::translate( "delete containers from catalog;" ) ;
	    cout << doc << endl ;
	    CPPUNIT_ASSERT( doc == del6 ) ;

	    doc = CmdTranslation::translate( "delete definitions;" ) ;
	    cout << doc << endl ;
	    CPPUNIT_ASSERT( doc == del7 ) ;

	    doc = CmdTranslation::translate( "delete definitions from catalog;" ) ;
	    cout << doc << endl ;
	    CPPUNIT_ASSERT( doc == del8 ) ;

	    // get
	    doc = CmdTranslation::translate( "get das for d;" ) ;
	    cout << doc << endl ;
	    CPPUNIT_ASSERT( doc == get1 ) ;

	    doc = CmdTranslation::translate( "get das for d return as nc;" ) ;
	    cout << doc << endl ;
	    CPPUNIT_ASSERT( doc == get2 ) ;

	    // set
	    // set.container
	    doc = CmdTranslation::translate( "set container values c,data/nc/fnoc1.nc,nc;" ) ;
	    cout << doc << endl ;
	    CPPUNIT_ASSERT( doc == set1 ) ;

	    doc = CmdTranslation::translate( "set container in catalog values c,data/nc/fnoc1.nc;" ) ;
	    cout << doc << endl ;
	    CPPUNIT_ASSERT( doc == set2 ) ;

	    // set.context
	    doc = CmdTranslation::translate( "set context format to dap2;" ) ;
	    cout << doc << endl ;
	    CPPUNIT_ASSERT( doc == set3 ) ;

	    // unkown set command
	    doc = CmdTranslation::translate( "set unkown;" ) ;
	    CPPUNIT_ASSERT( doc.empty() ) ;

	    // show
	    doc = CmdTranslation::translate( "show status;" ) ;
	    cout << doc << endl ;
	    CPPUNIT_ASSERT( doc == show1 ) ;

	    doc = CmdTranslation::translate( "show version;" ) ;
	    cout << doc << endl ;
	    CPPUNIT_ASSERT( doc == show2 ) ;

	    // show.catalog
	    doc = CmdTranslation::translate( "show catalog;" ) ;
	    cout << doc << endl ;
	    CPPUNIT_ASSERT( doc == cat1 ) ;

	    doc = CmdTranslation::translate( "show catalog for \"data\";" ) ;
	    cout << doc << endl ;
	    cout << cat2 << endl ;
	    CPPUNIT_ASSERT( doc == cat2 ) ;

	    // show.info
	    doc = CmdTranslation::translate( "show info;" ) ;
	    cout << doc << endl ;
	    CPPUNIT_ASSERT( doc == info1 ) ;

	    doc = CmdTranslation::translate( "show info for \"data\";" ) ;
	    cout << doc << endl ;
	    CPPUNIT_ASSERT( doc == info2 ) ;

	    CmdTranslation::terminate() ;
	}
	catch( BESError &e )
	{
	    cout << "failed with exception" << endl << e.get_message() << endl ;
	    CPPUNIT_ASSERT( false ) ;
	}

	cout << endl << "*****************************************" << endl;
	cout << "Leaving translateT::run" << endl;
    }

} ;

CPPUNIT_TEST_SUITE_REGISTRATION( translateT ) ;


int main(int argc, char *argv[])
{
    GetOpt getopt(argc, argv, "dh");
    char option_char;

    while ((option_char = getopt()) != -1)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;

        case 'h': {     // help - show test names
            cerr << "Usage: translateT has the following tests:" << endl;
            const std::vector<Test*> &tests = translateT::suite()->getTests();
            unsigned int prefix_len = translateT::suite()->getName().append("::").length();
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
        for (; i < argc; ++i) {
            if (debug) cerr << "Running " << argv[i] << endl;
            test = translateT::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
