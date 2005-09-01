// pvolT.cc

#include <iostream>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "pvolT.h"
#include "DODSContainerPersistenceVolatile.h"
#include "DODSContainer.h"
#include "TheDODSKeys.h"
#include "DODSException.h"
#include "DODSTextInfo.h"

int pvolT::
run(void)
{
    cout << endl << "*****************************************" << endl;
    cout << "Entered pvolT::run" << endl;
    int retVal = 0;

    cout << endl << "*****************************************" << endl;
    cout << "Create volatile and add five elements" << endl;
    DODSContainerPersistenceVolatile cpv( "volatile" ) ;
    cpv.add_container( "sym1", "real1", "type1" ) ;
    cpv.add_container( "sym2", "real2", "type2" ) ;
    cpv.add_container( "sym3", "real3", "type3" ) ;
    cpv.add_container( "sym4", "real4", "type4" ) ;
    cpv.add_container( "sym5", "real5", "type5" ) ;

    cout << endl << "*****************************************" << endl;
    cout << "show containers" << endl;
    DODSTextInfo info( false ) ;
    cpv.show_containers( info ) ;
    info.print( stdout ) ;

    cout << endl << "*****************************************" << endl;
    cout << "try to add sym1 again" << endl;
    try
    {
	cpv.add_container( "sym1", "real1", "type1" ) ;
	cerr << "succesfully added sym1 again, bad things man" << endl ;
	return 1 ;
    }
    catch( DODSException &e )
    {
	cout << "unable to add sym1 again, good" << endl ;
	cout << e.get_error_description() << endl ;
    }

    {
	char s[10] ;
	char r[10] ;
	char c[10] ;
	for( int i = 1; i < 6; i++ )
	{
	    sprintf( s, "sym%d", i ) ;
	    sprintf( r, "real%d", i ) ;
	    sprintf( c, "type%d", i ) ;
	    cout << endl << "*****************************************" << endl;
	    cout << "Looking for " << s << endl;
	    DODSContainer d( s ) ;
	    cpv.look_for( d ) ;
	    if( d.is_valid() )
	    {
		if( d.get_real_name() == r && d.get_container_type() == c )
		{
		    cout << "found " << s << endl ;
		}
		else
		{
		    cerr << "found " << s << " but real = " << r
			 << " and container = " << c << endl ;
		    return 1 ;
		}
	    }
	    else
	    {
		cerr << "couldn't find " << s << endl ;
		return 1 ;
	    }
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "remove sym1" << endl;
    DODSContainer *rem = cpv.rem_container( "sym1" ) ;
    if( rem )
    {
	if( rem->get_symbolic_name() != "sym1" )
	{
	    cerr << "removed, but sym " << rem->get_symbolic_name() << endl ;
	    return 1 ;
	}
	if( rem->get_real_name() != "real1" )
	{
	    cerr << "removed, but real " << rem->get_real_name() << endl ;
	    return 1 ;
	}
	if( rem->get_container_type() != "type1" )
	{
	    cerr << "removed, but real " << rem->get_container_type() << endl ;
	    return 1 ;
	}
	cout << "successfully removed sym1" << endl ;
	delete rem ; rem = 0 ;
    }

    {
	cout << endl << "*****************************************" << endl;
	cout << "find sym1" << endl;
	DODSContainer d( "sym1" ) ;
	cpv.look_for( d ) ;
	if( d.is_valid() == true )
	{
	    cerr << "found " << d.get_symbolic_name() << " with "
	         << "real = " << d.get_real_name() << " and "
		 << "type = " << d.get_container_type() << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "didn't remove it, good" << endl ;
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "remove sym5" << endl;
    rem = cpv.rem_container( "sym5" ) ;
    if( rem )
    {
	if( rem->get_symbolic_name() != "sym5" )
	{
	    cerr << "removed, but sym " << rem->get_symbolic_name() << endl ;
	    return 1 ;
	}
	if( rem->get_real_name() != "real5" )
	{
	    cerr << "removed, but real " << rem->get_real_name() << endl ;
	    return 1 ;
	}
	if( rem->get_container_type() != "type5" )
	{
	    cerr << "removed, but real " << rem->get_container_type() << endl ;
	    return 1 ;
	}
	cout << "successfully removed sym5" << endl ;
	delete rem ; rem = 0 ;
    }

    {
	cout << endl << "*****************************************" << endl;
	cout << "find sym5" << endl;
	DODSContainer d( "sym5" ) ;
	cpv.look_for( d ) ;
	if( d.is_valid() == true )
	{
	    cerr << "found " << d.get_symbolic_name() << " with "
	         << "real = " << d.get_real_name() << " and "
		 << "type = " << d.get_container_type() << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "didn't remove it, good" << endl ;
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "remove nosym" << endl;
    rem = cpv.rem_container( "nosym" ) ;
    if( !rem )
    {
	cout << "didn't find nosym, good" << endl ;
    }
    else
    {
	cerr << "removed a container with name "
	     << rem->get_symbolic_name() << ", bad things man" << endl ;
	delete rem ; rem = 0 ;
	return 1 ;
    }

    {
	char s[10] ;
	char r[10] ;
	char c[10] ;
	for( int i = 2; i < 5; i++ )
	{
	    sprintf( s, "sym%d", i ) ;
	    sprintf( r, "real%d", i ) ;
	    sprintf( c, "type%d", i ) ;
	    cout << endl << "*****************************************" << endl;
	    cout << "Looking for " << s << endl;
	    DODSContainer d( s ) ;
	    cpv.look_for( d ) ;
	    if( d.is_valid() )
	    {
		if( d.get_real_name() == r && d.get_container_type() == c )
		{
		    cout << "found " << s << endl ;
		}
		else
		{
		    cerr << "found " << s << " but real = " << r
			 << " and container = " << c << endl ;
		    return 1 ;
		}
	    }
	    else
	    {
		cerr << "couldn't find " << s << endl ;
		return 1 ;
	    }
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "show containers" << endl;
    DODSTextInfo info2( false ) ;
    cpv.show_containers( info2 ) ;
    info2.print( stdout ) ;

    cout << endl << "*****************************************" << endl;
    cout << "Returning from pvolT::run" << endl;
    return retVal;
}

int
main(int argC, char **argV) {
    putenv( "DODS_INI=./persistence_cgi_test.ini" ) ;
    Application *app = new pvolT();
    return app->main(argC, argV);
}

