// agglistT.C

#include <iostream>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "agglistT.h"
#include "BESAggFactory.h"
#include "BESTextInfo.h"
#include "BESError.h"
#include "TestAggServer.h"

int agglistT::
run(void)
{
    cout << endl << "*****************************************" << endl;
    cout << "Entered agglistT::run" << endl;
    int retVal = 0;

    cout << endl << "*****************************************" << endl;
    cout << "Adding three handlers to the list" << endl ;
    try
    {
	BESAggFactory::TheFactory()->add_handler( "h1", agglistT::h1 ) ;
	BESAggFactory::TheFactory()->add_handler( "h2", agglistT::h2 ) ;
	BESAggFactory::TheFactory()->add_handler( "h3", agglistT::h3 ) ;
	cout << "Successfully added three handlers" << endl ;
    }
    catch( BESError &e )
    {
	cerr << "Failed to add aggregation servers to list" << endl ;
	cerr << e.get_message() << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Find the three handlers" << endl ;
    try
    {
	BESAggregationServer *s = 0 ;
	s = BESAggFactory::TheFactory()->find_handler( "h1" ) ;
	if( !s )
	{
	    cerr << "Failed to find handler h1" << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "Successfully found handler h1" << endl ;
	}
	s = BESAggFactory::TheFactory()->find_handler( "h2" ) ;
	if( !s )
	{
	    cerr << "Failed to find handler h2" << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "Successfully found handler h2" << endl ;
	}
	s = BESAggFactory::TheFactory()->find_handler( "h3" ) ;
	if( !s )
	{
	    cerr << "Failed to find handler h3" << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "Successfully found handler h3" << endl ;
	}
    }
    catch( BESError &e )
    {
	cerr << "Failed to find aggregation servers" << endl ;
	cerr << e.get_message() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Remove handler h2" << endl ;
    try
    {
	bool removed = BESAggFactory::TheFactory()->remove_handler( "h2" ) ;
	if( removed )
	{
	    cout << "Successfully removed handler h2" << endl ;
	}
	else
	{
	    cerr << "Failed to remove handler h2" << endl ;
	    return 1 ;
	}
    }
    catch( BESError &e )
    {
	cerr << "Failed to remove aggregation server h2" << endl ;
	cerr << e.get_message() << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Find the two handlers" << endl ;
    try
    {
	BESAggregationServer *s = 0 ;
	s = BESAggFactory::TheFactory()->find_handler( "h1" ) ;
	if( !s )
	{
	    cerr << "Failed to find handler h1" << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "Successfully found handler h1" << endl ;
	}
	s = BESAggFactory::TheFactory()->find_handler( "h2" ) ;
	if( !s )
	{
	    cout << "Failed to find handler h2, good" << endl ;
	}
	else
	{
	    cout << "Successfully found handler h2, should not have" << endl ;
	    return 1 ;
	}
	s = BESAggFactory::TheFactory()->find_handler( "h3" ) ;
	if( !s )
	{
	    cerr << "Failed to find handler h3" << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "Successfully found handler h3" << endl ;
	}
    }
    catch( BESError &e )
    {
	cerr << "Failed to find aggregation servers" << endl ;
	cerr << e.get_message() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Show handler names registered" << endl;
    cout << BESAggFactory::TheFactory()->get_handler_names() << endl ;

    cout << endl << "*****************************************" << endl;
    cout << "Returning from agglistT::run" << endl;

    return retVal;
}

BESAggregationServer *
agglistT::h1( string name )
{
    return new TestAggServer( name ) ;
}

BESAggregationServer *
agglistT::h2( string name )
{
    return new TestAggServer( name ) ;
}

BESAggregationServer *
agglistT::h3( string name )
{
    return new TestAggServer( name ) ;
}

int
main(int argC, char **argV) {
    Application *app = new agglistT();
    putenv( "BES_CONF=./persistence_file_test.ini" ) ;
    return app->main(argC, argV);
}

