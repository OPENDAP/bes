// defT.C

#include <iostream>
#include <sstream>

using std::cerr ;
using std::cout ;
using std::endl ;
using std::stringstream ;

#include "defT.h"
#include "TheDefineList.h"
#include "DODSDefine.h"
#include "DODSTextInfo.h"

int defT::
run(void)
{
    cout << endl << "*****************************************" << endl;
    cout << "Entered defT::run" << endl;
    int retVal = 0;

    cout << endl << "*****************************************" << endl;
    cout << "add d1, d2, d3, d4, d5" << endl;
    for( unsigned int i = 1; i < 6; i++ )
    {
	stringstream name ; name << "d" << i ;
	stringstream agg ; agg << "d" << i << "agg" ;
	DODSDefine *dd = new DODSDefine ;
	dd->aggregation_command = agg.str() ;
	bool status = TheDefineList->add_def( name.str(), dd ) ;
	if( status == true )
	{
	    cout << "successfully added " << name.str() << endl ;
	}
	else
	{
	    cerr << "failed to add " << name.str() << endl ;
	    return 1 ;
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "find d1, d2, d3, d4, d5" << endl;
    for( unsigned int i = 1; i < 6; i++ )
    {
	stringstream name ; name << "d" << i ;
	stringstream agg ; agg << "d" << i << "agg" ;
	DODSDefine *dd = TheDefineList->find_def( name.str() ) ;
	if( dd )
	{
	    cout << "found " << name.str() << endl ;
	    if( dd->aggregation_command == agg.str() )
	    {
		cout << "    agg command correct" << endl ;
	    }
	    else
	    {
		cerr << "    agg command incorrect, = "
		     << dd->aggregation_command
		     << ", should be " << agg.str() << endl ;
		return 1 ;
	    }
	}
	else
	{
	    cerr << "didn't find " << name.str() << endl ;
	    return 1 ;
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "show definitions" << endl;
    {
	DODSTextInfo info( false ) ;
	TheDefineList->show_definitions( info ) ;
	info.print( stdout ) ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "delete d3" << endl;
    {
	bool ret = TheDefineList->remove_def( "d3" ) ;
	if( ret == true )
	{
	    cout << "successfully deleted d3" << endl ;
	}
	else
	{
	    cerr << "unable to delete d3" << endl ;
	    return 1 ;
	}
	DODSDefine *dd = TheDefineList->find_def( "d3" ) ;
	if( dd )
	{
	    cerr << "    found d3, bad" << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "    did not find d3" << endl ;
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "delete d1" << endl;
    {
	bool ret = TheDefineList->remove_def( "d1" ) ;
	if( ret == true )
	{
	    cout << "successfully deleted d1" << endl ;
	}
	else
	{
	    cerr << "unable to delete d1" << endl ;
	    return 1 ;
	}
	DODSDefine *dd = TheDefineList->find_def( "d1" ) ;
	if( dd )
	{
	    cerr << "    found d1, bad" << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "    did not find d1" << endl ;
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "delete d5" << endl;
    {
	bool ret = TheDefineList->remove_def( "d5" ) ;
	if( ret == true )
	{
	    cout << "successfully deleted d5" << endl ;
	}
	else
	{
	    cerr << "unable to delete d5" << endl ;
	    return 1 ;
	}
	DODSDefine *dd = TheDefineList->find_def( "d5" ) ;
	if( dd )
	{
	    cerr << "    found d5, bad" << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "    did not find d5" << endl ;
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "find d2 and d4" << endl;
    {
	DODSDefine *dd = TheDefineList->find_def( "d2" ) ;
	if( dd )
	{
	    cout << "found " << "d2" << ", good" << endl ;
	}
	else
	{
	    cerr << "didn't find " << "d2" << ", bad" << endl ;
	    return 1 ;
	}

	dd = TheDefineList->find_def( "d4" ) ;
	if( dd )
	{
	    cout << "found " << "d4" << ", good" << endl ;
	}
	else
	{
	    cerr << "didn't find " << "d4" << ", bad" << endl ;
	    return 1 ;
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "delete all definitions" << endl;
    TheDefineList->remove_defs() ;

    cout << endl << "*****************************************" << endl;
    cout << "find definitions d1, d2, d3, d4, d5" << endl;
    for( unsigned int i = 1; i < 6; i++ )
    {
	stringstream name ; name << "d" << i ;
	stringstream agg ; agg << "d" << i << "agg" ;
	DODSDefine *dd = TheDefineList->find_def( name.str() ) ;
	if( dd )
	{
	    cerr << "found " << name.str() << ", bad" << endl ;
	    return 1 ;
	}
	else
	{
	    cout << "didn't find " << name.str() << ", good" << endl ;
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "Returning from defT::run" << endl;

    return retVal;
}

int
main(int argC, char **argV) {
    Application *app = new defT();
    putenv( "DODS_INI=./defT.ini" ) ;
    return app->main(argC, argV);
}

