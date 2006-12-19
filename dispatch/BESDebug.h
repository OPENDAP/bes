// BESDebug.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
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
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

/** @brief top level BES object to house generic methods
 */

#ifndef I_BESDebug_h
#define I_BESDebug_h 1

#include <iostream>

using std::cout ;
using std::endl ;
using std::ostream ;

#define BESDEBUG( x ) { if( BESDebug::Debug() ) *(BESDebug::Get_strm()) << x ; }

class BESDebug
{
private:
    bool			_debug ;
    ostream *			_strm ;
    				BESDebug() : _debug( false ), _strm( 0 ) {}
    static BESDebug *		_debugger ;
public:
    				BESDebug( ostream *strm ) : _strm( strm ) {}
    virtual			~BESDebug() {}
    virtual	void	begin_debug() { _debug = true ; }
    virtual	void	end_debug() { _debug = false ; }
    virtual bool		debug() { return _debug ; }
    virtual ostream *		get_strm() { return _strm ; }

    static void			Set_debugger( BESDebug *debugger )
    				{
				    if( BESDebug::_debugger )
					delete BESDebug::_debugger ;
				    BESDebug::_debugger = debugger ;
				}
    static void			Begin_debug()
    				{
				    if( BESDebug::_debugger )
					BESDebug::_debugger->begin_debug() ;
				}
    static void			End_debug()
    				{
				    if( BESDebug::_debugger )
					BESDebug::_debugger->end_debug() ;
				}
    static bool			Debug()
    				{
				    if( BESDebug::_debugger )
					return BESDebug::_debugger->debug() ;
				    else
					return false ;
				}
    static ostream *		Get_strm()
    				{
				    if( BESDebug::_debugger )
					return BESDebug::_debugger->get_strm() ;
				    else
					return 0 ;
				}
} ;

#endif // I_BESDebug_h

/*
int
main( int argc, char **argv )
{
    int some_number = 1 ;
    BESDEBUG( "Shouldn't be seeing this part 1: " << some_number++ << endl )
    BESDebug::Set_debugger( new BESDebug( &cout ) ) ;
    BESDEBUG( "Shouldn't be seeing this part 2: " << some_number++ << endl )
    BESDebug::Begin_debug() ;
    BESDEBUG( "Should be seeing this: " << some_number++ << endl )

    return 0 ;
}
*/

