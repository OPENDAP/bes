// ConnSocket.cc

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

#include "cppunit/TestAssert.h"

#include <iostream>
#include <cstring>

using std::cout ;
using std::endl ;
using std::string ;
using std::ostream ;

#include "ConnSocket.h"
#include "ConnTestStrs.h"
#include "PPTProtocol.h"

ConnSocket::ConnSocket()
    : _test_num( 0 ),
      _test_rec( 0 )
{
}

ConnSocket::~ConnSocket()
{
}

void
ConnSocket::connect()
{
}

void
ConnSocket::listen()
{
}

void
ConnSocket::close()
{
}

void
ConnSocket::send( const string &str, int start, int end )
{
    cout << "****" << endl << str << endl << "****" << endl ;
    CPPUNIT_ASSERT( str == test_exp[_test_num++] ) ;
}

int
ConnSocket::receive( char *inBuff, int inSize )
{
    if( _test_rec == 0 )
    {
	_test_rec++ ;
	memcpy( inBuff, test_exp[0].c_str(), 8 ) ;
	return 8 ;
    }
    if( _test_rec == 1 )
    {
	_test_rec++ ;
	string this_return = test_exp[0].substr( 8, test_exp[0].length() - 8 ) ;
	memcpy( inBuff, this_return.c_str(), this_return.length() ) ;
	return this_return.length() ;
    }
    if( _test_rec == 2 )
    {
	_test_rec++ ;
	memcpy( inBuff, test_exp[0].c_str(), 8 ) ;
	return 8 ;
    }
    if( _test_rec == 3 )
    {
	cout << "test receive " << _test_rec << endl ;
	_test_rec++ ;
	string this_return = test_exp[0].substr( 8, 5 ) ;
	memcpy( inBuff, this_return.c_str(), this_return.length() ) ;
	return this_return.length() ;
    }
    if( _test_rec == 4 )
    {
	cout << "test receive " << _test_rec << endl ;
	_test_rec++ ;
	string this_return = test_exp[0].substr( 13, 5 ) ;
	memcpy( inBuff, this_return.c_str(), this_return.length() ) ;
	return this_return.length() ;
    }
    if( _test_rec == 5 )
    {
	cout << "test receive " << _test_rec << endl ;
	_test_rec++ ;
	string this_return = test_exp[0].substr( 18, 4 ) ;
	memcpy( inBuff, this_return.c_str(), this_return.length() ) ;
	return this_return.length() ;
    }
    if( _test_rec == 6 )
    {
	cout << "test receive " << _test_rec << endl ;
	_test_rec++ ;
	memcpy( inBuff, test_exp[2].c_str(), 8 ) ;
	return 8 ;
    }
    if( _test_rec == 7 )
    {
	cout << "test receive " << _test_rec << endl ;
	_test_rec++ ;
	string this_return = test_exp[2].substr( 8, 15 ) ;
	memcpy( inBuff, this_return.c_str(), this_return.length() ) ;
	return this_return.length() ;
    }
    if( _test_rec == 8 )
    {
	cout << "test receive " << _test_rec << endl ;
	_test_rec++ ;
	string this_return = "0000000d" ;
	memcpy( inBuff, this_return.c_str(), this_return.length() ) ;
	return this_return.length() ;
    }
    if( _test_rec == 9 )
    {
	cout << "test receive " << _test_rec << endl ;
	_test_rec++ ;
	string this_return = test_exp[3].substr( 0, 8 ) ;
	memcpy( inBuff, this_return.c_str(), this_return.length() ) ;
	return this_return.length() ;
    }
    if( _test_rec == 10 )
    {
	cout << "test receive " << _test_rec << endl ;
	_test_rec++ ;
	string this_return = test_exp[3].substr( 8, 15 ) ;
	memcpy( inBuff, this_return.c_str(), this_return.length() ) ;
	return this_return.length() ;
    }
    if( _test_rec == 11 )
    {
	cout << "test receive " << _test_rec << endl ;
	_test_rec++ ;
	string this_return = test_exp[3].substr( 23, 5 ) ;
	memcpy( inBuff, this_return.c_str(), this_return.length() ) ;
	cout << "returning \"" << this_return << "\" of length " << this_return.length() << endl ;
	return this_return.length() ;
    }
    if( _test_rec == 12 )
    {
	cout << "test receive " << _test_rec << endl ;
	_test_rec++ ;
	string this_return = test_exp[3].substr( 28, 24 ) ;
	memcpy( inBuff, this_return.c_str(), this_return.length() ) ;
	cout << "returning \"" << this_return << "\" of length " << this_return.length() << endl ;
	return this_return.length() ;
    }
    if( _test_rec == 13 )
    {
	cout << "test receive " << _test_rec << endl ;
	_test_rec++ ;
	string this_return = "0000000d" ;
	memcpy( inBuff, this_return.c_str(), this_return.length() ) ;
	return this_return.length() ;
    }

    return 0 ;
}

Socket *
ConnSocket::newSocket( int, struct sockaddr * )
{
    return new ConnSocket ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
ConnSocket::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "ConnSocket::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    Socket::dump( strm ) ;
    BESIndent::UnIndent() ;
}

