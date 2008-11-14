// propsT.cc

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

using namespace CppUnit ;

#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>

#include <string>
#include <iostream>
#include <sstream>
#include <map>

using std::cout ;
using std::endl ;
using std::string ;
using std::ostringstream ;
using std::map ;

#include "BESXMLUtils.h"
#include "BESSyntaxUserError.h"
#include <test_config.h>

xmlDoc *doc = NULL ;

class propsT: public TestFixture {
private:

public:
    propsT() {}
    ~propsT() {}

    void setUp()
    {
	LIBXML_TEST_VERSION
    } 

    void tearDown()
    {
	if( doc ) xmlFreeDoc( doc ) ;

	xmlCleanupParser() ;
    }

    CPPUNIT_TEST_SUITE( propsT ) ;

    CPPUNIT_TEST( do_test ) ;

    CPPUNIT_TEST_SUITE_END() ;

    void do_test()
    {
	cout << endl << "*****************************************" << endl;
	cout << "Entered propsT::run" << endl;

	try
	{
	    string myDoc = "<?xml version=\"1.0\" encoding=\"UTF-8\"?> \
<request prop1=\"val1\" prop2=\"val2\"> \
</request>" ; 

	    xmlNode *root_element = NULL ;

	    doc = xmlParseDoc( (unsigned char *)myDoc.c_str() ) ;
	    if( doc == NULL )
	    {
		string err = "Problem parsing the request xml document" ;
		throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
	    }

	    // get the root element and make sure it exists and is called
	    // request
	    root_element = xmlDocGetRootElement( doc ) ;
	    if( !root_element )
	    {
		string err = "There is no root element in the xml document" ;
		throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
	    }
	    if( (string)(char *)root_element->name != "request" )
	    {
		string err = (string)"The root element not request element, "
			     + "name is " + (char *)root_element->name ;
		throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
	    }

	    // there should be a request id property with one value.
	    map< string, string> props ;
	    BESXMLUtils::GetProps( root_element, props ) ;
	    string &prop_val = props["prop1"] ;
	    cout << "  val: " << prop_val << endl ;
	    CPPUNIT_ASSERT( prop_val == "val1" ) ;

	    prop_val = props["prop2"] ;
	    cout << "  val: " << prop_val << endl ;
	    CPPUNIT_ASSERT( prop_val == "val2" ) ;

	}
	catch( BESError &e )
	{
	    cout << "failed with exception" << endl << e.get_message()
		 << endl ;
	    CPPUNIT_ASSERT( false ) ;
	}

	cout << endl << "*****************************************" << endl;
	cout << "Leaving propsT::run" << endl;
    }

} ;

CPPUNIT_TEST_SUITE_REGISTRATION( propsT ) ;

int 
main( int, char** )
{
    string env_var = (string)"BES_CONF=" + TEST_SRC_DIR + "/bes.conf" ;
    putenv( (char *)env_var.c_str() ) ;

    CppUnit::TextTestRunner runner ;
    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() ) ;

    bool wasSuccessful = runner.run( "", false )  ;

    return wasSuccessful ? 0 : 1 ;
}

