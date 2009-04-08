// pvolT.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <iostream>
#include <fstream>
#include <cstdlib>

using std::cerr ;
using std::cout ;
using std::endl ;
using std::ofstream ;
using std::ios ;

#include "pvolT.h"
#include "BESContainerStorageVolatile.h"
#include "BESContainer.h"
#include "TheBESKeys.h"
#include "BESError.h"
#include "BESTextInfo.h"
#include <test_config.h>

int pvolT::
initialize(int argC, char **argV)
{
    int retval = baseApp::initialize( argC, argV ) ;
    if( retval )
	return retval ;

    ofstream real1( "./real1", ios::trunc ) ;
    real1 << "real1" << endl ;
    real1.close() ;
    ofstream real2( "./real2", ios::trunc ) ;
    real2 << "real2" << endl ;
    real2.close() ;
    ofstream real3( "./real3", ios::trunc ) ;
    real3 << "real3" << endl ;
    real3.close() ;
    ofstream real4( "./real4", ios::trunc ) ;
    real4 << "real4" << endl ;
    real4.close() ;
    ofstream real5( "./real5", ios::trunc ) ;
    real5 << "real5" << endl ;
    real5.close() ;

    return 0 ;
}

int pvolT::
run(void)
{
    cout << endl << "*****************************************" << endl;
    cout << "Entered pvolT::run" << endl;
    int retVal = 0;

    BESContainerStorageVolatile cpv( "volatile" ) ;

    cout << endl << "*****************************************" << endl;
    cout << "Create volatile and add five elements" << endl;
    try
    {
	cpv.add_container( "sym1", "real1", "type1" ) ;
	cpv.add_container( "sym2", "real2", "type2" ) ;
	cpv.add_container( "sym3", "real3", "type3" ) ;
	cpv.add_container( "sym4", "real4", "type4" ) ;
	cpv.add_container( "sym5", "real5", "type5" ) ;
    }
    catch( BESError &e )
    {
	cerr << "failed to add elements" << endl << e.get_message() << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "show containers" << endl;
    BESTextInfo info ;
    cpv.show_containers( info ) ;
    info.print( cout ) ;

    cout << endl << "*****************************************" << endl;
    cout << "try to add sym1 again" << endl;
    try
    {
	cpv.add_container( "sym1", "real1", "type1" ) ;
	cerr << "succesfully added sym1 again, bad things man" << endl ;
	return 1 ;
    }
    catch( BESError &e )
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
    info2.print( cout ) ;

    cout << endl << "*****************************************" << endl;
    cout << "Returning from pvolT::run" << endl;
    return retVal;
}

int pvolT::
terminate(int sig)
{
    remove( "./real1" ) ;
    remove( "./real2" ) ;
    remove( "./real3" ) ;
    remove( "./real4" ) ;
    remove( "./real5" ) ;

    return sig ;
}

int
main(int argC, char **argV) {
    string env_var = (string)"BES_CONF=" + TEST_SRC_DIR
                     + "/persistence_cgi_test.ini" ;
    putenv( (char *)env_var.c_str() ) ;
    Application *app = new pvolT();
    return app->main(argC, argV);
}

