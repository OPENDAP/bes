//
// Created by ndp on 12/13/19.
//

#include "config.h"

#include <cerrno>
#include <cstring>

#include <string>
#include <sstream>
#include <set>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "BESUtil.h"
#include "BESFSDir.h"
#include "BESFSFile.h"
#include "BESInternalFatalError.h"
#include "BESSyntaxUserError.h"

#include "kvp_utils.h"


#define BES_INCLUDE_KEY "BES.Include"

using namespace std;

// Forward declaration, implementation at end of file..
void load_keys(
        set<string> &loaded_kvp_files,
        const std::string &keys_file_name,
        std::map<std::string, std::vector<std::string> > &keystore);

bool only_blanks(const char *line)
{
    string my_line = line;
    if (my_line.find_first_not_of(" ") != string::npos)
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
inline bool break_pair(const char* b, string& key, string &value, bool &addto)
{
    addto = false;
    // Ignore comments and lines with only spaces
    if (b && (b[0] != '#') && (!only_blanks(b))) {
        register size_t l = strlen(b);
        if (l > 1) {
            int pos = 0;
            bool done = false;
            for (register size_t j = 0; j < l && !done; j++) {
                if (b[j] == '=') {
                    if (!addto)
                        pos = j;
                    else {
                        if (pos != static_cast<int>(j - 1)) {
                            string s = string("BES: Invalid entry ") + b + " in configuration file "// + _keys_file_name
                                       + " '+' character found in variable name" + " or attempting '+=' with space"
                                       + " between the characters.\n";
                            throw BESInternalFatalError(s, __FILE__, __LINE__);
                        }
                    }
                    done = true;
                }
                else if (b[j] == '+') {
                    addto = true;
                    pos = j;
                }
            }
            if (!done) {
                string s = string("BES: Invalid entry ") + b + " in configuration file " //+ _keys_file_name + ": "
                           + " '=' character not found.\n";
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
void load_include_file(
        set<string> &loaded_kvp_files,
        const string &file,
        std::map<std::string, std::vector<std::string> > &keystore
        ){
    // make sure the file exists and is readable
    // throws exception if unable to read
    // not loaded if has already be started to be loaded
    set<string>::iterator it = loaded_kvp_files.find(file);

    if (it == loaded_kvp_files.end()) {
        // Didn't find it, better load it...
        loaded_kvp_files.insert(file);
        load_keys(loaded_kvp_files, file, keystore);
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
 * patter for 1 or more files
 * @param keystore The map into which the key value pairs will be placed.
*/
void load_include_files(
        set<string> &loaded_kvp_files,
        const string &file_expr,
        std::map<std::string, std::vector<std::string> > &keystore,
        const string &current_keys_file_name
        ){
    string newdir;
    BESFSFile allfiles(file_expr);

    // If the files specified begin with a /, then use that directory
    // instead of the current keys file directory.
    if (!file_expr.empty() && file_expr[0] == '/') {
        newdir = allfiles.getDirName();
    }
    else {
        // determine the directory of the current keys file. All included
        // files will be relative to this file.
        BESFSFile currfile(current_keys_file_name);
        string currdir = currfile.getDirName();

        string alldir = allfiles.getDirName();

        if ((currdir == "./" || currdir == ".") && (alldir == "./" || alldir == ".")) {
            newdir = "./";
        }
        else {
            if (alldir == "./" || alldir == ".") {
                newdir = currdir;
            }
            else {
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
        string include_file =  (*i).getFullPath();
        load_include_file(loaded_kvp_files, include_file, keystore);
    }
}

void set_key(
        const string &key,
        const string &val,
        bool addto,
        std::map<std::string, std::vector<std::string> > &keystore)
{
    map<string, vector<string> >::iterator i;
    i = keystore.find(key);
    if (i == keystore.end()) {
        vector<string> vals;
        keystore[key] = vals;
    }
    if (!addto) keystore[key].clear();
    if (!val.empty()) {
        keystore[key].push_back(val);
    }
}

void load_keys(
        set<string> &loaded_kvp_files,
        std::ifstream *keys_file,
        std::map<std::string, std::vector<std::string> > &keystore,
        const string &current_keys_file_name ){

    string key, value, line;
    while (!keys_file->eof()) {
        bool addto = false;
        getline(*keys_file, line);
        if (break_pair(line.c_str(), key, value, addto)) {
            if (key == BES_INCLUDE_KEY) {
                // We make this call to set_key() and force 'addto' to
                // be true because we need access to the child configuration
                // files and their values for the admin interface.
                set_key(key, value, true, keystore);
                //load_include_files(kvp_files, value, keystore);
                load_include_files(loaded_kvp_files, value, keystore, current_keys_file_name);
            }
            else {
                set_key(key, value, addto, keystore);
            }
        }
    }
}

void load_keys(
        set<string> &loaded_kvp_files,
        const std::string &keys_file_name,
        std::map<std::string, std::vector<std::string> > &keystore
){
    std::ifstream *keys_file = new ifstream(keys_file_name.c_str());

    if (!(*keys_file)) {
        char path[500];
        getcwd(path, sizeof(path));
        string s = string("Cannot open configuration file '") + keys_file_name + "': ";
        char *err = strerror(errno);
        if (err)
            s += err;
        else
            s += "Unknown error";

        s += (string) ".\n" + "The current working directory is " + path;
        throw BESInternalFatalError(s, __FILE__, __LINE__);
    }

    try {
        loaded_kvp_files.insert(keys_file_name);
        load_keys(loaded_kvp_files, keys_file, keystore, keys_file_name);
    }
    catch (BESError &e) {
        // be sure we're throwing a fatal error, since the BES can't run
        // without the configuration file
        //clean();
        throw BESInternalFatalError(e.get_message(), e.get_file(), e.get_line());
    }
    catch (...) {
        //clean();
        string s = (string) "Undefined exception while trying to load keys from the BES configuration file '"
                            + keys_file_name + "'";
        throw BESInternalFatalError(s, __FILE__, __LINE__);
    }
}




void load_keys(
        const std::string &keys_file_name,
        std::map<std::string, std::vector<std::string> > &keystore
){
    set<string> loaded_kvp_files;
    load_keys(loaded_kvp_files, keys_file_name, keystore);
}

