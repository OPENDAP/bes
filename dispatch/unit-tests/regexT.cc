// regexT.C

#include <iostream>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "regexT.h"
#include "BESRegex.h"
#include "BESException.h"

int regexT::
run(void)
{
    cout << endl << "*****************************************" << endl;
    cout << "Entered regexT::run" << endl;
    int retVal = 0;

    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "Match reg ex 123456 against string 01234567" << endl;
	BESRegex reg_expr( "123456" ) ;
	string inQuestion = "01234567" ;
	int result = reg_expr.match( inQuestion.c_str(), inQuestion.length() ) ;
	if( result == 6 )
	{
	    cout << "Successfully matched characters" << endl ;
	}
	else
	{
	    cerr << "Regular expression matched " << result
	         << " characets, should have matched 6" << endl ;
	    return 1 ;
	}
    }
    catch( BESException &e )
    {
	cerr << "Failed to match, exception caught" << endl << e.get_message() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "Failed to match, unknown exception caught" << endl ;
	return 1 ;
    }

    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "Match reg ex ^123456$ against string 01234567" << endl;
	BESRegex reg_expr( "^123456$" ) ;
	string inQuestion = "01234567" ;
	int result = reg_expr.match( inQuestion.c_str(), inQuestion.length() ) ;
	if( result == -1 )
	{
	    cout << "Does not match, good" << endl ;
	}
	else
	{
	    cerr << "Regular expression matched " << result
	         << " characets, should have matched none" << endl ;
	    return 1 ;
	}
    }
    catch( BESException &e )
    {
	cerr << "Failed to match, exception caught" << endl << e.get_message() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "Failed to match, unknown exception caught" << endl ;
	return 1 ;
    }

    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "Match reg ex ^123456$ against string 123456" << endl;
    cout << "    besregtest include \"^123456$;\" 123456 matches all 6 of 6 characters" << endl ;
	BESRegex reg_expr( "^123456$" ) ;
	string inQuestion = "123456" ;
	int result = reg_expr.match( inQuestion.c_str(), inQuestion.length() ) ;
	if( result == 6 )
	{
	    cout << "Successfully matched characters" << endl ;
	}
	else
	{
	    cerr << "Regular expression matched " << result
	         << " characets, should have matched 6" << endl ;
	    return 1 ;
	}
    }
    catch( BESException &e )
    {
	cerr << "Failed to match, exception caught" << endl << e.get_message() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "Failed to match, unknown exception caught" << endl ;
	return 1 ;
    }

    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "Match reg ex \".*\\.nc$;\" against string fnoc1.nc" << endl;
	BESRegex reg_expr( ".*\\.nc$" ) ;
	string inQuestion = "fnoc1.nc" ;
	int result = reg_expr.match( inQuestion.c_str(), inQuestion.length() ) ;
	if( result == 8 )
	{
	    cout << "Successfully matched characters" << endl ;
	}
	else
	{
	    cerr << "Regular expression matched " << result
	         << " characets, should have matched 8" << endl ;
	    return 1 ;
	}
    }
    catch( BESException &e )
    {
	cerr << "Failed to match, exception caught" << endl << e.get_message() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "Failed to match, unknown exception caught" << endl ;
	return 1 ;
    }

    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "Match reg ex \".*\\.nc$;\" against string fnoc1.ncd" << endl;
	BESRegex reg_expr( ".*\\.nc$" ) ;
	string inQuestion = "fnoc1.ncd" ;
	int result = reg_expr.match( inQuestion.c_str(), inQuestion.length() ) ;
	if( result == -1 )
	{
	    cout << "Successfully did not match characters" << endl ;
	}
	else
	{
	    cerr << "Regular expression matched " << result
	         << " characets, should have matched none" << endl ;
	    return 1 ;
	}
    }
    catch( BESException &e )
    {
	cerr << "Failed to match, exception caught" << endl << e.get_message() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "Failed to match, unknown exception caught" << endl ;
	return 1 ;
    }

    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "Match reg ex .*\\.(nc|NC)(\\.gz|\\.bz2|\\.Z)?$ against string fnoc1.nc" << endl;
	BESRegex reg_expr( ".*\\.(nc|NC)(\\.gz|\\.bz2|\\.Z)?$" ) ;
	string inQuestion = "fnoc1.nc" ;
	int result = reg_expr.match( inQuestion.c_str(), inQuestion.length() ) ;
	if( result == 8 )
	{
	    cout << "Successfully matched characters" << endl ;
	}
	else
	{
	    cerr << "Regular expression matched " << result
	         << " characets, should have matched 8" << endl ;
	    return 1 ;
	}
    }
    catch( BESException &e )
    {
	cerr << "Failed to match, exception caught" << endl << e.get_message() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "Failed to match, unknown exception caught" << endl ;
	return 1 ;
    }

    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "Match reg ex .*\\.(nc|NC)(\\.gz|\\.bz2|\\.Z)?$ against string fnoc1.nc.gz" << endl;
	BESRegex reg_expr( ".*\\.(nc|NC)(\\.gz|\\.bz2|\\.Z)?$" ) ;
	string inQuestion = "fnoc1.nc.gz" ;
	int result = reg_expr.match( inQuestion.c_str(), inQuestion.length() ) ;
	if( result == 11 )
	{
	    cout << "Successfully matched characters" << endl ;
	}
	else
	{
	    cerr << "Regular expression matched " << result
	         << " characets, should have matched 11" << endl ;
	    return 1 ;
	}
    }
    catch( BESException &e )
    {
	cerr << "Failed to match, exception caught" << endl << e.get_message() << endl ;
	return 1 ;
    }
    catch( ... )
    {
	cerr << "Failed to match, unknown exception caught" << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Returning from regexT::run" << endl;

    return retVal;
}

int
main(int argC, char **argV) {
    Application *app = new regexT();
    return app->main(argC, argV);
}

