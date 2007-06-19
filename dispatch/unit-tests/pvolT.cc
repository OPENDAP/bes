// pvolT.cc

#include <iostream>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "pvolT.h"
#include "BESContainerStorageVolatile.h"
#include "BESContainer.h"
#include "TheBESKeys.h"
#include "BESException.h"
#include "BESTextInfo.h"
#include <test_config.h>

int pvolT::
run(void)
{
    cout << endl << "*****************************************" << endl;
    cout << "Entered pvolT::run" << endl;
    int retVal = 0;

    cout << endl << "*****************************************" << endl;
    cout << "Create volatile and add five elements" << endl;
    BESContainerStorageVolatile cpv( "volatile" ) ;
    cpv.add_container( "sym1", "real1", "type1" ) ;
    cpv.add_container( "sym2", "real2", "type2" ) ;
    cpv.add_container( "sym3", "real3", "type3" ) ;
    cpv.add_container( "sym4", "real4", "type4" ) ;
    cpv.add_container( "sym5", "real5", "type5" ) ;

    cout << endl << "*****************************************" << endl;
    cout << "show containers" << endl;
    BESTextInfo info ;
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
    catch( BESException &e )
    {
	cout << "unable to add sym1 again, good" << endl ;
	cout << e.get_message() << endl ;
    }

    {
	char s[10] ;
	char r[10] ;
	char c[10] ;
	for( int i = 1; i < 6; i++ )
	{
	    sprintf( s, "sym%d", i ) ;
	    sprintf( r, "./real%d", i ) ;
	    sprintf( c, "type%d", i ) ;
	    cout << endl << "*****************************************" << endl;
	    cout << "Looking for " << s << endl;
	    BESContainer *d = cpv.look_for( s ) ;
	    if( d )
	    {
		if( d->get_real_name() == r && d->get_container_type() == c )
		{
		    cout << "found " << s << endl ;
		}
		else
		{
		    cerr << "found " << s << " but:" << endl ;
		    cerr << "real = " << r << ", should be " << d->get_real_name() << endl ;
		    cerr << "type = " << c << ", should be " << d->get_container_type() << endl ;
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
    bool rem = cpv.del_container( "sym1" ) ;
    if( rem )
    {
	cout << "successfully removed sym1" << endl ;
    }

    {
	cout << endl << "*****************************************" << endl;
	cout << "find sym1" << endl;
	BESContainer *d = cpv.look_for( "sym1" ) ;
	if( d )
	{
	    cerr << "found " << d->get_symbolic_name() << " with "
	         << "real = " << d->get_real_name() << " and "
		 << "type = " << d->get_container_type() << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "didn't remove it, good" << endl ;
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "remove sym5" << endl;
    rem = cpv.del_container( "sym5" ) ;
    if( rem )
    {
	cout << "successfully removed sym5" << endl ;
    }

    {
	cout << endl << "*****************************************" << endl;
	cout << "find sym5" << endl;
	BESContainer *d = cpv.look_for( "sym5" ) ;
	if( d )
	{
	    cerr << "found " << d->get_symbolic_name() << " with "
	         << "real = " << d->get_real_name() << " and "
		 << "type = " << d->get_container_type() << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "didn't remove it, good" << endl ;
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "remove nosym" << endl;
    rem = cpv.del_container( "nosym" ) ;
    if( !rem )
    {
	cout << "didn't find nosym, good" << endl ;
    }
    else
    {
	cerr << "removed a container, bad things man" << endl ;
	return 1 ;
    }

    {
	char s[10] ;
	char r[10] ;
	char c[10] ;
	for( int i = 2; i < 5; i++ )
	{
	    sprintf( s, "sym%d", i ) ;
	    sprintf( r, "./real%d", i ) ;
	    sprintf( c, "type%d", i ) ;
	    cout << endl << "*****************************************" << endl;
	    cout << "Looking for " << s << endl;
	    BESContainer *d = cpv.look_for( s ) ;
	    if( d )
	    {
		if( d->get_real_name() == r && d->get_container_type() == c )
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
    BESTextInfo info2 ;
    cpv.show_containers( info2 ) ;
    info2.print( stdout ) ;

    cout << endl << "*****************************************" << endl;
    cout << "Returning from pvolT::run" << endl;
    return retVal;
}

int
main(int argC, char **argV) {
    string env_var = (string)"BES_CONF=" + TEST_SRC_DIR
                     + "/persistence_cgi_test.ini" ;
    putenv( (char *)env_var.c_str() ) ;
    Application *app = new pvolT();
    return app->main(argC, argV);
}

