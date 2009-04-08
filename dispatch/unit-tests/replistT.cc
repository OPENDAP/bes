// replistT.C

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

using std::cerr ;
using std::cout ;
using std::endl ;

#include "replistT.h"
#include "BESReporterList.h"
#include "TestReporter.h"

int replistT::
run(void)
{
    cout << endl << "*****************************************" << endl;
    cout << "Entered replistT::run" << endl;
    int retVal = 0;

    cout << endl << "*****************************************" << endl;
    cout << "add the 5 reporters" << endl ;
    BESReporterList *rl = BESReporterList::TheList() ;
    char num[10] ;
    for( int i = 0; i < 5; i++ )
    {
	sprintf( num, "rep%d", i ) ;
	if( rl->add_reporter( num, new TestReporter( num ) ) == true )
	{
	    cout << "successfully added " << num << endl ;
	}
	else
	{
	    cerr << "failed to add " << num << endl ;
	    return 1 ;
	}
    }

    cout << endl << "*****************************************" << endl;
    cout << "try to add rep3 again" << endl ;
    TestReporter *r = new TestReporter( "rep3" ) ;
    if( rl->add_reporter( "rep3", r ) == true )
    {
	cerr << "successfully added rep3 again" << endl ;
	return 1 ;
    }
    else
    {
	cout << "failed to add rep3 again, good" << endl ;
	delete r ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "finding the reporters" << endl ;
    for( int i = 4; i >=0; i-- )
    {
	sprintf( num, "rep%d", i ) ;
	r = (TestReporter *)rl->find_reporter( num ) ;
	if( r )
	{
	    if( r->get_name() == num )
	    {
		cout << "found " << num << endl ;
	    }
	    else
	    {
		cerr << "looking for " << num
		     << ", found " << r->get_name() << endl ;
		return 1 ;
	    }
	}
	else
	{
	    cerr << "coundn't find " << num << endl ;
	    return 1 ;
	}
    }
    r = (TestReporter *)rl->find_reporter( "thingy" ) ;
    if( r )
    {
	if( r->get_name() == "thingy" )
	{
	    cerr << "found thingy" << endl ;
	    return 1 ;
	}
	else
	{
	    cerr << "looking for thingy, found " << r->get_name() << endl ;
	    return 1 ;
	}
    }
    else
    {
	cout << "coundn't find thingy" << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "removing rep2" << endl ;
    r = (TestReporter *)rl->remove_reporter( "rep2" ) ;
    if( r )
    {
	string name = r->get_name() ;
	if( name == "rep2" )
	{
	    cout << "successfully removed rep2" << endl ;
	    delete r ;
	}
	else
	{
	    cerr << "trying to remove rep2, but removed " << name << endl ;
	    return 1 ;
	}
    }
    else
    {
	cerr << "failed to remove rep2" << endl ;
	return 1 ;
    }

    r = (TestReporter *)rl->find_reporter( "rep2" ) ;
    if( r )
    {
	if( r->get_name() == "rep2" )
	{
	    cerr << "found rep2, should have been removed" << endl ;
	    return 1 ;
	}
	else
	{
	    cerr << "found " << r->get_name() << " when looking for rep2"
		 << endl ;
	    return 1 ;
	}
    }
    else
    {
	cout << "couldn't find rep2, good" << endl ;
    }

    if( rl->add_reporter( "rep2", new TestReporter( "rep2" ) ) == true )
    {
	cout << "successfully added rep2 back" << endl ;
    }
    else
    {
	cerr << "failed to add rep2 back" << endl ;
	return 1 ;
    }

    r = (TestReporter *)rl->find_reporter( "rep2" ) ;
    if( r )
    {
	if( r->get_name() == "rep2" )
	{
	    cout << "found rep2" << endl ;
	}
	else
	{
	    cerr << "looking for rep2, found " << r->get_name() << endl ;
	    return 1 ;
	}
    }
    else
    {
	cerr << "coundn't find rep2" << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "report" << endl;
    BESDataHandlerInterface dhi ;
    rl->report( dhi ) ;

    cout << endl << "*****************************************" << endl;
    cout << "Returning from replistT::run" << endl;

    return retVal;
}

int
main(int argC, char **argV) {
    Application *app = new replistT();
    return app->main(argC, argV);
}

