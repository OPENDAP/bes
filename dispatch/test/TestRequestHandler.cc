// TestRequestHandler.cc

#include <iostream>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "TestRequestHandler.h"

TestRequestHandler *trh = 0 ;

TestRequestHandler::TestRequestHandler( string name )
    : BESRequestHandler( name ),
      _resp_num( 0 )
{
    trh = this ;
    add_handler( "resp1", TestRequestHandler::test_build_resp1 ) ;
    add_handler( "resp2", TestRequestHandler::test_build_resp2 ) ;
    add_handler( "resp3", TestRequestHandler::test_build_resp3 ) ;
    add_handler( "resp4", TestRequestHandler::test_build_resp4 ) ;
}

TestRequestHandler::~TestRequestHandler()
{
}

bool
TestRequestHandler::test_build_resp1( BESDataHandlerInterface &r )
{
    trh->_resp_num = 1 ;
    return true ;
}

bool
TestRequestHandler::test_build_resp2( BESDataHandlerInterface &r )
{
    trh->_resp_num = 2 ;
    return true ;
}

bool
TestRequestHandler::test_build_resp3( BESDataHandlerInterface &r )
{
    trh->_resp_num = 3 ;
    return true ;
}

bool
TestRequestHandler::test_build_resp4( BESDataHandlerInterface &r )
{
    trh->_resp_num = 4 ;
    return true ;
}

int
TestRequestHandler::test()
{
    cout << endl << "*****************************************" << endl;
    cout << "finding the handlers" << endl ;
    BESDataHandlerInterface r ;
    p_request_handler p = find_handler( "resp1" ) ;
    if( p )
    {
	p( r ) ;
	if( _resp_num == 1 )
	{
	    cout << "found resp1" << endl ;
	}
	else
	{
	    cerr << "looking for resp1, found " << _resp_num << endl ;
	    return 1 ;
	}
    }
    else
    {
	cerr << "coundn't find resp1" << endl ;
	return 1 ;
    }

    p = find_handler( "resp2" ) ;
    if( p )
    {
	p( r ) ;
	if( _resp_num == 2 )
	{
	    cout << "found resp2" << endl ;
	}
	else
	{
	    cerr << "looking for resp2, found " << _resp_num << endl ;
	    return 1 ;
	}
    }
    else
    {
	cerr << "coundn't find resp2" << endl ;
	return 1 ;
    }

    p = find_handler( "resp3" ) ;
    if( p )
    {
	p( r ) ;
	if( _resp_num == 3 )
	{
	    cout << "found resp3" << endl ;
	}
	else
	{
	    cerr << "looking for resp3, found " << _resp_num << endl ;
	    return 1 ;
	}
    }
    else
    {
	cerr << "coundn't find resp3" << endl ;
	return 1 ;
    }

    p = find_handler( "resp4" ) ;
    if( p )
    {
	p( r ) ;
	if( _resp_num == 4 )
	{
	    cout << "found resp4" << endl ;
	}
	else
	{
	    cerr << "looking for resp4, found " << _resp_num << endl ;
	    return 1 ;
	}
    }
    else
    {
	cerr << "coundn't find resp4" << endl ;
	return 1 ;
    }

    p = find_handler( "thingy" ) ;
    if( p )
    {
	p( r ) ;
	cerr << "found the response handler " << _resp_num << endl ;
	return 1 ;
    }
    else
    {
	cout << "didn't find thingy, good" << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "try to add resp3 again" << endl ;
    bool ret = add_handler( "resp3", TestRequestHandler::test_build_resp3 ) ;
    if( ret == true )
    {
	cerr << "successfully added resp3 again" << endl ;
	return 1 ;
    }
    else
    {
	cout << "failed to add resp3 again, good" << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "removing resp2" << endl ;
    ret = remove_handler( "resp2" ) ;
    if( ret == true )
    {
	p = find_handler( "resp2" ) ;
	if( p )
	{
	    p( r ) ;
	    cerr << "found resp2, execution = " << _resp_num << endl ;
	    return 1 ;
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

    if( add_handler( "resp2", TestRequestHandler::test_build_resp2 ) == true )
    {
	cout << "successfully added resp2 back" << endl ;
    }
    else
    {
	cerr << "failed to add resp2 back" << endl ;
	return 1 ;
    }

    p = find_handler( "resp2" ) ;
    if( p )
    {
	p( r ) ;
	if( _resp_num == 2 )
	{
	    cout << "found resp2" << endl ;
	}
	else
	{
	    cerr << "looking for resp2, found " << _resp_num << endl ;
	    return 1 ;
	}
    }
    else
    {
	cerr << "coundn't find resp2" << endl ;
	return 1 ;
    }

    return 0 ;
}

