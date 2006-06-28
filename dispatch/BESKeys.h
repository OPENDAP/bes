// BESKeys.h

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

#ifndef BESKeys_h_
#define BESKeys_h_ 1

#include <fstream>
#include <map>
#include <string>

using std::string ;
using std::map ;
using std::ifstream ;

/** @brief mapping of key/value pairs defining different behaviors of an
 * application.
 *
 * BESKeys provides a mechanism to define the behavior of an application
 * given key/value paris. For example, how authentication will work, database
 * access information, level of debugging and where log files are to be
 * located.
 *
 * Key/value pairs can be loaded from an external initialization file or set
 * within the application itself, for example from the command line.
 *
 * If from a file the key/value pair is set one per line and cannot span
 * multiple lines. Comments are allowed using the pound (#) character.
 *
 * <PRE>
 * #
 * # These keys define the behavior of database authentication
 * #
 * OpenDAP.Authentication.MySQL.username=username
 * OpenDAP.Authentication.MySQL.password=password
 * OpenDAP.Authentication.MySQL.server=myMachine
 * OpenDAP.Authentication.MySQL.database=authDB
 * </PRE>
 *
 * Key/value pairs can also be set by passing in a key=value string, or by
 * passing in a key and value string to the object.
 *
 * OpenDAP provides a single object for access to a single BESKeys object,
 * TheBESKeys.
 */
class BESKeys
{
private:
    ifstream *		_keys_file ;
    string		_keys_file_name ;
    map<string,string> *	_the_keys ;

    void		clean() ;
    void 		load_keys() ;
    bool 		break_pair( const char* b,
				    string& key,
				    string &value ) ;
    bool		only_blanks( const char *line ) ;
    			BESKeys() {}
protected:
    			BESKeys( const string &keys_file_name ) ;
public:
    			~BESKeys() ;

    string		keys_file_name() { return _keys_file_name ; }

    string		set_key( const string &key, const string &val ) ;
    string		set_key( const string &pair ) ;
    string		get_key( const string& s, bool &found ) ;
    void		show_keys();

    typedef map< string, string >::const_iterator Keys_citer ;
    Keys_citer		keys_begin() { return _the_keys->begin() ; }
    Keys_citer		keys_end() { return _the_keys->end() ; }
};

#endif // BESKeys_h_

