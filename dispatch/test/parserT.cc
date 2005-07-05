// parserT.C

#include <iostream>
#include <sstream>

using std::cerr ;
using std::cout ;
using std::endl ;
using std::stringstream ;

#include "parserT.h"
#include "DODSParser.h"
#include "DODSDataHandlerInterface.h"
#include "DODSResponseHandler.h"
#include "ThePersistenceList.h"
#include "DODSContainer.h"
#include "DODSException.h"
#include "DODSDefine.h"
#include "TheDefineList.h"
#include "DODSContainer.h"
#include "DODSTextInfo.h"
#include "TheDODSKeys.h"

// what is to be tested
// show containers;
// show definitions;
// show version;
// show help;
// show process;
// set container in store values sym1,real1,type1;
// define defname as container_names with container.constraint="",container.attributes="" aggregate by "";
// get dods for defname return as "";

// what is needed. For the definition the containers need to exist, so need
// to actually set containers. For the get command need the definitions to
// exist, so need to actually create some definitions. This can all be done
// from here without running through the whole system. Just add some
// containers to volatile and make some definitions.

int parserT::
run(void) {
    cout << endl << "*****************************************" << endl;
    cout << "Entered parserT::run" << endl;
    int retVal = 0;

    cout << endl << "*****************************************" << endl;
    cout << "just \"set;\"" << endl;
    {
	try
	{
	    DODSParser p ;
	    DODSDataHandlerInterface dhi ;
	    p.parse( "set;", dhi ) ;
	    cerr << "should have failed" << endl ;
	    return 1 ;
	}
	catch( DODSException &e )
	{
	    cout << "caught exception, good" << endl
	         << e.get_error_description() ;
	}
    }
    cout << endl << "*****************************************" << endl;
    cout << "just \"set nothing;\"" << endl;
    {
	try
	{
	    DODSParser p ;
	    DODSDataHandlerInterface dhi ;
	    p.parse( "set nothing;", dhi ) ;
	    cerr << "should have failed" << endl ;
	    return 1 ;
	}
	catch( DODSException &e )
	{
	    cout << "caught exception, good" << endl
	         << e.get_error_description() ;
	}
    }
    cout << endl << "*****************************************" << endl;
    cout << "just \"set container;\"" << endl;
    {
	try
	{
	    DODSParser p ;
	    DODSDataHandlerInterface dhi ;
	    p.parse( "set container;", dhi ) ;
	    cerr << "should have failed" << endl ;
	    return 1 ;
	}
	catch( DODSException &e )
	{
	    cout << "caught exception, good" << endl
	         << e.get_error_description() ;
	}
    }
    cout << endl << "*****************************************" << endl;
    cout << "only sym \"set container values onlysym;\"" << endl;
    {
	try
	{
	    DODSParser p ;
	    DODSDataHandlerInterface dhi ;
	    p.parse( "set container values onlysym;", dhi ) ;
	    cerr << "should have failed" << endl ;
	    return 1 ;
	}
	catch( DODSException &e )
	{
	    cout << "caught exception, good" << endl
	         << e.get_error_description() ;
	}
    }
    cout << endl << "*****************************************" << endl;
    cout << "colon between sym and real \"set container values sym;real;\"" << endl;
    {
	try
	{
	    DODSParser p ;
	    DODSDataHandlerInterface dhi ;
	    p.parse( "set container values sym;real;", dhi ) ;
	    cerr << "should have failed" << endl ;
	    return 1 ;
	}
	catch( DODSException &e )
	{
	    cout << "caught exception, good" << endl
	         << e.get_error_description() ;
	}
    }
    cout << endl << "*****************************************" << endl;
    cout << "colon between real and type \"set container values sym,real;type;\"" << endl;
    {
	try
	{
	    DODSParser p ;
	    DODSDataHandlerInterface dhi ;
	    p.parse( "set container values sym,real;type;", dhi ) ;
	    cerr << "should have failed" << endl ;
	    return 1 ;
	}
	catch( DODSException &e )
	{
	    cout << "caught exception, good" << endl
	         << e.get_error_description() ;
	}
    }
    cout << endl << "*****************************************" << endl;
    cout << "no store name \"set container in values sym,real,type;\"" << endl;
    {
	try
	{
	    DODSParser p ;
	    DODSDataHandlerInterface dhi ;
	    p.parse( "set container in values sym,real,type;", dhi ) ;
	    cerr << "should have failed" << endl ;
	    return 1 ;
	}
	catch( DODSException &e )
	{
	    cout << "caught exception, good" << endl
	         << e.get_error_description() ;
	}
    }
    cout << endl << "*****************************************" << endl;
    cout << "syntax correct, create sym1" << endl;
    {
	try
	{
	    DODSParser p ;
	    DODSDataHandlerInterface dhi ;
	    p.parse( "set container values sym1,real1,type1;", dhi ) ;
	    DODSResponseHandler *rh = dhi.response_handler ;
	    if( rh )
	    {
		rh->execute( dhi ) ;
		delete rh ;
	    }
	    else
	    {
		cerr << "response handler not set, exiting" << endl ;
		return 1 ;
	    }
	}
	catch( DODSException &e )
	{
	    cerr << "caught exception" << endl << e.get_error_description() ;
	    return 1 ;
	}
    }
    cout << endl << "*****************************************" << endl;
    cout << "syntax correct, add in syntax create sym2" << endl;
    {
	try
	{
	    DODSParser p ;
	    DODSDataHandlerInterface dhi ;
	    p.parse( "set container in volatile values sym2,real2,type2;", dhi ) ;
	    DODSResponseHandler *rh = dhi.response_handler ;
	    if( rh )
	    {
		rh->execute( dhi ) ;
		delete rh ;
	    }
	    else
	    {
		cerr << "response handler not set, exiting" << endl ;
		return 1 ;
	    }
	}
	catch( DODSException &e )
	{
	    cerr << "caught exception" << endl << e.get_error_description() ;
	    return 1 ;
	}
    }
    cout << endl << "*****************************************" << endl;
    cout << "syntax correct, create sym3" << endl;
    {
	try
	{
	    DODSParser p ;
	    DODSDataHandlerInterface dhi ;
	    p.parse( "set container in volatile values sym3,real3,type3;", dhi ) ;
	    DODSResponseHandler *rh = dhi.response_handler ;
	    if( rh )
	    {
		rh->execute( dhi ) ;
		delete rh ;
	    }
	    else
	    {
		cerr << "response handler not set, exiting" << endl ;
		return 1 ;
	    }
	}
	catch( DODSException &e )
	{
	    cerr << "caught exception" << endl << e.get_error_description() ;
	    return 1 ;
	}
    }
    cout << endl << "*****************************************" << endl;
    cout << "fetch those containers" << endl;
    {
	DODSContainer d( "sym1" ) ;
	ThePersistenceList->look_for( d ) ;
	if( d.is_valid() && d.get_symbolic_name() == "sym1" )
	{
	    cout << "found sym1" << endl ;
	}
	else
	{
	    cerr << "didn't find sym1" << endl ;
	    return 1 ;
	}
    }
    {
	DODSContainer d( "sym2" ) ;
	ThePersistenceList->look_for( d ) ;
	if( d.is_valid() && d.get_symbolic_name() == "sym2" )
	{
	    cout << "found sym2" << endl ;
	}
	else
	{
	    cerr << "didn't find sym2" << endl ;
	    return 1 ;
	}
    }
    {
	DODSContainer d( "sym3" ) ;
	ThePersistenceList->look_for( d ) ;
	if( d.is_valid() && d.get_symbolic_name() == "sym3" )
	{
	    cout << "found sym3" << endl ;
	}
	else
	{
	    cerr << "didn't find sym3" << endl ;
	    return 1 ;
	}
    }
    cout << endl << "*****************************************" << endl;
    cout << "create definition" << endl;
    {
	try
	{
	    DODSParser p ;
	    DODSDataHandlerInterface dhi ;
	    string req = (string)"define d1 as sym1,sym2,sym3 with "
		    + "sym1.constraint=\"const11,const12\""
		    + ",sym1.attributes=\"attr11,attr12\""
		    + ",sym2.constraint=\"const21,const22\""
		    + ",sym2.attributes=\"attr21,attr22\""
		    + ",sym3.constraint=\"const31,const32\""
		    + ",sym3.attributes=\"attr31,attr32\""
		    + "aggregate by \"a,c,e\";" ;
	    p.parse( req, dhi ) ;
	    DODSResponseHandler *rh = dhi.response_handler ;
	    if( rh )
	    {
		rh->execute( dhi ) ;
		delete rh ;
	    }
	    else
	    {
		cerr << "response handler not set, exiting" << endl ;
		return 1 ;
	    }
	}
	catch( DODSException &e )
	{
	    cerr << "caught exception" << endl << e.get_error_description() ;
	    return 1 ;
	}
    }
    cout << endl << "*****************************************" << endl;
    cout << "get that definition" << endl;
    {
	try
	{
	    DODSDefine *dd = TheDefineList->find_def( "d1" ) ;
	    if( dd )
	    {
		cout << "found definition d1" << endl ;
		unsigned short j = 1 ;
		DODSDefine::containers_iterator i = dd->first_container() ;
		DODSDefine::containers_iterator e = dd->end_container() ;
		for( ; i != e; i++ )
		{
		    stringstream sym ;
		    sym << "sym" << j ;
		    stringstream real ;
		    real << "real" << j ;
		    stringstream type ;
		    type << "type" << j ;
		    stringstream constraint ;
		    constraint << "const" << j << "1,const" << j << "2" ;
		    stringstream attrs ;
		    attrs << "attr" << j << "1,attr" << j << "2" ;
		    if( (*i).is_valid() == false ||
		        (*i).get_symbolic_name() != sym.str() ||
		        (*i).get_real_name() != real.str() ||
		        (*i).get_container_type() != type.str() ||
		        (*i).get_constraint() != constraint.str() ||
		        (*i).get_attributes() != attrs.str() )
		    {
			cerr << "something ain't right" << endl ;
			cerr << "    sym = " << (*i).get_symbolic_name()
			     << ", should be " << sym.str() << endl ;
			cerr << "    real = " << (*i).get_real_name()
			     << ", shold be " << real.str() << endl ;
			cerr << "    type = " << (*i).get_container_type()
			     << ", should be " << type.str() << endl ;
			return 1 ;
		    }
		    else
		    {
			cout << "    found " << sym.str() << endl ;
		    }
		    j++ ;
		}
		if( dd->aggregation_command != "a,c,e" )
		{
		    cerr << "agg = " << dd->aggregation_command
		         << ", should be a,c,e"
			 << endl ;
		    return 1 ;
		}
		else
		{
		    cout << "    aggregation correct" << endl ;
		}
	    }
	    else
	    {
		cerr << "couldn't find the definition" << endl ;
		return 1 ;
	    }
	}
	catch( DODSException &e )
	{
	    cerr << "caught exception" << endl << e.get_error_description() ;
	    return 1 ;
	}
    }
    cout << endl << "*****************************************" << endl;
    cout << "parse a get command" << endl;
    {
	try
	{
	    DODSParser p ;
	    DODSDataHandlerInterface dhi ;
	    p.parse( "get dods for d1 return as \"netcdf\";", dhi ) ;
	    unsigned short j = 1 ;
	    dhi.first_container() ;
	    while( dhi.container )
	    {
		stringstream sym ;
		sym << "sym" << j ;
		if( dhi.container->get_symbolic_name() != sym.str() )
		{
		    cerr << "    container "
		         << dhi.container->get_symbolic_name()
		         << ", should be " << sym.str() << endl ;
		    return 1 ;
		}
		else
		{
		    cout << "    found " << sym.str() << endl ;
		}
		dhi.next_container() ;
		j++ ;
	    }
	    if( dhi.return_command != "netcdf" )
	    {
		cerr << "    return command = " << dhi.return_command
		     << ", should be netcdf" << endl ;
		return 1 ;
	    }
	    else
	    {
		cout << "    return command = " << dhi.return_command << endl ;
	    }
	    if( dhi.aggregation_command != "a,c,e" )
	    {
		cerr << "    agg command = " << dhi.aggregation_command
		     << ", should be netcdf" << endl ;
		return 1 ;
	    }
	    else
	    {
		cout << "    agg command = " << dhi.aggregation_command << endl;
	    }
	    DODSResponseHandler *rh = dhi.response_handler ;
	    if( rh )
	    {
		delete rh ;
	    }
	    else
	    {
		cerr << "response handler not set, exiting" << endl ;
		return 1 ;
	    }
	}
	catch( DODSException &e )
	{
	    cerr << "caught exception" << endl << e.get_error_description() ;
	    return 1 ;
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "create definitions d2,d3 and d4" << endl;
    {
	try
	{
	    {
		DODSParser p ;
		DODSDataHandlerInterface dhi ;
		string req = (string)"define d2 as sym1;" ;
		p.parse( req, dhi ) ;
		DODSResponseHandler *rh = dhi.response_handler ;
		if( rh )
		{
		    rh->execute( dhi ) ;
		    delete rh ;
		}
		else
		{
		    cerr << "response handler not set, exiting" << endl ;
		    return 1 ;
		}
	    }
	    {
		DODSParser p ;
		DODSDataHandlerInterface dhi ;
		string req = (string)"define d3 as sym1;" ;
		p.parse( req, dhi ) ;
		DODSResponseHandler *rh = dhi.response_handler ;
		if( rh )
		{
		    rh->execute( dhi ) ;
		    delete rh ;
		}
		else
		{
		    cerr << "response handler not set, exiting" << endl ;
		    return 1 ;
		}
	    }
	    {
		DODSParser p ;
		DODSDataHandlerInterface dhi ;
		string req = (string)"define d4 as sym1;" ;
		p.parse( req, dhi ) ;
		DODSResponseHandler *rh = dhi.response_handler ;
		if( rh )
		{
		    rh->execute( dhi ) ;
		    delete rh ;
		}
		else
		{
		    cerr << "response handler not set, exiting" << endl ;
		    return 1 ;
		}
	    }
	}
	catch( DODSException &e )
	{
	    cerr << "caught exception" << endl << e.get_error_description() ;
	    return 1 ;
	}

	DODSDefine *dd = TheDefineList->find_def( "d1" ) ;
	if( dd )
	{
	    cout << "found definition d1" << endl ;
	}
	else
	{
	    cerr << "couldn't find definition d1" << endl ;
	    return 1 ;
	}
	dd = TheDefineList->find_def( "d2" ) ;
	if( dd )
	{
	    cout << "found definition d2" << endl ;
	}
	else
	{
	    cerr << "couldn't find definition d2" << endl ;
	    return 1 ;
	}
	dd = TheDefineList->find_def( "d3" ) ;
	if( dd )
	{
	    cout << "found definition d3" << endl ;
	}
	else
	{
	    cerr << "couldn't find definition d3" << endl ;
	    return 1 ;
	}
	dd = TheDefineList->find_def( "d4" ) ;
	if( dd )
	{
	    cout << "found definition d4" << endl ;
	}
	else
	{
	    cerr << "couldn't find definition d4" << endl ;
	    return 1 ;
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "show containers and definitions" << endl;
    DODSTextInfo info( false ) ;
    ThePersistenceList->show_containers( info ) ;
    info.add_data( "\n\n" ) ;
    TheDefineList->show_definitions( info ) ;
    info.print( stdout ) ;

    string retKey =
	TheDODSKeys->set_key( "DODS.Container.Persistence", "nice" ) ;
    if( retKey == "" )
    {
	cerr << "failed to set persistence to nice" << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Delete container sym2 using default store" << endl;
    {
	try
	{
	    DODSParser p ;
	    DODSDataHandlerInterface dhi ;
	    string req = (string)"delete container sym2;" ;
	    p.parse( req, dhi ) ;
	    DODSResponseHandler *rh = dhi.response_handler ;
	    if( rh )
	    {
		rh->execute( dhi ) ;
		DODSContainer d( "sym2" ) ;
		ThePersistenceList->look_for( d ) ;
		if( d.is_valid() == false )
		{
		    cout << "couldn't find container sym2, good" << endl ;
		}
		else
		{
		    cerr << "found sym2, bad" << endl ;
		    return 1 ;
		}
		delete rh ;
	    }
	    else
	    {
		cerr << "response handler not set, exiting" << endl ;
		return 1 ;
	    }
	}
	catch( DODSException &e )
	{
	    cerr << "problem deleting sym2" << endl ;
	    cerr << e.get_error_description() << endl ;
	    return 1 ;
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "Delete container sym3 using volatile" << endl;
    {
	try
	{
	    DODSParser p ;
	    DODSDataHandlerInterface dhi ;
	    string req = (string)"delete container sym3 from volatile;" ;
	    p.parse( req, dhi ) ;
	    DODSResponseHandler *rh = dhi.response_handler ;
	    if( rh )
	    {
		rh->execute( dhi ) ;
		DODSContainer d( "sym3" ) ;
		ThePersistenceList->look_for( d ) ;
		if( d.is_valid() == false )
		{
		    cout << "couldn't find container sym3, good" << endl ;
		}
		else
		{
		    cerr << "found sym3, bad" << endl ;
		    return 1 ;
		}
		delete rh ;
	    }
	    else
	    {
		cerr << "response handler not set, exiting" << endl ;
		return 1 ;
	    }
	}
	catch( DODSException &e )
	{
	    cerr << "problem deleting sym3" << endl ;
	    cerr << e.get_error_description() << endl ;
	    return 1 ;
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "Try to delete container sym1 using no_store" << endl;
    {
	try
	{
	    DODSParser p ;
	    DODSDataHandlerInterface dhi ;
	    string req = (string)"delete container sym1 from no_store;" ;
	    p.parse( req, dhi ) ;
	    DODSResponseHandler *rh = dhi.response_handler ;
	    if( rh )
	    {
		rh->execute( dhi ) ;
		DODSContainer d( "sym1" ) ;
		ThePersistenceList->look_for( d ) ;
		if( d.is_valid() == true )
		{
		    cout << "found sym1, good" << endl ;
		}
		else
		{
		    cerr << "couldn't find sym1, bad" << endl ;
		    return 1 ;
		}
		delete rh ;
	    }
	    else
	    {
		cerr << "response handler not set, exiting" << endl ;
		return 1 ;
	    }
	}
	catch( DODSException &e )
	{
	    cerr << "problem trying to delete sym1" << endl ;
	    cerr << e.get_error_description() << endl ;
	    return 1 ;
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "Try to delete container no_sym using volatile" << endl;
    {
	try
	{
	    DODSParser p ;
	    DODSDataHandlerInterface dhi ;
	    string req = (string)"delete container no_sym from volatile;" ;
	    p.parse( req, dhi ) ;
	    DODSResponseHandler *rh = dhi.response_handler ;
	    if( rh )
	    {
		rh->execute( dhi ) ;
		DODSContainer d( "sym1" ) ;
		ThePersistenceList->look_for( d ) ;
		if( d.is_valid() == true )
		{
		    cout << "found sym1, good" << endl ;
		}
		else
		{
		    cerr << "couldn't find sym1, bad" << endl ;
		    return 1 ;
		}
		delete rh ;
	    }
	    else
	    {
		cerr << "response handler not set, exiting" << endl ;
		return 1 ;
	    }
	}
	catch( DODSException &e )
	{
	    cerr << "problem trying to delete sym1" << endl ;
	    cerr << e.get_error_description() << endl ;
	    return 1 ;
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "Delete definition def2" << endl;
    {
	try
	{
	    DODSParser p ;
	    DODSDataHandlerInterface dhi ;
	    string req = (string)"delete definition d2;" ;
	    p.parse( req, dhi ) ;
	    DODSResponseHandler *rh = dhi.response_handler ;
	    if( rh )
	    {
		rh->execute( dhi ) ;
		DODSDefine *d = TheDefineList->find_def( "d2" ) ;
		if( !d )
		{
		    cout << "couldn't find definition d2, good" << endl ;
		}
		else
		{
		    cerr << "found d2, bad" << endl ;
		    return 1 ;
		}
		delete rh ;
	    }
	    else
	    {
		cerr << "response handler not set, exiting" << endl ;
		return 1 ;
	    }
	}
	catch( DODSException &e )
	{
	    cerr << "problem deleting d2" << endl ;
	    cerr << e.get_error_description() << endl ;
	    return 1 ;
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "Delete definition d4" << endl;
    {
	try
	{
	    DODSParser p ;
	    DODSDataHandlerInterface dhi ;
	    string req = (string)"delete definition d4;" ;
	    p.parse( req, dhi ) ;
	    DODSResponseHandler *rh = dhi.response_handler ;
	    if( rh )
	    {
		rh->execute( dhi ) ;
		DODSDefine *d = TheDefineList->find_def( "d4" ) ;
		if( !d )
		{
		    cout << "couldn't find definition d4, good" << endl ;
		}
		else
		{
		    cerr << "found d4, bad" << endl ;
		    return 1 ;
		}
		delete rh ;
	    }
	    else
	    {
		cerr << "response handler not set, exiting" << endl ;
		return 1 ;
	    }
	}
	catch( DODSException &e )
	{
	    cerr << "problem deleting d4" << endl ;
	    cerr << e.get_error_description() << endl ;
	    return 1 ;
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "Delete definitions" << endl;
    {
	try
	{
	    DODSParser p ;
	    DODSDataHandlerInterface dhi ;
	    string req = (string)"delete definitions;" ;
	    p.parse( req, dhi ) ;
	    DODSResponseHandler *rh = dhi.response_handler ;
	    if( rh )
	    {
		rh->execute( dhi ) ;
		DODSDefine *d = TheDefineList->find_def( "d1" ) ;
		if( !d )
		{
		    cout << "couldn't find definition d1, good" << endl ;
		}
		else
		{
		    cerr << "found d1, bad" << endl ;
		    return 1 ;
		}
		d = TheDefineList->find_def( "d3" ) ;
		if( !d )
		{
		    cout << "couldn't find definition d3, good" << endl ;
		}
		else
		{
		    cerr << "found d3, bad" << endl ;
		    return 1 ;
		}
		delete rh ;
	    }
	    else
	    {
		cerr << "response handler not set, exiting" << endl ;
		return 1 ;
	    }
	}
	catch( DODSException &e )
	{
	    cerr << "problem deleting definitions" << endl ;
	    cerr << e.get_error_description() << endl ;
	    return 1 ;
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "Returning from parserT::run" << endl;

    return retVal;
}

int
main(int argC, char **argV) {
    putenv( "DODS_INI=./parserT.ini" ) ;
    Application *app = new parserT();
    return app->main(argC, argV);
}

