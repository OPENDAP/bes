// resplistT.C

#include <iostream>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "resplistT.h"
#include "DODSResponseHandlerList.h"
#include "TestResponseHandler.h"

int resplistT::
run(void) {
    cout << endl << "*****************************************" << endl;
    cout << "Entered resplistT::run" << endl;
    int retVal = 0;

    cout << endl << "*****************************************" << endl;
    cout << "add the 5 response handlers" << endl ;
    DODSResponseHandlerList *rhl = DODSResponseHandlerList::TheList() ;
    char num[10] ;
    for( int i = 0; i < 5; i++ )
    {
	sprintf( num, "resp%d", i ) ;
	if( rhl->add_handler( num, TestResponseHandler::TestResponseBuilder ) == true )
	{
	    cout << "successfully added " << num << endl ;
	}
	else
	{
	    cerr << "failed to add " << num << endl ;
	    return 1 ;
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "try to add resp3 again" << endl ;
    if( rhl->add_handler( "resp3", TestResponseHandler::TestResponseBuilder ) == true )
    {
	cerr << "successfully added resp3 again" << endl ;
	return 1 ;
    }
    else
    {
	cout << "failed to add resp3 again, good" << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "finding the handlers" << endl ;
    for( int i = 4; i >= 0; i-- )
    {
	sprintf( num, "resp%d", i ) ;
	DODSResponseHandler *rh = rhl->find_handler( num ) ;
	if( rh )
	{
	    if( rh->get_name() == num )
	    {
		cout << "found " << num << endl ;
		delete rh ;
	    }
	    else
	    {
		cerr << "looking for " << num
		     << ", found " << rh->get_name() << endl ;
		delete rh ;
		return 1 ;
	    }
	}
	else
	{
	    cerr << "coundn't find " << num << endl ;
	    return 1 ;
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "removing resp2" << endl ;
    if( rhl->remove_handler( "resp2" ) == true )
    {
	DODSResponseHandler *rh = rhl->find_handler( "resp2" ) ;
	if( rh )
	{
	    if( rh->get_name() == "resp2" )
	    {
		cerr << "remove successful, but found resp2" << endl ;
		delete rh ;
		return 1 ;
	    }
	    else
	    {
		cerr << "remove successful, but found not resp2 but "
		     << rh->get_name() << endl ;
		delete rh ;
		return 1 ;
	    }
	}
	else
	{
	    cout << "successfully removed resp2" << endl ;
	}
    }
    else
    {
	cerr << "failed to remove resp2" << endl ;
	return 1 ;
    }

    if( rhl->add_handler( "resp2", TestResponseHandler::TestResponseBuilder ) == true )
    {
	cout << "successfully added resp2 back" << endl ;
    }
    else
    {
	cerr << "failed to add resp2 back" << endl ;
	return 1 ;
    }

    DODSResponseHandler *rh = rhl->find_handler( "resp2" ) ;
    if( rh )
    {
	if( rh->get_name() == "resp2" )
	{
	    cout << "found resp2" << endl ;
	    delete rh ;
	}
	else
	{
	    cerr << "looking for resp2, found " << rh->get_name() << endl ;
	    delete rh ;
	    return 1 ;
	}
    }
    else
    {
	cerr << "coundn't find resp2" << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Returning from resplistT::run" << endl;

    return retVal;
}

int
main(int argC, char **argV) {
    Application *app = new resplistT();
    return app->main(argC, argV);
}

