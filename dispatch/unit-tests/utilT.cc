// utilT.C

#include <iostream>
#include <fstream>
#include <cstdlib>

using std::cerr ;
using std::cout ;
using std::endl ;
using std::ifstream ;

#include "utilT.h"
#include "BESUtil.h"
#include "BESError.h"
#include "test_config.h"

int
utilT::run(void)
{
    cout << endl << "*****************************************" << endl;
    cout << "Entered utilT::run" << endl;
    int retVal = 0;

    cout << endl << "*****************************************" << endl;
    cout << "Remove escaped quotes" << endl;
    string s = BESUtil::unescape( "\\\"This is a test, this is \\\"ONLY\\\" a test\\\"" ) ;
    string result = "\"This is a test, this is \"ONLY\" a test\"" ;
    if( s != result )
    {
	cerr << "resulting string incorrect: " << s << " should be " << result << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Remove Leading and Trailing Blanks" << endl;
    s = "This is a test" ;
    result = s ;
    BESUtil::removeLeadingAndTrailingBlanks( s ) ;
    if( s != result )
    {
	cerr << "resulting string incorrect: \"" << s
	     << "\" should be \"" << result << "\"" << endl ;
	return 1 ;
    }

    s = " This is a test" ;
    BESUtil::removeLeadingAndTrailingBlanks( s ) ;
    if( s != result )
    {
	cerr << "resulting string incorrect: \"" << s
	     << "\" should be \"" << result << "\"" << endl ;
	return 1 ;
    }

    s = "	This is a test" ;
    BESUtil::removeLeadingAndTrailingBlanks( s ) ;
    if( s != result )
    {
	cerr << "resulting string incorrect: \"" << s
	     << "\" should be \"" << result << "\"" << endl ;
	return 1 ;
    }

    s = "    This is a test" ;
    BESUtil::removeLeadingAndTrailingBlanks( s ) ;
    if( s != result )
    {
	cerr << "resulting string incorrect: \"" << s
	     << "\" should be \"" << result << "\"" << endl ;
	return 1 ;
    }

    s = "    	This is a test" ;
    BESUtil::removeLeadingAndTrailingBlanks( s ) ;
    if( s != result )
    {
	cerr << "resulting string incorrect: \"" << s
	     << "\" should be \"" << result << "\"" << endl ;
	return 1 ;
    }

    s = "    	This is a test " ;
    BESUtil::removeLeadingAndTrailingBlanks( s ) ;
    if( s != result )
    {
	cerr << "resulting string incorrect: \"" << s
	     << "\" should be \"" << result << "\"" << endl ;
	return 1 ;
    }

    s = "    	This is a test    " ;
    BESUtil::removeLeadingAndTrailingBlanks( s ) ;
    if( s != result )
    {
	cerr << "resulting string incorrect: \"" << s
	     << "\" should be \"" << result << "\"" << endl ;
	return 1 ;
    }

    s = "    	This is a test    	" ;
    BESUtil::removeLeadingAndTrailingBlanks( s ) ;
    if( s != result )
    {
	cerr << "resulting string incorrect: \"" << s
	     << "\" should be \"" << result << "\"" << endl ;
	return 1 ;
    }

    s = "This is a test    " ;
    BESUtil::removeLeadingAndTrailingBlanks( s ) ;
    if( s != result )
    {
	cerr << "resulting string incorrect: \"" << s
	     << "\" should be \"" << result << "\"" << endl ;
	return 1 ;
    }

    s = "This is a test    	" ;
    BESUtil::removeLeadingAndTrailingBlanks( s ) ;
    if( s != result )
    {
	cerr << "resulting string incorrect: \"" << s
	     << "\" should be \"" << result << "\"" << endl ;
	return 1 ;
    }

    s = " 	This is a test 	\n" ;
    BESUtil::removeLeadingAndTrailingBlanks( s ) ;
    if( s != result )
    {
	cerr << "resulting string incorrect: \"" << s
	     << "\" should be \"" << result << "\"" << endl ;
	return 1 ;
    }

    s = "    " ;
    result = "" ;
    BESUtil::removeLeadingAndTrailingBlanks( s ) ;
    if( s != result )
    {
	cerr << "resulting string incorrect: \"" << s
	     << "\" should be \"" << result << "\"" << endl ;
	return 1 ;
    }

    s = "    	" ;
    BESUtil::removeLeadingAndTrailingBlanks( s ) ;
    if( s != result )
    {
	cerr << "resulting string incorrect: \"" << s
	     << "\" should be \"" << result << "\"" << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Exploding delimited strings" << endl;
    list<string> values ;

    string value = "val1,val2,val3,val4" ;
    cout << value << endl ;
    BESUtil::explode( ',', value, values ) ;
    display_values( values ) ;
    if( values.size() != 4 )
    {
	cerr << "should be 4 values, have " << values.size() << endl ;
	return 1 ;
    }
    list<string>::iterator i = values.begin() ;
    list<string>::iterator i1 = i++ ;
    list<string>::iterator i2 = i++ ;
    list<string>::iterator i3 = i++ ;
    list<string>::iterator i4 = i++ ;
    if( (*i1) != "val1"
        || (*i2) != "val2"
        || (*i3) != "val3"
        || (*i4) != "val4" )
    {
	cout << (*i1) << endl ;
	cout << (*i2) << endl ;
	cout << (*i3) << endl ;
	cout << (*i4) << endl ;
	cerr << "bad values" << endl ;
	return 1 ;
    }

    values.clear() ;

    value = "val1,val2,val3,val4," ;
    cout << value << endl ;
    BESUtil::explode( ',', value, values ) ;
    display_values( values ) ;
    if( values.size() != 4 )
    {
	cerr << "should be 4 values, have " << values.size() << endl ;
	return 1 ;
    }
    i = values.begin() ;
    i1 = i++ ;
    i2 = i++ ;
    i3 = i++ ;
    i4 = i++ ;
    if( (*i1) != "val1"
        || (*i2) != "val2"
        || (*i3) != "val3"
        || (*i4) != "val4" )
    {
	cout << (*i1) << endl ;
	cout << (*i2) << endl ;
	cout << (*i3) << endl ;
	cout << (*i4) << endl ;
	cerr << "bad values" << endl ;
	return 1 ;
    }

    values.clear() ;

    value = "val1;\"val2 with quotes\";val3;\"val4 with quotes\"" ;
    cout << value << endl ;
    BESUtil::explode( ';', value, values ) ;
    display_values( values ) ;
    if( values.size() != 4 )
    {
	cerr << "should be 4 values, have " << values.size() << endl ;
	return 1 ;
    }
    i = values.begin() ;
    i1 = i++ ;
    i2 = i++ ;
    i3 = i++ ;
    i4 = i++ ;
    if( (*i1) != "val1"
        || (*i2) != "\"val2 with quotes\""
        || (*i3) != "val3"
        || (*i4) != "\"val4 with quotes\"" )
    {
	cout << (*i1) << endl ;
	cout << (*i2) << endl ;
	cout << (*i3) << endl ;
	cout << (*i4) << endl ;
	cerr << "bad values" << endl ;
	return 1 ;
    }

    values.clear() ;

    value = "val1;\"val2 with \\\"embedded quotes\\\"\";val3;\"val4 with quotes\";" ;
    cout << value << endl ;
    BESUtil::explode( ';', value, values ) ;
    display_values( values ) ;
    if( values.size() != 4 )
    {
	cerr << "should be 4 values, have " << values.size() << endl ;
	return 1 ;
    }
    i = values.begin() ;
    i1 = i++ ;
    i2 = i++ ;
    i3 = i++ ;
    i4 = i++ ;
    if( (*i1) != "val1"
        || (*i2) != "\"val2 with \\\"embedded quotes\\\"\""
        || (*i3) != "val3"
        || (*i4) != "\"val4 with quotes\"" )
    {
	cout << (*i1) << endl ;
	cout << (*i2) << endl ;
	cout << (*i3) << endl ;
	cout << (*i4) << endl ;
	cerr << "bad values" << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Returning from utilT::run" << endl;

    return retVal;
}

void
utilT::display_values( const list<string> &values )
{
    list<string>::const_iterator i = values.begin() ;
    list<string>::const_iterator e = values.end() ;
    for( ; i != e; i++ )
    {
	cout << "  " << (*i) << endl ;
    }
}

int
main(int argC, char **argV) {
    string env_var = (string)"BES_CONF=" + TEST_SRC_DIR + "/bes.conf" ;
    putenv( (char *)env_var.c_str() ) ;
    Application *app = new utilT();
    return app->main(argC, argV);
}

