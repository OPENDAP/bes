// pfileT.C

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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

using namespace CppUnit ;

#include <iostream>
#include <cstdlib>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "BESContainerStorageFile.h"
#include "BESContainer.h"
#include "BESError.h"
#include "BESTextInfo.h"
#include "TheBESKeys.h"
#include <test_config.h>

class connT: public TestFixture {
private:

public:
    connT() {}
    ~connT() {}

    void setUp()
    {
	string bes_conf = (string)TEST_SRC_DIR + "/persistence_file_test.ini" ;
	TheBESKeys::ConfigFile = bes_conf ;
    } 

    void tearDown()
    {
    }

    CPPUNIT_TEST_SUITE( connT ) ;

    CPPUNIT_TEST( do_test ) ;

    CPPUNIT_TEST_SUITE_END() ;

    void do_test()
    {
	TheBESKeys *keys = TheBESKeys::TheKeys() ;
	keys->set_key( (string)"BES.Container.Persistence.File.FileNot=" + TEST_SRC_DIR + "/persistence_filenot.txt" ) ;
	keys->set_key( (string)"BES.Container.Persistence.File.FileTooMany=" + TEST_SRC_DIR + "/persistence_file3.txt" ) ;
	keys->set_key( (string)"BES.Container.Persistence.File.FileTooFew=" + TEST_SRC_DIR + "/persistence_file4.txt" ) ;
	keys->set_key( (string)"BES.Container.Persistence.File.File1=" + TEST_SRC_DIR + "/persistence_file1.txt" ) ;
	keys->set_key( (string)"BES.Container.Persistence.File.File2=" + TEST_SRC_DIR + "/persistence_file2.txt" ) ;

	cout << "*****************************************" << endl;
	cout << "Entered pfileT::run" << endl;

	cout << "*****************************************" << endl;
	cout << "Open File, incomplete key information" << endl;
	try
	{
	    BESContainerStorageFile cpf( "File" ) ;
	    CPPUNIT_ASSERT( !"opened file File, shouldn't have" ) ;
	}
	catch( BESError &ex )
	{
	    cout << "couldn't get File, good, because" << endl ;
	    cout << ex.get_message() << endl ;
	}

	cout << "*****************************************" << endl;
	cout << "Open FileNot, doesn't exist" << endl;
	try
	{
	    BESContainerStorageFile cpf( "FileNot" ) ;
	    CPPUNIT_ASSERT( !"opened file FileNot, shouldn't have" ) ;
	}
	catch( BESError &ex )
	{
	    cout << "couldn't get FileNot, good, because" << endl ;
	    cout << ex.get_message() << endl ;
	}

	cout << "*****************************************" << endl;
	cout << "Open FileTooMany, too many values on line" << endl;
	try
	{
	    BESContainerStorageFile cpf( "FileTooMany" ) ;
	    CPPUNIT_ASSERT( ! "opened file FileTooMany, shouldn't have" ) ;
	}
	catch( BESError &ex )
	{
	    cout << "couldn't get FileTooMany, good, because" << endl ;
	    cout << ex.get_message() << endl ;
	}

	cout << "*****************************************" << endl;
	cout << "Open FileTooFew, too few values on line" << endl;
	try
	{
	    BESContainerStorageFile cpf( "FileTooFew" ) ;
	    CPPUNIT_ASSERT( !"opened file FileTooFew, shouldn't have"  ) ;
	}
	catch( BESError &ex )
	{
	    cout << "couldn't get FileTooFew, good, because" << endl ;
	    cout << ex.get_message() << endl ;
	}

	cout << "*****************************************" << endl;
	cout << "Open File1" << endl;
	try
	{
	    BESContainerStorageFile cpf( "File1" ) ;
	}
	catch( BESError &ex )
	{
	    cerr << ex.get_message() << endl ;
	    CPPUNIT_ASSERT( !"couldn't open File1" ) ;
	}

	BESContainerStorageFile cpf( "File1" ) ;
	char s[10] ;
	char r[10] ;
	char c[10] ;
	for( int i = 1; i < 6; i++ )
	{
	    sprintf( s, "sym%d", i ) ;
	    sprintf( r, "real%d", i ) ;
	    sprintf( c, "type%d", i ) ;
	    cout << "    looking for " << s << endl;
	    BESContainer *d = cpf.look_for( s ) ;
	    CPPUNIT_ASSERT( d ) ;
	    CPPUNIT_ASSERT( d->get_real_name() == r ) ;
	    CPPUNIT_ASSERT( d->get_container_type() == c ) ;
	}

	cout << "    looking for notthere" << endl;
	BESContainer *d = cpf.look_for( "notthere" ) ;
	CPPUNIT_ASSERT( !d ) ;

	cout << "*****************************************" << endl;
	cout << "show containers" << endl;
	BESTextInfo info ;
	cpf.show_containers( info ) ;
	info.print( cout ) ;

	cout << "*****************************************" << endl;
	cout << "Returning from pfileT::run" << endl;
    }
} ;

CPPUNIT_TEST_SUITE_REGISTRATION( connT ) ;

int 
main( int, char** )
{
    CppUnit::TextTestRunner runner ;
    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() ) ;

    bool wasSuccessful = runner.run( "", false )  ;

    return wasSuccessful ? 0 : 1 ;
}

