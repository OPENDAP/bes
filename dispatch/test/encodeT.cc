// encodeT.cc

#include <iostream>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "encodeT.h"
#include "DODSEncode.h"
#include "TheDODSKeys.h"
#include "DODSMySQLAuthenticate.h"
#include "DODSDataRequestInterface.h"
#include "DODSException.h"

void
usage( string name_program )
{
    cout << "Usage: " << name_program
         << " encode text[must be 8 characters] key[must be 8 characters]"
	 << endl ;
    cout << "or" << endl ;
    cout << "Usage: " << name_program
         << " decode entext[must be 64 characters] key[must be 8 characters]"
	 << endl ;
}

int
encodeT::initialize(int argC, char **argV)
{
    _app = argV[0] ;

    if( argC == 1 )
    {
	_test = true ;
	return 0 ;
    }

    if( argC != 4 )
    {
	cout << "Incorrect number of arguments, got " << argC << endl ;
	usage( _app ) ;
	return 1 ;
    }
    _action = argV[1] ;
    _var1 = argV[2] ;
    _var2 = argV[3] ;

    return 0 ;
}

int
encodeT::run_test(void)
{
    cout << endl << "*****************************************" << endl;
    cout << "Entered encodeT::run" << endl;

    // try text < 8, == 8, > 8
    {
	cout << endl << "*****************************************" << endl;
	cout << "text with length < 8" << endl;
	string key = "test_key" ;
	string text1 = "012345" ;
	string b1 = DODSEncode::encode( text1.c_str(), key.c_str() ) ;
	string b2 = DODSEncode::decode( b1.c_str(), key.c_str() ) ;
	if( text1 == b2 )
	{
	    cout << "encoded and decoded correctly" << endl ;
	}
	else
	{
	    cerr << "incorrectly encoded and/or decoded" << endl ;
	    cerr << "encoded_text = " << b1 << endl ;
	    cerr << "decoded_text = " << b2 << endl ;
	    return 1 ;
	}
    }

    {
	cout << endl << "*****************************************" << endl;
	cout << "text with length = 8" << endl;
	string key = "test_key" ;
	string text1 = "01234567" ;
	string b1 = DODSEncode::encode( text1.c_str(), key.c_str() ) ;
	string b2 = DODSEncode::decode( b1.c_str(), key.c_str() ) ;
	if( text1 == b2 )
	{
	    cout << "encoded and decoded correctly" << endl ;
	}
	else
	{
	    cerr << "incorrectly encoded and/or decoded" << endl ;
	    cerr << "encoded_text = " << b1 << endl ;
	    cerr << "decoded_text = " << b2 << endl ;
	    return 1 ;
	}
    }

    {
	cout << endl << "*****************************************" << endl;
	cout << "text with length > 8" << endl;
	string key = "test_key" ;
	string text1 = "0123456789012345" ;
	string b1 = DODSEncode::encode( text1.c_str(), key.c_str() ) ;
	string b2 = DODSEncode::decode( b1.c_str(), key.c_str() ) ;
	if( text1 == b2 )
	{
	    cout << "encoded and decoded correctly" << endl ;
	}
	else
	{
	    cerr << "incorrectly encoded and/or decoded" << endl ;
	    cerr << "encoded_text = " << b1 << endl ;
	    cerr << "decoded_text = " << b2 << endl ;
	    return 1 ;
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "Returning from encodeT::run" << endl;

    return 0 ;
}

int
encodeT::run(void)
{
    if( _test ) return run_test() ;

    if( _action == "encode" )
    {
	// load and confirm arguments
	if( _var2.length() != 8 )
	{
	    usage( _app ) ;
	    return 1 ;
	}

	// decode data and display
	string b1 = DODSEncode::encode( _var1.c_str(), _var2.c_str() ) ;
	cout << "\"" << b1 << "\"" << endl ;
    }
    else  if( _action == "decode" )
    {
	// load and confirm arguments
	if( _var2.length() != 8 )
	{
	    usage( _app ) ;
	    return 1 ;
	}

	// decode data and display
	string b2 = DODSEncode::decode( _var1.c_str(), _var2.c_str() ) ;
	cout << "\"" << b2 << "\"" << endl ;
    }
    else
    {
	usage( _app ) ;
	return 1 ;
    }
    return 0 ;
}

int
main(int argC, char **argV) {
    Application *app = new encodeT();
    return app->main(argC, argV);
}

