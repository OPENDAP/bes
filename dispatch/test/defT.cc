// defT.C

#include <iostream>
#include <sstream>

using std::cerr ;
using std::cout ;
using std::endl ;
using std::stringstream ;

#include "defT.h"
#include "BESDefinitionStorageList.h"
#include "BESDefinitionStorageVolatile.h"
#include "BESDefine.h"
#include "BESTextInfo.h"
#include "BESException.h"

int defT::
run(void)
{
    cout << endl << "*****************************************" << endl;
    cout << "Entered defT::run" << endl;
    int retVal = 0;

    BESDefinitionStorageList::TheList()->add_persistence( new BESDefinitionStorageVolatile( PERSISTENCE_VOLATILE ) ) ;
    BESDefinitionStorage *store = BESDefinitionStorageList::TheList()->find_persistence( PERSISTENCE_VOLATILE ) ;

    cout << endl << "*****************************************" << endl;
    cout << "add d1, d2, d3, d4, d5" << endl;
    for( unsigned int i = 1; i < 6; i++ )
    {
	stringstream name ; name << "d" << i ;
	stringstream agg ; agg << "d" << i << "agg" ;
	BESDefine *dd = new BESDefine ;
	dd->aggregation_command = agg.str() ;
	bool status = store->add_definition( name.str(), dd ) ;
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
	BESDefine *dd = store->look_for( name.str() ) ;
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
	BESTextInfo info( false ) ;
	store->show_definitions( info ) ;
	info.print( stdout ) ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "delete d3" << endl;
    {
	bool ret = store->del_definition( "d3" ) ;
	if( ret == true )
	{
	    cout << "successfully deleted d3" << endl ;
	}
	else
	{
	    cerr << "unable to delete d3" << endl ;
	    return 1 ;
	}
	BESDefine *dd = store->look_for( "d3" ) ;
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
	bool ret = store->del_definition( "d1" ) ;
	if( ret == true )
	{
	    cout << "successfully deleted d1" << endl ;
	}
	else
	{
	    cerr << "unable to delete d1" << endl ;
	    return 1 ;
	}
	BESDefine *dd = store->look_for( "d1" ) ;
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
	bool ret = store->del_definition( "d5" ) ;
	if( ret == true )
	{
	    cout << "successfully deleted d5" << endl ;
	}
	else
	{
	    cerr << "unable to delete d5" << endl ;
	    return 1 ;
	}
	BESDefine *dd = store->look_for( "d5" ) ;
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
	BESDefine *dd = store->look_for( "d2" ) ;
	if( dd )
	{
	    cout << "found " << "d2" << ", good" << endl ;
	}
	else
	{
	    cerr << "didn't find " << "d2" << ", bad" << endl ;
	    return 1 ;
	}

	dd = store->look_for( "d4" ) ;
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
    store->del_definitions() ;

    cout << endl << "*****************************************" << endl;
    cout << "find definitions d1, d2, d3, d4, d5" << endl;
    for( unsigned int i = 1; i < 6; i++ )
    {
	stringstream name ; name << "d" << i ;
	stringstream agg ; agg << "d" << i << "agg" ;
	BESDefine *dd = store->look_for( name.str() ) ;
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
    putenv( "BES_CONF=./defT.ini" ) ;
    return app->main(argC, argV);
}

