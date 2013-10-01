// BESKeys.cc

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

#include "config.h"

#ifdef __cplusplus
extern "C"
{
#include <sys/types.h>
#include "regex.h"
}
#endif

#include <cerrno>
#include <cstring>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "BESKeys.h"
#include "BESUtil.h"
#include "BESFSDir.h"
#include "BESFSFile.h"
#include "BESInternalFatalError.h"
#include "BESSyntaxUserError.h"

#define BES_INCLUDE_KEY "BES.Include"

vector<string> BESKeys::KeyList;

/** @brief default constructor that reads loads key/value pairs from the
 * specified file.
 *
 * This constructor uses the specified file to load key/value pairs.
 * This file holds different key/value pairs for the application, one
 * key/value pair per line separated by an equal (=) sign.
 *
 * key=value
 *
 * Comments are allowed in the file and must begin with a pound (#) sign at
 * the beginning of the line. No comments are allowed at the end of lines.
 *
 * @throws BESInternalFatalError thrown if there is an error reading the
 * initialization file or a syntax error in the file, i.e. a malformed
 * key/value pair.
 */
BESKeys::BESKeys(const string &keys_file_name) :
    _keys_file(0), _keys_file_name(keys_file_name), _the_keys(0), _own_keys(true)
{
    _the_keys = new map<string, vector<string> > ;
    initialize_keys();
}

BESKeys::BESKeys(const string &keys_file_name, map<string, vector<string> > *keys) :
    _keys_file(0), _keys_file_name(keys_file_name), _the_keys(keys), _own_keys(false)
{
    initialize_keys();
}

/** @brief cleans up the key/value pair mapping
 */
BESKeys::~BESKeys()
{
    clean();
}

void BESKeys::initialize_keys()
{
    _keys_file = new ifstream(_keys_file_name.c_str());
    int myerrno = errno;
    if (!(*_keys_file))
    {
        char path[500];
        getcwd(path, sizeof(path));
        string s = string("BES: fatal, cannot open BES configuration file ") + _keys_file_name + ": ";
        char *err = strerror(myerrno);
        if (err)
            s += err;
        else
            s += "Unknown error";

        s += (string) ".\n" + "The current working directory is " + path + "\n";
        throw BESInternalFatalError(s, __FILE__, __LINE__);
    }

    try
    {
        load_keys();
    }
    catch (BESError &e)
    {
        // be sure we're throwing a fatal error, since the BES can't run
        // within the configuration file
        clean();
        throw BESInternalFatalError(e.get_message(), e.get_file(), e.get_line());
    }
    catch (...)
    {
        clean();
        string s = (string) "Undefined exception while trying to load keys " + "from bes configuration file " + _keys_file_name;
        throw BESInternalFatalError(s, __FILE__, __LINE__);
    }
}

void BESKeys::clean()
{
    if (_keys_file)
    {
        _keys_file->close();
        delete _keys_file;
    }
    if (_the_keys && _own_keys)
    {
        delete _the_keys;
    }
}

/* @brief Determine if the specified key file has been loaded yet
 *
 * Given the name of the key file, determine if it has already been
 * loaded. More specifically, if started to load the file.
 *
 * @returns true if already started to load, false otherwise
 */
bool BESKeys::LoadedKeys(const string &key_file)
{
    vector<string>::const_iterator i = BESKeys::KeyList.begin();
    vector<string>::const_iterator e = BESKeys::KeyList.end();
    for (; i != e; i++)
    {
        if ((*i) == key_file)
        {
            return true;
        }
    }
    return false;
}

void BESKeys::load_keys()
{
    char buffer[255];
    string key, value;
    while (!(*_keys_file).eof())
    {
        if ((*_keys_file).getline(buffer, 255))
        {
            bool addto = false;
            if (break_pair(buffer, key, value, addto))
            {
                if (key == BES_INCLUDE_KEY)
                {
                    // I added this call to set_key() because we need access
                    // to the child config files for the admin interface.
                    // jhrg 5/27/11
                    // Force 'addto' to true; we need all of the include values.
                    // jhrg 6/21/11
                    set_key(key, value, true);
                    load_include_files(value);
                }
                else
                {
                    set_key(key, value, addto);
                }
            }
        }
    }
}

// The string contained in the character buffer b should be of the
// format key=value or key+=value. The pair is broken apart, storing the
// key in the key parameter and the value of the key in the value
// parameter. If += is used, then the value should be added to the value
// of key, not replacing.
//
// It used to be that we would validate the key=value line. Instead,
// anything after the equal sign is considered the value of the key.
inline bool BESKeys::break_pair(const char* b, string& key, string &value, bool &addto)
{
    addto = false;
    // Ignore comments and lines with only spaces
    if (b && (b[0] != '#') && (!only_blanks(b)))
    {
        register size_t l = strlen(b);
        if (l > 1)
        {
            int pos = 0;
            bool done = false;
            for (register size_t j = 0; j < l && !done; j++)
            {
                if (b[j] == '=')
                {
                    if (!addto)
                        pos = j;
                    else
                    {
                        if (pos != static_cast<int> (j - 1))
                        {
                            string s = string("BES: Invalid entry ") + b + " in configuration file " + _keys_file_name
                                    + " '+' character found in variable name" + " or attempting '+=' with space" + " between the characters.\n";
                            throw BESInternalFatalError(s, __FILE__, __LINE__);
                        }
                    }
                    done = true;
                }
                else if (b[j] == '+')
                {
                    addto = true;
                    pos = j;
                }
            }
            if (!done)
            {
                string s = string("BES: Invalid entry ") + b + " in configuration file " + _keys_file_name + ": " + " '=' character not found.\n";
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

/** Load key/value pairs from other files
 *
 * Given a file, or a regular expression matching multiple files, where
 * the location is relative to the location of the current keys file,
 * load the keys from that file into this key list
 *
 * @param files string representing a file or a regular expression
 * patter for 1 or more files
 */
void BESKeys::load_include_files(const string &files)
{
    string newdir;
    BESFSFile allfiles(files);

    // If the files specified begin with a /, then use that directory
    // instead of the current keys file directory.
    if (!files.empty() && files[0] == '/')
    {
        newdir = allfiles.getDirName();
    }
    else
    {
        // determine the directory of the current keys file. All included
        // files will be relative to this file.
        BESFSFile currfile(_keys_file_name);
        string currdir = currfile.getDirName();

        string alldir = allfiles.getDirName();

        if ((currdir == "./" || currdir == ".") && (alldir == "./" || alldir == "."))
        {
            newdir = "./";
        }
        else
        {
            if (alldir == "./" || alldir == ".")
            {
                newdir = currdir;
            }
            else
            {
                newdir = currdir + "/" + alldir;
            }
        }
    }

    // load the files one at a time. If the directory doesn't exist,
    // then don't load any configuration files
    BESFSDir fsd(newdir, allfiles.getFileName());
    BESFSDir::fileIterator i = fsd.beginOfFileList();
    BESFSDir::fileIterator e = fsd.endOfFileList();
    for (; i != e; i++)
    {
        load_include_file((*i).getFullPath());
    }
}

/** Load key/value pairs from one include file
 *
 * Helper function to load key/value pairs from a BES configuration file
 *
 * @param file name of the configuration file to load
 */
void BESKeys::load_include_file(const string &file)
{
    // make sure the file exists and is readable
    // throws exception if unable to read
    // not loaded if has already be started to be loaded
    if (!BESKeys::LoadedKeys(file))
    {
        BESKeys::KeyList.push_back(file);
        BESKeys tmp(file, _the_keys);
    }
}

bool BESKeys::only_blanks(const char *line)
{
    string my_line = line;
    if (my_line.find_first_not_of(" ") != string::npos)
        return false;
    else
        return true;

#if 0
    int val;
    regex_t rx;
    string expr = "[^[:space:]]";
    val = regcomp(&rx, expr.c_str(), REG_ICASE);

    if (val != 0)
    {
        string s = (string) "Regular expression " + expr + " did not compile correctly " + " in configuration file " + _keys_file_name;
        throw BESInternalFatalError(s, __FILE__, __LINE__);
    }
    val = regexec(&rx, line, 0, 0, REG_NOTBOL);
    if (val == 0)
    {
        regfree(&rx);
        return false;
    }
    else
    {
        if (val == REG_NOMATCH)
        {
            regfree(&rx);
            return true;
        }
        else if (val == REG_ESPACE)
        {
            string s = (string) "Execution of regular expression out of space" + " in configuration file " + _keys_file_name;
            throw BESInternalFatalError(s, __FILE__, __LINE__);
        }
        else
        {
            string s = (string) "Execution of regular expression has unknown " + " problem in configuration file " + _keys_file_name;
            throw BESInternalFatalError(s, __FILE__, __LINE__);
        }
    }
#endif
}

/** @brief allows the user to set key/value pairs from within the application.
 *
 * This method allows users of BESKeys to set key/value pairs from within the
 * application, such as for testing purposes, key/value pairs from the command
 * line, etc...
 *
 * If addto is set to true then the value is added to the list of values for key
 *
 * If addto is false, and the key is already set then this value
 * replaces all values for the key
 *
 * @param key name of the key/value pair to be set
 * @param val value of the key to be set
 * @param addto Specifies whether to add the value to the key or set the
 * value. Default is to set, not add to
 */
void BESKeys::set_key(const string &key, const string &val, bool addto)
{
    map<string, vector<string> >::iterator i;
    i = _the_keys->find(key);
    if (i == _the_keys->end())
    {
        vector<string> vals;
        (*_the_keys)[key] = vals;
    }
    if (!addto)
        (*_the_keys)[key].clear();
    if (!val.empty())
    {
        (*_the_keys)[key].push_back(val);
    }
}

/** @brief allows the user to set key/value pairs from within the application.
 *
 * This method allows users of BESKeys to set key/value pairs from within the
 * application, such as for testing purposes, key/value pairs from the command
 * line, etc...
 *
 * If the key is already set then this value replaces the value currently held
 * in the keys map.
 *
 * @param pair the key/value pair passed as key=value
 */
void BESKeys::set_key(const string &pair)
{
    string key;
    string val;
    bool addto = false;
    break_pair(pair.c_str(), key, val, addto);
    set_key(key, val, addto);
}

/** @brief Retrieve the value of a given key, if set.
 *
 * This method allows the user of BESKeys to retrieve the value of the
 * specified key. If multiple values are set then an exception is
 * thrown.
 *
 * @param s The key the user is looking for
 * @param val The value of the key the user is looking for
 * @param found Set to true of the key is set or false if the key is not set.
 * The value of a key can be set to the empty string, which is why this
 * boolean is provided.
 * @throws BESSyntaxUserError if multiple values are available for the
 * specified key
 */
void BESKeys::get_value(const string& s, string &val, bool &found)
{
    found = false;
    map<string, vector<string> >::iterator i;
    i = _the_keys->find(s);
    if (i != _the_keys->end())
    {
        found = true;
        if ((*i).second.size() > 1)
        {
            string err = string("Multiple values for the key ") + s + " found, should only be one.";
            throw BESSyntaxUserError(err, __FILE__, __LINE__);
        }
        if ((*i).second.size() == 1)
        {
            val = (*i).second[0];
        }
        else
        {
            val = "";
        }
    }
}

/** @brief Retrieve the values of a given key, if set.
 *
 * This method allows the user of BESKeys to retrieve the value of the
 * specified key.
 *
 * @param s The key the user is looking for
 * @param vals The value set for the specified key
 * @param found Set to true of the key is set or false if the key is not set.
 * The value of a key can be set to the empty string, which is why this
 * boolean is provided.
 */
void BESKeys::get_values(const string& s, vector<string> &vals, bool &found)
{
    found = false;
    map<string, vector<string> >::iterator i;
    i = _the_keys->find(s);
    if (i != _the_keys->end())
    {
        found = true;
        vals = (*i).second;
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with all of the keys.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESKeys::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESKeys::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "key file:" << _keys_file_name << endl;
    if (_keys_file && *_keys_file)
    {
        strm << BESIndent::LMarg << "key file is valid" << endl;
    }
    else
    {
        strm << BESIndent::LMarg << "key file is NOT valid" << endl;
    }
    if (_the_keys && _the_keys->size())
    {
        strm << BESIndent::LMarg << "    keys:" << endl;
        BESIndent::Indent();
        Keys_citer i = _the_keys->begin();
        Keys_citer ie = _the_keys->end();
        for (; i != ie; i++)
        {
            strm << BESIndent::LMarg << (*i).first << ":" << endl;
            BESIndent::Indent();
            vector<string>::const_iterator v = (*i).second.begin();
            vector<string>::const_iterator ve = (*i).second.end();
            for (; v != ve; v++)
            {
                strm << (*v) << endl;
            }
            BESIndent::UnIndent();
        }
        BESIndent::UnIndent();
    }
    else
    {
        strm << BESIndent::LMarg << "keys: none" << endl;
    }
    BESIndent::UnIndent();
}

