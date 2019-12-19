// TheBESKeys.h

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

#ifndef TheBESKeys_h_
#define TheBESKeys_h_ 1

#include <fstream>
#include <map>
#include <set>
#include <vector>
#include <string>

#include "BESObj.h"

/** @brief mapping of key/value pairs defining different behaviors of an
 * application.
 *
 * TheBESKeys provides a mechanism to define the behavior of an application
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
 BES.ServerAdministrator=email:support@opendap.org

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
class TheBESKeys: public BESObj {
private:

    // TODO I don't think this needs to be a pointer - the code could be
    // redesigned. jhrg 3/7/18
    std::ifstream * _keys_file;
    std::string _keys_file_name;
    std::map<std::string, std::vector<std::string> > *_the_keys;
    bool _own_keys;

    static std::set<std::string> KeyList;
    static bool LoadedKeys(const std::string &key_file);

    void clean();
    void initialize_keys();
    //void load_keys();
    //bool break_pair(const char* b, std::string& key, std::string &value, bool &addto);
    //bool only_blanks(const char *line);
    //void load_include_files(const std::string &files);
    //void load_include_file(const std::string &file);

    TheBESKeys() :
        _keys_file(0), _keys_file_name(""), _the_keys(0), _own_keys(false)
    {
    }

    TheBESKeys(const std::string &keys_file_name, std::map<std::string, std::vector<std::string> > *keys);

protected:
    TheBESKeys(const std::string &keys_file_name);

public:
    static TheBESKeys *_instance;
    virtual ~TheBESKeys();

    std::string keys_file_name() const
    {
        return _keys_file_name;
    }

    void set_key(const std::string &key, const std::string &val, bool addto = false);
    void set_key(const std::string &pair);

    void get_value(const std::string& s, std::string &val, bool &found);
    void get_values(const std::string& s, std::vector<std::string> &vals, bool &found);

    bool read_bool_key(const std::string &key, bool default_value);
    std::string read_string_key(const std::string &key, const std::string &default_value);
    int read_int_key(const std::string &key, int default_value);

    typedef std::map<std::string, std::vector<std::string> >::const_iterator Keys_citer;

    Keys_citer keys_begin()
    {
        return _the_keys->begin();
    }

    Keys_citer keys_end()
    {
        return _the_keys->end();
    }

    virtual void dump(std::ostream &strm) const;

    /**
     * TheBESKeys::ConfigFile provides a way for the daemon and test code to
     * set the location of a particular configuration file.
     */
    static std::string ConfigFile;

    /**
     * Access to the singleton.
     */
    static TheBESKeys *TheKeys();
};

#endif // TheBESKeys_h_

