// defT.C

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
#include <sstream>
#include <cstdlib>

using std::cerr ;
using std::cout ;
using std::endl ;
using std::stringstream ;

#include "defT.h"
#include "BESDefinitionStorageList.h"
#include "BESDefinitionStorageVolatile.h"
#include "BESDefine.h"
#include "BESTextInfo.h"

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
	dd->set_agg_cmd( agg.str() ) ;
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
	    if( dd->get_agg_cmd() == agg.str() )
	    {
		cout << "    agg command correct" << endl ;
	    }
	    else
	    {
		cerr << "    agg command incorrect, = "
		     << dd->get_agg_cmd()
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
	BESTextInfo info ;
	store->show_definitions( info ) ;
	info.print( cout ) ;
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

