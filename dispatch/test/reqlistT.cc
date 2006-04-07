// reqlistT.C

#include <iostream>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "reqlistT.h"
#include "DODSRequestHandlerList.h"
#include "TestRequestHandler.h"

int reqlistT::
run(void) {
    cout << endl << "*****************************************" << endl;
    cout << "Entered reqlistT::run" << endl;
    int retVal = 0;

    cout << endl << "*****************************************" << endl;
    cout << "add the 5 request handlers" << endl ;
    DODSRequestHandlerList *rhl = DODSRequestHandlerList::TheList() ;
    char num[10] ;
    for( int i = 0; i < 5; i++ )
    {
	sprintf( num, "req%d", i ) ;
	if( rhl->add_handler( num, new TestRequestHandler( num ) ) == true )
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
    cout << "try to add req3 again" << endl ;
    DODSRequestHandler *rh = new TestRequestHandler( "req3" ) ;
    if( rhl->add_handler( "req3", rh ) == true )
    {
	cerr << "successfully added req3 again" << endl ;
	return 1 ;
    }
    else
    {
	cout << "failed to add req3 again, good" << endl ;
	delete rh ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "finding the handlers" << endl ;
    for( int i = 4; i >=0; i-- )
    {
	sprintf( num, "req%d", i ) ;
	rh = rhl->find_handler( num ) ;
	if( rh )
	{
	    if( rh->get_name() == num )
	    {
		cout << "found " << num << endl ;
	    }
	    else
	    {
		cerr << "looking for " << num
		     << ", found " << rh->get_name() << endl ;
		return 1 ;
	    }
	}
	else
	{
	    cerr << "coundn't find " << num << endl ;
	    return 1 ;
	}
    }
    rh = rhl->find_handler( "thingy" ) ;
    if( rh )
    {
	if( rh->get_name() == "thingy" )
	{
	    cerr << "found thingy" << endl ;
	    return 1 ;
	}
	else
	{
	    cerr << "looking for thingy, found " << rh->get_name() << endl ;
	    return 1 ;
	}
    }
    else
    {
	cout << "coundn't find thingy" << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "removing req2" << endl ;
    rh = rhl->remove_handler( "req2" ) ;
    if( rh )
    {
	string name = rh->get_name() ;
	if( name == "req2" )
	{
	    cout << "successfully removed req2" << endl ;
	    delete rh ;
	}
	else
	{
	    cerr << "trying to remove req2, but removed " << name << endl ;
	    return 1 ;
	}
    }
    else
    {
	cerr << "failed to remove req2" << endl ;
	return 1 ;
    }

    rh = rhl->find_handler( "req2" ) ;
    if( rh )
    {
	if( rh->get_name() == "req2" )
	{
	    cerr << "found req2, should have been removed" << endl ;
	    return 1 ;
	}
	else
	{
	    cerr << "found " << rh->get_name() << " when looking for req2"
		 << endl ;
	    return 1 ;
	}
    }
    else
    {
	cout << "couldn't find req2, good" << endl ;
    }

    if( rhl->add_handler( "req2", new TestRequestHandler( "req2" ) ) == true )
    {
	cout << "successfully added req2 back" << endl ;
    }
    else
    {
	cerr << "failed to add req2 back" << endl ;
	return 1 ;
    }

    rh = rhl->find_handler( "req2" ) ;
    if( rh )
    {
	if( rh->get_name() == "req2" )
	{
	    cout << "found req2" << endl ;
	}
	else
	{
	    cerr << "looking for req2, found " << rh->get_name() << endl ;
	    return 1 ;
	}
    }
    else
    {
	cerr << "coundn't find req2" << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Iterating through handler list" << endl ;
    DODSRequestHandlerList::Handler_citer h = rhl->get_first_handler() ;
    DODSRequestHandlerList::Handler_citer hl = rhl->get_last_handler() ;
    int count = 0 ;
    for( ; h != hl; h++ )
    {
	rh = (*h).second ;
	char sb[10] ;
	sprintf( sb, "req%d", count ) ;
	string n = rh->get_name() ;
	if( n == sb )
	{
	    cout << "found " << n << endl ;
	}
	else
	{
	    cerr << "found " << n << ", looking for " << sb << endl ;
	    return 1 ;
	}
	count++ ;
    }
    if( count == 5 )
    {
	cout << "found right number of handlers" << endl ;
    }
    else
    {
	cerr << "wrong number of handlers, found " << count << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Returning from reqlistT::run" << endl;

    return retVal;
}

int
main(int argC, char **argV) {
    Application *app = new reqlistT();
    return app->main(argC, argV);
}

