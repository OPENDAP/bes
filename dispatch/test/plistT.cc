// plistT.C

#include <iostream>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "plistT.h"
#include "ContainerStorageList.h"
#include "ContainerStorageFile.h"
#include "DODSContainer.h"
#include "DODSException.h"
#include "DODSTextInfo.h"

int plistT::
run(void)
{
    cout << endl << "*****************************************" << endl;
    cout << "Entered plistT::run" << endl;
    int retVal = 0;

    cout << endl << "*****************************************" << endl;
    cout << "Create the DODSContainerPersistentList" << endl;
    ContainerStorageList *cpl = ContainerStorageList::TheList() ;

    cout << endl << "*****************************************" << endl;
    cout << "Add ContainerStorageFile for File1 and File2" << endl;
    ContainerStorageFile *cpf ;
    cpf = new ContainerStorageFile( "File1" ) ;
    if( cpl->add_persistence( cpf ) == true )
    {
	cout << "successfully added File1" << endl ;
    }
    else
    {
	cerr << "unable to add File1" << endl ;
	return 1 ;
    }

    cpf = new ContainerStorageFile( "File2" ) ;
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
    cpf = new ContainerStorageFile( "File2" ) ;
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
	    DODSContainer d( s ) ;
	    cpl->look_for( d ) ;
	    if( d.is_valid() )
	    {
		if( d.get_real_name() == r && d.get_container_type() == c )
		{
		    cout << "found " << s << endl ;
		}
		else
		{
		    cerr << "found " << s << " but real = " << d.get_real_name()
			 << " and container = " << d.get_container_type() << endl;
		    return 1 ;
		}
	    }
	    else
	    {
		cerr << "couldn't find " << s << endl ;
		return 1 ;
	    }
	}
	catch( DODSException &e )
	{
	    cerr << "couldn't find " << s << endl ;
	    return 1 ;
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "looking for non-existant thingy" << endl;
    try
    {
	DODSContainer dnot( "thingy" ) ;
	cpl->look_for( dnot ) ;
	if( dnot.is_valid() )
	{
	    cerr << "found thingy, shouldn't have" << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "didn't find thingy, good" << endl ;
	}
    }
    catch( DODSException &e )
    {
	cout << "didn't find thingy, good" << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "show containers" << endl;
    DODSTextInfo info( false ) ;
    cpl->show_containers( info ) ;
    info.print( stdout ) ;

    cout << endl << "*****************************************" << endl;
    cout << "remove File1" << endl;
    if( cpl->rem_persistence( "File1" ) == true )
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
	DODSContainer d2( "sym2" ) ;
	cpl->look_for( d2 ) ;
	if( d2.is_valid() )
	{
	    cerr << "found sym2 with real = " << d2.get_real_name()
		 << " and container = " << d2.get_container_type() << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "couldn't find sym2, good" << endl ;
	}
    }
    catch( DODSException &e )
    {
	cout << "couldn't find sym2, good" << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "looking for sym7" << endl;
    try
    {
	DODSContainer d7( "sym7" ) ;
	cpl->look_for( d7 ) ;
	if( d7.is_valid() )
	{
	    if( d7.get_real_name() == "real7" &&
		d7.get_container_type() == "type7" )
	    {
		cout << "found sym7" << endl ;
	    }
	    else
	    {
		cerr << "found sym7 but real = " << d7.get_real_name()
		     << " and container = " << d7.get_container_type() << endl ;
		return 1 ;
	    }
	}
	else
	{
	    cerr << "couldn't find sym7, should have" << endl ;
	    return 1 ;
	}
    }
    catch( DODSException &e )
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
    putenv( "OPENDAP_INI=./persistence_file_test.ini" ) ;
    return app->main(argC, argV);
}

