// scrubT.C

#include <iostream>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "scrubT.h"
#include "BESScrub.h"
#include "BESException.h"

int scrubT::
run(void)
{
    cout << endl << "*****************************************" << endl;
    cout << "Entered scrubT::run" << endl;
    int retVal = 0;

    {
	cout << endl << "*****************************************" << endl;
	cout << "Test command line length over 255 characters" << endl;
	char arg[512] ;
	memset( arg, 'a', 300 ) ;
	arg[300] = '\0' ;
	if( BESScrub::command_line_arg_ok( arg ) )
	{
	    cerr << "command line ok, shouldn't be" << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "command line not ok, good" << endl ;
	}
    }

    {
	cout << endl << "*****************************************" << endl;
	cout << "Test command line length ok" << endl;
	string arg = "anarg" ;
	if( BESScrub::command_line_arg_ok( arg ) )
	{
	    cout << "command line ok, good" << endl ;
	}
	else
	{
	    cerr << "command line not ok, should be" << endl ;
	    return 1 ;
	}
    }

    {
	cout << endl << "*****************************************" << endl;
	cout << "Test path name length over 255 characters" << endl;
	char path_name[512] ;
	memset( path_name, 'a', 300 ) ;
	path_name[300] = '\0' ;
	if( BESScrub::pathname_ok( path_name, true ) )
	{
	    cerr << "path name ok, shouldn't be" << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "path name not ok, good" << endl ;
	}
    }

    {
	cout << endl << "*****************************************" << endl;
	cout << "Test path name good" << endl;
	if( BESScrub::pathname_ok( "/usr/local/something_goes_here-and-is-ok.txt", true ) )
	{
	    cout << "path name ok, good" << endl ;
	}
	else
	{
	    cerr << "path name not ok, should be" << endl ;
	    return 1 ;
	}
    }

    {
	cout << endl << "*****************************************" << endl;
	cout << "Test path name bad characters strict" << endl;
	if( BESScrub::pathname_ok( "*$^&;@/user/local/bin/ls", true ) )
	{
	    cerr << "path name ok, shouldn't be" << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "path name not ok, good" << endl ;
	}
    }

    {
	cout << endl << "*****************************************" << endl;
	cout << "Test array size too big" << endl;
	if( BESScrub::size_ok( 4, UINT_MAX ) )
	{
	    cerr << "array size ok, shouldn't be" << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "array size not ok, good" << endl ;
	}
    }

    {
	cout << endl << "*****************************************" << endl;
	cout << "Test array size ok" << endl;
	if( BESScrub::size_ok( 4, 32 ) )
	{
	    cout << "array size ok, good" << endl ;
	}
	else
	{
	    cerr << "array size not ok, should be" << endl ;
	    return 1 ;
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "Returning from scrubT::run" << endl;

    return retVal;
}

int
main(int argC, char **argV) {
    Application *app = new scrubT();
    return app->main(argC, argV);
}

