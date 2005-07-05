// pcgiT.cc

#include <iostream>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "pcgiT.h"
#include "DODSContainerPersistenceCGI.h"
#include "DODSContainer.h"
#include "TheDODSKeys.h"
#include "DODSException.h"
#include "DODSTextInfo.h"

int pcgiT::
run(void)
{
    cout << endl << "*****************************************" << endl;
    cout << "Entered pcgiT::run" << endl;
    int retVal = 0;

    cout << endl << "*****************************************" << endl;
    cout << "Try to create without base dir set" << endl;
    try
    {
	DODSContainerPersistenceCGI cpc( "CGI" ) ;
	cerr << "created persistence, shouldn't have" << endl ;
	return 1 ;
    }
    catch( DODSException &ex )
    {
	cout << "couldn't create persistence, good, because" << endl ;
	cout << ex.get_error_description() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "set base dir" << endl;
    TheDODSKeys->set_key( "DODS.Container.Persistence.CGI.CGI.BaseDirectory=/twilek/d/pwest/local/apache/htdocs/data" ) ;

    cout << endl << "*****************************************" << endl;
    cout << "Try to create without reg expr" << endl;
    try
    {
	DODSContainerPersistenceCGI cpc( "CGI" ) ;
	cerr << "created persistence, shouldn't have" << endl ;
	return 1 ;
    }
    catch( DODSException &ex )
    {
	cout << "couldn't create persistence, good, because" << endl ;
	cout << ex.get_error_description() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "set reg expr, no semicolon" << endl;
    TheDODSKeys->set_key( "DODS.Container.Persistence.CGI.CGI.TypeMatch=cedar:.*\\/cedar" ) ;

    cout << endl << "*****************************************" << endl;
    cout << "Try to create with malformed reg expression, no semi" << endl;
    try
    {
	DODSContainerPersistenceCGI cpc( "CGI" ) ;
	cerr << "created persistence, shouldn't have" << endl ;
	return 1 ;
    }
    catch( DODSException &ex )
    {
	cout << "couldn't create persistence, good, because" << endl ;
	cout << ex.get_error_description() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "set reg expr, no colon" << endl;
    TheDODSKeys->set_key( "DODS.Container.Persistence.CGI.CGI.TypeMatch=cedar+.*\\/cedar\\/.*\\.cbf;" ) ;

    cout << endl << "*****************************************" << endl;
    cout << "Try to create with malformed reg expression, no colon" << endl;
    try
    {
	DODSContainerPersistenceCGI cpc( "CGI" ) ;
	cerr << "created persistence, shouldn't have" << endl ;
	return 1 ;
    }
    catch( DODSException &ex )
    {
	cout << "couldn't create persistence, good, because" << endl ;
	cout << ex.get_error_description() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "set good reg expr" << endl;
    bool found = false ;
    string expr =
	TheDODSKeys->get_key( "DODS.Container.Persistence.CGI.CGI.Dummy", found ) ;
    string set_expr = "DODS.Container.Persistence.CGI.CGI.TypeMatch=" + expr ;
    TheDODSKeys->set_key( set_expr ) ;
    //TheDODSKeys->set_key( "DODS.Container.Persistence.CGI.CGI.TypeMatch=cedar+.*\\/cedar\\/.*\\.cbf;" ) ;

    cout << endl << "*****************************************" << endl;
    cout << "Try to create with good reg expression" << endl;
    try
    {
	DODSContainerPersistenceCGI cpc( "CGI" ) ;
	cout << "created persistence" << endl ;
    }
    catch( DODSException &ex )
    {
	cerr << "couldn't create persistence, because" << endl ;
	cerr << ex.get_error_description() << endl ;
	return 1 ;
    }

    DODSContainerPersistenceCGI cpc( "CGI" ) ;

    {
	cout << endl << "*****************************************" << endl;
	cout << "try to find notfound.ext" << endl;
	DODSContainer d( "notfound.ext" ) ;
	cpc.look_for( d ) ;
	if( d.is_valid() )
	{
	    cerr << "notfound.ext found with "
		 << "real_name = " << d.get_real_name() 
		 << " and container_type = " << d.get_container_type()
		 << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "notfound.ext not found" << endl ;
	}
    }

    {
	cout << endl << "*****************************************" << endl;
	cout << "try to find notcedar.cbf" << endl;
	DODSContainer d( "notcedar.cbf" ) ;
	cpc.look_for( d ) ;
	if( d.is_valid() )
	{
	    cerr << "notcedar.cbf found with "
		 << "real_name = " << d.get_real_name() 
		 << " and container_type = " << d.get_container_type()
		 << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "notcedar.cbf not found" << endl ;
	}
    }

    {
	cout << endl << "*****************************************" << endl;
	cout << "try to find cedar/notcedar.ext" << endl;
	DODSContainer d( "cedar/notcedar.ext" ) ;
	cpc.look_for( d ) ;
	if( d.is_valid() )
	{
	    cerr << "cedar/notcedar.ext found with "
		 << "real_name = " << d.get_real_name() 
		 << " and container_type = " << d.get_container_type()
		 << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "cedar/notcedar.ext not found" << endl ;
	}
    }

    string base = "/twilek/d/pwest/local/apache/htdocs/data" ;

    {
	cout << endl << "*****************************************" << endl;
	cout << "try to find cedar/iscedar.cbf" << endl;
	DODSContainer d( "cedar/iscedar.cbf" ) ;
	cpc.look_for( d ) ;
	if( d.is_valid() == true )
	{
	    string real_name = base + "/cedar/iscedar.cbf" ;
	    if( d.get_real_name() == real_name &&
		d.get_container_type() == "cedar" )
	    {
		cout << "cedar/iscedar.cbf found with "
		     << "real_name = " << d.get_real_name() 
		     << " and container_type = " << d.get_container_type()
		     << endl ;
	    }
	    else
	    {
		cerr << "cedar/iscedar.cbf found, but "
		     << "real_name = " << d.get_real_name() 
		     << " and container_type = " << d.get_container_type()
		     << endl ;
		return 1 ;
	    }
	}
	else
	{
	    cerr << "BAD. cedar/iscedar.cbf not found" << endl ;
	    return 1 ;
	}
    }

    {
	cout << endl << "*****************************************" << endl;
	cout << "try to find notcdf.cdf" << endl;
	DODSContainer d( "notcdf.cdf" ) ;
	cpc.look_for( d ) ;
	if( d.is_valid() )
	{
	    cerr << "notcdf.cdf found with "
		 << "real_name = " << d.get_real_name() 
		 << " and container_type = " << d.get_container_type()
		 << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "notcdf.cdf not found" << endl ;
	}
    }

    {
	cout << endl << "*****************************************" << endl;
	cout << "try to find cdf/notcdf.ext" << endl;
	DODSContainer d( "cdf/notcdf.ext" ) ;
	cpc.look_for( d ) ;
	if( d.is_valid() )
	{
	    cerr << "cdf/notcdf.ext found with "
		 << "real_name = " << d.get_real_name() 
		 << " and container_type = " << d.get_container_type()
		 << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "cdf/notcdf.ext not found" << endl ;
	}
    }

    {
	cout << endl << "*****************************************" << endl;
	cout << "try to find cdf/iscdf.cdf" << endl;
	DODSContainer d( "cdf/iscdf.cdf" ) ;
	cpc.look_for( d ) ;
	if( d.is_valid() )
	{
	    string real_name = base + "/cdf/iscdf.cdf" ;
	    if( d.get_real_name() == real_name &&
		d.get_container_type() == "cdf" )
	    {
		cout << "cdf/iscdf.cdf found with "
		     << "real_name = " << d.get_real_name() 
		     << " and container_type = " << d.get_container_type()
		     << endl ;
	    }
	    else
	    {
		cerr << "cdf/iscdf.cdf found, but "
		     << "real_name = " << d.get_real_name() 
		     << " and container_type = " << d.get_container_type()
		     << endl ;
		return 1 ;
	    }
	}
	else
	{
	    cerr << "BAD, cdf/iscdf.cdf not found" << endl ;
	    return 1 ;
	}
    }

    {
	cout << endl << "*****************************************" << endl;
	cout << "show containers" << endl;
	DODSTextInfo info( false ) ;
	cpc.show_containers( info ) ;
	info.print( stdout ) ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Returning from pcgiT::run" << endl;

    return retVal;
}

int
main(int argC, char **argV) {
    putenv( "DODS_INI=./persistence_cgi_test.ini" ) ;
    Application *app = new pcgiT();
    return app->main(argC, argV);
}

