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
#include <unordered_map>
#include <set>
#include <vector>
#include <string>
#include <memory>

#include "BESObj.h"

constexpr auto DYNAMIC_CONFIG_KEY = "DynamicConfig";
constexpr auto DC_REGEX_KEY = "regex";
constexpr auto DC_CONFIG_KEY = "config";

#define DYNAMIC_CONFIG_ENABLED 0

namespace http {
class HttpCacheTest;
}

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
 # is secure or not.
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

    friend class keysT;
    friend class http::HttpCacheTest;

    std::string d_keys_file_name;

    std::unordered_map< std::string, std::vector<std::string> > d_the_keys;

#if DYNAMIC_CONFIG_ENABLED
    std::unique_ptr<keys_kvp> d_the_original_keys{new keys_kvp()};
#endif

    bool d_dynamic_config_in_use = false;
    bool d_own_keys = false;

    std::set<std::string> d_ingested_key_files;

    // Only called by the static TheBESKeys::TheKeys() method.
    explicit TheBESKeys(std::string keys_file_name);

public:
    /**
     * TheBESKeys::ConfigFile provides a way for the daemon and test code to
     * set the location of a particular configuration file.
     */
    static std::string ConfigFile;

    TheBESKeys() = delete;
    TheBESKeys(const TheBESKeys &) = delete;
    TheBESKeys(TheBESKeys &&) = delete;
    TheBESKeys &operator=(const TheBESKeys &) = delete;
    TheBESKeys &operator=(TheBESKeys &&) = delete;
    ~TheBESKeys() override = default;

    /// Access to the singleton.
    static TheBESKeys *TheKeys();

    std::string keys_file_name() const {
        return d_keys_file_name;
    }

    void reload_keys();

    /**
     * @brief Delete the key
     * Added primarily for testing purposes.
     * @param key
     */
    void delete_key(const std::string &key) {
        d_the_keys.erase(key);
    }

    void set_key(const std::string &key, const std::string &val, bool addto = false);

    void set_key(const std::string &pair);

    void set_keys(const std::string &key, const std::vector<std::string> &values, bool addto);

    void set_keys(const std::string &key, const std::unordered_map<std::string, std::string> &values,
                  bool case_insensitive_map_keys, bool addto);

    void get_value(const std::string &s, std::string &val, bool &found);

    // get values for a vector-valued key
    void get_values(const std::string &s, std::vector<std::string> &vals, bool &found);

    // get value for a map-valued key
    void get_values(const std::string &, std::unordered_map<std::string, std::string> &map_values,
                    bool case_insensitive_map_keys, bool &found);

    void get_values(const std::string &, std::unordered_map<std::string,
                    std::unordered_map<std::string, std::vector<std::string> > > &map,
                    bool case_insensitive_map_keys, bool &found);

    static bool read_bool_key(const std::string &key, bool default_value);

    static std::string read_string_key(const std::string &key, const std::string &default_value);

    static int read_int_key(const std::string &key, int default_value);

    static unsigned long read_ulong_key(const std::string &key, unsigned long default_value);

    static uint64_t read_uint64_key(const std::string &key, uint64_t default_value);

    std::unordered_map<std::string, std::vector<std::string> >::const_iterator keys_begin() {
        return d_the_keys.begin();
    }

    std::unordered_map<std::string, std::vector<std::string> >::const_iterator keys_end() {
        return d_the_keys.end();
    }

    std::string get_as_config() const;

    void load_dynamic_config(const std::string &name);

    bool using_dynamic_config() const {
        return d_dynamic_config_in_use;
    }

    void dump(std::ostream &strm) const override;
    virtual std::string dump() const;
};

#endif // TheBESKeys_h_

