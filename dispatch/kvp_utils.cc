// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES

// Copyright (c) 2020 OPeNDAP, Inc.
// Author: Nathan Potter<ndp@opendap.org>
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
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// Created by ndp on 12/11/19.
//

#include "config.h"

#include <cerrno>
#include <cstring>

#include <set>
#include <sstream>
#include <string>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "BESFSDir.h"
#include "BESFSFile.h"
#include "BESInternalFatalError.h"
#include "BESUtil.h"

#include "kvp_utils.h"

#define BES_INCLUDE_KEY "BES.Include"

using namespace std;

namespace kvp {

// Forward declaration, implementation at end of file...
void load_keys(const std::string &keys_file_name, set<string> &loaded_kvp_files,
               std::unordered_map<std::string, std::vector<std::string>> &keystore);

bool only_blanks(const char *line) {
    string my_line = line;
    if (my_line.find_first_not_of(' ') != string::npos)
        return false;
    else
        return true;
}

// The string contained in the character buffer b should be of the
// format key=value or key+=value. The pair is broken apart, storing the
// key in the key parameter and the value of the key in the value
// parameter. If += is used, then the value should be added to the value
// of key, not replacing.
//
// It used to be that we would validate the key=value line. Instead,
// anything after the equal sign is considered the value of the key.
bool break_pair(const char *b, string &key, string &value, bool &addto) {
    addto = false;
    // Ignore comments and lines with only spaces
    if (b && (b[0] != '#') && (!only_blanks(b))) {
        size_t l = strlen(b);
        if (l > 1) {
            int pos = 0;
            bool done = false;
            for (size_t j = 0; j < l && !done; j++) {
                if (b[j] == '=') {
                    if (!addto)
                        pos = j;
                    else {
                        if (pos != static_cast<int>(j - 1)) {
                            string s = string("BES: Invalid entry ") + b +
                                       " in configuration file " // + d_keys_file_name
                                       + " '+' character found in variable name" + " or attempting '+=' with space" +
                                       " between the characters.\n";
                            throw BESInternalFatalError(s, __FILE__, __LINE__);
                        }
                    }
                    done = true;
                } else if (b[j] == '+') {
                    addto = true;
                    pos = j;
                }
            }
            if (!done) {
                string s = string("BES: Invalid entry ") + b + " in configuration file, '=' character not found.\n";
                throw BESInternalFatalError(s, __FILE__, __LINE__);
            }

            string s = b;
            key = s.substr(0, pos);
            BESUtil::removeLeadingAndTrailingBlanks(key);
            if (addto)
                value = s.substr(pos + 2, s.size());
            else
                value = s.substr(pos + 1, s.size());
            BESUtil::removeLeadingAndTrailingBlanks(value);
            return true;
        }
        return false;
    }
    return false;
}

/** Load key/value pairs from one include file
 *
 * Helper function to load key/value pairs from a BES configuration file
 *
 * @param file name of the configuration file to load
 */
void load_include_file(const string &file, set<string> &loaded_kvp_files,
                       std::unordered_map<std::string, std::vector<std::string>> &keystore) {
    // make sure the file exists and is readable
    // throws exception if unable to read
    // not loaded if has already be started to be loaded
    auto it = loaded_kvp_files.find(file);

    if (it == loaded_kvp_files.end()) {
        // Didn't find it, better load it...
        loaded_kvp_files.insert(file);
        load_keys(file, loaded_kvp_files, keystore);
    }
}

/** Load key/value pairs from other files
 *
 * Given a file, or a regular expression matching multiple files, where
 * the location is relative to the location of the current keys file,
 * load the keys from that file into this key list
 *
 * @param kvp_files The set of files that have been read.
 * @param file_expr A string representing a file or a regular expression
 * pattern for 1 or more files
 * @param keystore The map into which the key value pairs will be placed.
 */
void load_include_files(const string &current_keys_file_name, const string &file_expr, set<string> &loaded_kvp_files,
                        std::unordered_map<std::string, std::vector<std::string>> &keystore) {
    string newdir = "";
    BESFSFile allfiles(file_expr);

    // If the files specified begin with a /, then use that directory
    // instead of the current keys file directory.
    if (!file_expr.empty() && file_expr[0] == '/') {
        newdir = allfiles.getDirName();
    } else {
        // determine the directory of the current keys file. All included
        // files will be relative to this file.
        BESFSFile currfile(current_keys_file_name);
        string currdir = currfile.getDirName();

        string alldir = allfiles.getDirName();

        if ((currdir == "./" || currdir == ".") && (alldir == "./" || alldir == ".")) {
            newdir = "./";
        } else {
            if (alldir == "./" || alldir == ".") {
                newdir = currdir;
            } else {
                newdir = currdir + "/" + alldir;
            }
        }
    }

    // load the files one at a time. If the directory doesn't exist,
    // then don't load any configuration files
    BESFSDir fsd(newdir, allfiles.getFileName());
    BESFSDir::fileIterator i = fsd.beginOfFileList();
    BESFSDir::fileIterator e = fsd.endOfFileList();
    for (; i != e; i++) {
        string include_file = (*i).getFullPath();
        load_include_file(include_file, loaded_kvp_files, keystore);
    }
}

void set_key(const string &key, const string &val, bool addto,
             std::unordered_map<std::string, std::vector<std::string>> &keystore) {

    auto i = keystore.find(key);
    if (i == keystore.end()) {
        vector<string> vals;
        keystore[key] = vals;
    }
    if (!addto)
        keystore[key].clear();
    if (!val.empty()) {
        keystore[key].push_back(val);
    }
}

void load_keys(const string &current_keys_file_name, std::ifstream &keys_file, set<string> &loaded_kvp_files,
               std::unordered_map<std::string, std::vector<std::string>> &keystore) {

    string key, value, line;
    while (!keys_file.eof()) {
        bool addto = false;
        getline(keys_file, line);
        if (break_pair(line.c_str(), key, value, addto)) {
            if (key == BES_INCLUDE_KEY) {
                // We make this call to set_key() and force 'addto' to
                // be true because we need access to the child configuration
                // files and their values for the admin interface.
                set_key(key, value, true, keystore);
                // load_include_files(kvp_files, value, keystore);
                load_include_files(current_keys_file_name, value, loaded_kvp_files, keystore);
            } else {
                set_key(key, value, addto, keystore);
            }
        }
    }
}

void load_keys(const std::string &keys_file_name, set<string> &loaded_kvp_files,
               std::unordered_map<std::string, std::vector<std::string>> &keystore) {
    std::ifstream keys_file(keys_file_name.c_str());

    if (!keys_file) {
        char path[500];
        getcwd(path, sizeof(path));
        string s = string("Cannot open configuration file '") + keys_file_name + "': ";
        const char *err = strerror(errno);
        if (err)
            s += err;
        else
            s += "Unknown error";

        s += (string) ".\n" + "The current working directory is " + path;
        throw BESInternalFatalError(s, __FILE__, __LINE__);
    }

    try {
        loaded_kvp_files.insert(keys_file_name);
        load_keys(keys_file_name, keys_file, loaded_kvp_files, keystore);
    } catch (const BESError &e) {
        throw BESInternalFatalError(e.get_message(), e.get_file(), e.get_line());
    } catch (const std::exception &e) {
        string s = (string) "Caught exception load keys from the BES configuration file '" + keys_file_name +
                   "' message:" + e.what();
        throw BESInternalFatalError(s, __FILE__, __LINE__);
    }
}

void load_keys(const std::string &keys_file_name, std::unordered_map<std::string, std::vector<std::string>> &keystore) {
    set<string> loaded_kvp_files;
    // FIXME: Don't make this just to throw it away. jhrg 2/2/23
    load_keys(keys_file_name, loaded_kvp_files, keystore);
}

} // namespace kvp
