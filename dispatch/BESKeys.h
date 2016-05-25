// BESKeys.h

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

#ifndef BESKeys_h_
#define BESKeys_h_ 1

#include <fstream>
#include <map>
#include <vector>
#include <string>

using std::string;
using std::map;
using std::vector;
using std::ifstream;

#include "BESObj.h"

/** @brief mapping of key/value pairs defining different behaviors of an
 * application.
 *
 * BESKeys provides a mechanism to define the behavior of an application
 * given key/value pairs. For example, how authentication will work, database
 * access information, level of debugging and where log files are to be
 * located.
 *
 * Key/value pairs can be loaded from an external initialization file or set
 * within the application itself, for example from the command line.
 *
 * If from a file the key/value pair is set one per line and cannot span
 * multiple lines. Comments are allowed using the pound (#) character. For
 * example:
 *
 * @verbatim
 #
 # Who is responsible for this server
 #
 BES.ServerAdministrator=support@opendap.org

 #
 # Default server port and unix socket information and whether the server
 #is secure or not.
 #
 BES.ServerPort=10022
 BES.ServerUnixSocket=/tmp/bes.socket
 BES.ServerSecure=no
 * @endverbatim
 *
 * Key/value pairs can also be set by passing in a key=value string, or by
 * passing in a key and value string to the object.
 *
 * BES provides a single object for access to a single BESKeys object,
 * TheBESKeys.
 */
class BESKeys: public BESObj
{
private:
	ifstream * _keys_file;
	string _keys_file_name;
	map<string, vector<string> > *_the_keys;
	bool _own_keys;

	static vector<string> KeyList;
	static bool LoadedKeys(const string &key_file);

	void clean();
	void initialize_keys();
	void load_keys();
	bool break_pair(const char* b, string& key, string &value, bool &addto);
	bool only_blanks(const char *line);
	void load_include_files(const string &files);
	void load_include_file(const string &file);

	BESKeys() :
			_keys_file(0), _keys_file_name(""), _the_keys(0), _own_keys(false)
	{
	}

	BESKeys(const string &keys_file_name, map<string, vector<string> > *keys);

protected:
	BESKeys(const string &keys_file_name);

public:
	virtual ~BESKeys();

	string keys_file_name() const
	{
		return _keys_file_name;
	}

	void set_key(const string &key, const string &val, bool addto = false);
	void set_key(const string &pair);
	void get_value(const string& s, string &val, bool &found);
	void get_values(const string& s, vector<string> &vals, bool &found);

	typedef map<string, vector<string> >::const_iterator Keys_citer;

	Keys_citer keys_begin()
	{
		return _the_keys->begin();
	}

	Keys_citer keys_end()
	{
		return _the_keys->end();
	}

	virtual void dump(ostream &strm) const;
};

#endif // BESKeys_h_

