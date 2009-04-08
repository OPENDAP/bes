// plistT.C

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
#include <cstdlib>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "plistT.h"
#include "BESContainerStorageList.h"
#include "BESContainerStorageFile.h"
#include "BESContainer.h"
#include "BESError.h"
#include "BESTextInfo.h"
#include "TheBESKeys.h"
#include <test_config.h>

int plistT::
run(void)
{
    BESKeys *keys = TheBESKeys::TheKeys() ;
    keys->set_key( (string)"BES.Container.Persistence.File.FileTooMany=" + TEST_SRC_DIR + "/persistence_file3.txt" ) ;
    keys->set_key( (string)"BES.Container.Persistence.File.FileTooFew=" + TEST_SRC_DIR + "/persistence_file4.txt" ) ;
    keys->set_key( (string)"BES.Container.Persistence.File.File1=" + TEST_SRC_DIR + "/persistence_file1.txt" ) ;
    keys->set_key( (string)"BES.Container.Persistence.File.File2=" + TEST_SRC_DIR + "/persistence_file2.txt" ) ;

    cout << endl << "*****************************************" << endl;
    cout << "Entered plistT::run" << endl;
    int retVal = 0;

    cout << endl << "*****************************************" << endl;
    cout << "Create the BESContainerPersistentList" << endl;
    BESContainerStorageList *cpl = BESContainerStorageList::TheList() ;

    cout << endl << "*****************************************" << endl;
    cout << "Add BESContainerStorageFile for File1 and File2" << endl;
    BESContainerStorageFile *cpf ;
    cpf = new BESContainerStorageFile( "File1" ) ;
    if( cpl->add_persistence( cpf ) == true )
    {
	cout << "successfully added File1" << endl ;
    }
    else
    {
	cerr << "unable to add File1" << endl ;
	return 1 ;
    }

    cpf = new BESContainerStorageFile( "File2" ) ;
    if( cpl->add_persistence( cpf ) == true )
    {
	cout << "successfully added File2" << endl ;
    }
    else
    {
	cerr << "unable to add File2" << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Try to add File2 again" << endl;
    cpf = new BESContainerStorageFile( "File2" ) ;
    if( cpl->add_persistence( cpf ) == true )
    {
	cerr << "successfully added File2 again" << endl ;
	delete cpf ;
	return 1 ;
    }
    else
    {
	cout << "unable to add File2, good" << endl ;
	delete cpf ;
    }

    char s[10] ;
    char r[10] ;
    char c[10] ;
    for( int i = 1; i < 11; i++ )
    {
	sprintf( s, "sym%d", i ) ;
	sprintf( r, "real%d", i ) ;
	sprintf( c, "type%d", i ) ;
	cout << endl << "*****************************************" << endl;
	cout << "looking for " << s << endl;
	try
	{
	    BESContainer *d = cpl->look_for( s ) ;
	    if( d )
	    {
		if( d->get_real_name() == r && d->get_container_type() == c )
		{
		    cout << "found " << s << endl ;
		}
		else
		{
		    cerr << "found " << s << " but real = "
		         << d->get_real_name()
			 << " and container = "
			 << d->get_container_type() << endl;
		    return 1 ;
		}
	    }
	    else
	    {
		cerr << "couldn't find " << s << endl ;
		return 1 ;
	    }
	}
	catch( BESError &e )
	{
	    cerr << "couldn't find " << s << endl ;
	    return 1 ;
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "looking for non-existant thingy" << endl;
    try
    {
	BESContainer *dnot = cpl->look_for( "thingy" ) ;
	if( dnot )
	{
	    cerr << "found thingy, shouldn't have" << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "didn't find thingy, good" << endl ;
	}
    }
    catch( BESError &e )
    {
	cout << "didn't find thingy, good" << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "show containers" << endl;
    BESTextInfo info ;
    cpl->show_containers( info ) ;
    info.print( cout ) ;

    cout << endl << "*****************************************" << endl;
    cout << "remove File1" << endl;
    if( cpl->deref_persistence( "File1" ) == true )
    {
	cout << "successfully removed File1" << endl ;
    }
    else
    {
	cerr << "unable to remove File1" << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "looking for sym2" << endl;
    try
    {
	BESContainer *d2 = cpl->look_for( "sym2" ) ;
	if( d2 )
	{
	    cerr << "found sym2 with real = " << d2->get_real_name()
		 << " and container = " << d2->get_container_type() << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "couldn't find sym2, good" << endl ;
	}
    }
    catch( BESError &e )
    {
	cout << "couldn't find sym2, good" << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "looking for sym7" << endl;
    try
    {
	BESContainer *d7 = cpl->look_for( "sym7" ) ;
	if( d7 )
	{
	    if( d7->get_real_name() == "real7" &&
		d7->get_container_type() == "type7" )
	    {
		cout << "found sym7" << endl ;
	    }
	    else
	    {
		cerr << "found sym7 but real = " << d7->get_real_name()
		     << " and container = " << d7->get_container_type()
		     << endl ;
		return 1 ;
	    }
	}
	else
	{
	    cerr << "couldn't find sym7, should have" << endl ;
	    return 1 ;
	}
    }
    catch( BESError &e )
    {
	cerr << "couldn't find sym7, should have" << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Returning from plistT::run" << endl;

    return retVal;
}

int
main(int argC, char **argV) {
    Application *app = new plistT();
    string env_var = (string)"BES_CONF=" + TEST_SRC_DIR
                     + "/persistence_file_test.ini" ;
    putenv( (char *)env_var.c_str() ) ;
    return app->main(argC, argV);
}

