// TheBESKeys.cc

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

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <cerrno>
#include <cstring>

#include <string>
#include <vector>
#include <map>
#include <sstream>

#include "BESDebug.h"
#include "TheBESKeys.h"
#include "kvp_utils.h"
#include "BESUtil.h"
#include "BESRegex.h"
#include "BESFSDir.h"
#include "BESFSFile.h"
#include "BESInternalFatalError.h"
#include "BESInternalError.h"
#include "BESSyntaxUserError.h"
#include "BESLog.h"

#define BES_INCLUDE_KEY "BES.Include"

using namespace std;

#define MODULE "bes"
#define prolog std::string("TheBESKeys::").append(__func__).append("() - ")

set<string> TheBESKeys::d_ingested_key_files;

TheBESKeys *TheBESKeys::d_instance = 0;
string TheBESKeys::ConfigFile = "";

TheBESKeys *TheBESKeys::TheKeys()
{
    if (d_instance) return d_instance;

    if (!TheBESKeys::ConfigFile.empty()) {
        d_instance = new TheBESKeys(TheBESKeys::ConfigFile);
        return d_instance;
    }

    // d_instance is a nullptr and TheBESKeys::ConfigFile is ""
    // so lets try some obvious places...

    string try_ini = "/usr/local/etc/bes/bes.conf";
    if (access(try_ini.c_str(), R_OK) == 0) {
        TheBESKeys::ConfigFile = try_ini;
        d_instance = new TheBESKeys(TheBESKeys::ConfigFile);
        return d_instance;
    }

    try_ini = "/etc/bes/bes.conf";
    if (access(try_ini.c_str(), R_OK) == 0) {
        TheBESKeys::ConfigFile = try_ini;
        d_instance = new TheBESKeys(TheBESKeys::ConfigFile);
        return d_instance;
    }

    try_ini = "/usr/etc/bes/bes.conf";
    if (access(try_ini.c_str(), R_OK) == 0) {
        TheBESKeys::ConfigFile = try_ini;
        d_instance = new TheBESKeys(TheBESKeys::ConfigFile);
    return d_instance;
}
    throw BESInternalFatalError("Unable to locate a BES configuration file.", __FILE__, __LINE__);
}

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
TheBESKeys::TheBESKeys(const string &keys_file_name) :
        d_keys_file_name(keys_file_name), d_the_keys(0), d_the_original_keys(0), d_dynamic_config_in_use(false), d_own_keys(true)
{
    d_the_keys = new map<string, vector<string> >;
    d_the_original_keys = new map<string, vector<string> >;
    initialize_keys();
}

#if 0
TheBESKeys::TheBESKeys(const string &keys_file_name, map<string, vector<string> > *keys) :
        d_keys_file_name(keys_file_name), d_the_keys(keys), d_the_original_keys(0), d_dynamic_config_in_use(false), d_own_keys(false)
{
    initialize_keys();
}
#endif

/** @brief cleans up the key/value pair mapping
 */
TheBESKeys::~TheBESKeys()
{
    clean();
}

void TheBESKeys::initialize_keys()
{
    kvp::load_keys(d_keys_file_name, d_ingested_key_files, *d_the_keys);
    *d_the_original_keys = *d_the_keys;
    BESDEBUG(MODULE, prolog << "          d_keys_file_name: " << d_keys_file_name << endl);
    BESDEBUG(MODULE, prolog << "         d_the_keys.size(): " << d_the_keys->size() << endl);
    BESDEBUG(MODULE, prolog << "d_the_original_keys.size(): " << d_the_original_keys->size() << endl);
}

void TheBESKeys::clean()
{

    if (d_the_keys && d_own_keys) {
        delete d_the_keys;
        d_the_keys = 0;
    }
    if(d_the_original_keys){
        delete d_the_original_keys;
        d_the_original_keys = 0;
    }
}

/** @brief Determine if the specified key file has been loaded yet
 *
 * Given the name of the key file, determine if it has already been
 * loaded. More specifically, if started to load the file.
 *
 * @returns true if already started to load, false otherwise
 */
bool TheBESKeys::LoadedKeys(const string &key_file)
{
#if 0
    vector<string>::const_iterator i = TheBESKeys::d_ingested_key_files.begin();
    vector<string>::const_iterator e = TheBESKeys::d_ingested_key_files.end();
    for (; i != e; i++) {
        if ((*i) == key_file) {
            return true;
        }
    }
#endif
    set<string>::iterator it = d_ingested_key_files.find(key_file);

    return it != d_ingested_key_files.end();
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
void TheBESKeys::set_key(const string &key, const string &val, bool addto)
{
    map<string, vector<string> >::iterator i;
    i = d_the_keys->find(key);
    if (i == d_the_keys->end()) {
        vector<string> vals;
        (*d_the_keys)[key] = vals;
    }
    if (!addto) (*d_the_keys)[key].clear();
    if (!val.empty()) {
        (*d_the_keys)[key].push_back(val);
    }
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
 * @param values A collection of values to to associate with the key
 * @param addto Specifies whether to append the values to the key or set the
 * value. Default is to set, not append to
 */
void TheBESKeys::set_keys(const string &key, const vector<string> &values, bool addto)
{
    map<string, vector<string> >::iterator i;
    i = d_the_keys->find(key);
    if (i == d_the_keys->end()) {
        vector<string> vals;
        (*d_the_keys)[key] = vals;
    }
    if (!addto) (*d_the_keys)[key].clear();

    size_t j;
    for(j = 0; j!=values.size(); j++){
        if (!values[j].empty()) {
            (*d_the_keys)[key].push_back(values[j]);
        }
    }
}


/** @brief allows the user to encode a map in the Keys from within the application.
 *
 * This method allows users of BESKeys to set the value of a kep to be a map
 * of key value pairs from within the
 * application, such as for testing purposes, key/value pairs from the command
 * line, etc...
 *
 * If addto is set to true then the value is added to the list of values for key
 *
 * If addto is false, and the key is already set then this value
 * replaces all values for the key
 *
 * @param key name of the key/value pair to be set
 * @param values A map of key value pairs to associate with the key
 * @param addto Specifies whether to append the values to the key or set the
 * value. Default is to set, not append to
 */
void TheBESKeys::set_keys(
        const string &key,
        const map<string, string> &values,
        const bool case_insensitive_map_keys,
        bool addto)
{
    map<string, vector<string> >::iterator i;
    i = d_the_keys->find(key);
    if (i == d_the_keys->end()) {
        vector<string> vals;
        (*d_the_keys)[key] = vals;
    }
    if (!addto) {
        (*d_the_keys)[key].clear();
    }

    map<string, string>::const_iterator mit;
    for(mit = values.begin(); mit!=values.end(); mit++){
        string map_key = mit->first;
        if(map_key.empty() ){
            BESDEBUG(MODULE, prolog << "The map_key is empty. SKIPPING." << endl);
        }
        else {
            if(case_insensitive_map_keys){
                map_key = BESUtil::lowercase(map_key);
            }
            string map_record=map_key+":"+mit->second;
            (*d_the_keys)[key].push_back(map_record);
        }
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
void TheBESKeys::set_key(const string &pair)
{
    string key;
    string val;
    bool addto = false;
    kvp::break_pair(pair.c_str(), key, val, addto);
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
void TheBESKeys::get_value(const string &s, string &val, bool &found)
{
    found = false;
    map<string, vector<string> >::iterator i;
    i = d_the_keys->find(s);
    if (i != d_the_keys->end()) {
        found = true;
        if ((*i).second.size() > 1) {
            string err = string("Multiple values for the key ") + s + " found, should only be one.";
            throw BESInternalError(err, __FILE__, __LINE__);
        }
        if ((*i).second.size() == 1) {
            val = (*i).second[0];
        }
        else {
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
void TheBESKeys::get_values(const string& s, vector<string> &vals, bool &found)
{
    found = false;
    map<string, vector<string> >::iterator i;
    i = d_the_keys->find(s);
    if (i != d_the_keys->end()) {
        found = true;
        vector<string>::iterator j;
        for(j=(*i).second.begin(); j!=(*i).second.end(); j++){
            vals.push_back(*j);
        }
        // vals = (*i).second; // BUT WHY NOT?
    }
}

/**
 * @brief Read a boolean-valued key from the bes.conf file
 *
 * Look-up the bes key \arg key and return its value if set. If the
 * key is not set, return the default value.
 *
 * @param key The key to loop up
 * @param default_value Return this value if \arg key is not found.
 * @return The boolean value of \arg key. The value of the key is true if the
 * key is set to "true", "yes", or "on", otherwise the key value is
 * interpreted as false. If \arg key is not set, return \arg default_value.
 */
bool TheBESKeys::read_bool_key(const string &key, bool default_value)
{
    bool found = false;
    string value;
    TheBESKeys::TheKeys()->get_value(key, value, found);
    // 'key' holds the string value at this point if key_found is true
    if (found) {
        value = BESUtil::lowercase(value);
        return (value == "true" || value == "yes"|| value == "on");
    }
    else {
        return default_value;
    }
 }

/**
 * @brief Read a string-valued key from the bes.conf file.
 *
 * Look-up the bes key \arg key and return its value if set. If the
 * key is not set, return the default value.
 *
 * @param key The key to loop up
 * @param default_value Return this value if \arg key is not found.
 * @return The string value of \arg key.
 */
string TheBESKeys::read_string_key(const string &key, const string &default_value)
{
    bool found = false;
    string value;
    TheBESKeys::TheKeys()->get_value(key, value, found);
    // 'value' holds the string value at this point if found is true
    if (found) {
        if (value[value.length() - 1] == '/') value.erase(value.length() - 1);
        return value;
    }
    else {
        return default_value;
    }
}

/**
 * @brief Read an integer-valued key from the bes.conf file.
 *
 * Look-up the bes key \arg key and return its value if set. If the
 * key is not set, return the default value.
 *
 * @param key The key to loop up
 * @param default_value Return this value if \arg key is not found.
 * @return The integer value of \arg key.
 */
int TheBESKeys::read_int_key(const string &key, int default_value)
{
    bool found = false;
    string value;
    TheBESKeys::TheKeys()->get_value(key, value, found);
    // 'key' holds the string value at this point if found is true
    if (found) {
        std::istringstream iss(value);
        int int_val;
        iss >> int_val;
        if (iss.eof() || iss.bad() || iss.fail())
            return default_value;
        else
            return int_val;
    }
    else {
        return default_value;
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with all of the keys.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void TheBESKeys::dump(ostream &strm) const
{
    strm << dump();
}

/** @brief dumps information about this object
 *
 * @return A string containing the dump from this instance of TheBESKeys object.
 *
 */
string TheBESKeys::dump() const
{
    stringstream ss;
    ss << BESIndent::LMarg << "BESKeys::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    ss << BESIndent::LMarg << "key file:" << d_keys_file_name << endl;

#if 0
    if (_keys_file && *_keys_file) {
        strm << BESIndent::LMarg << "key file is valid" << endl;
    }
    else {
        strm << BESIndent::LMarg << "key file is NOT valid" << endl;
    }
#endif

    if (d_the_keys && d_the_keys->size()) {
        ss << BESIndent::LMarg << "  keys:" << endl;
        BESIndent::Indent();
        Keys_citer i = d_the_keys->begin();
        Keys_citer ie = d_the_keys->end();
        for (; i != ie; i++) {
            ss << BESIndent::LMarg << (*i).first << ": " /*<< endl*/;
            // BESIndent::Indent();
            vector<string>::const_iterator v = (*i).second.begin();
            vector<string>::const_iterator ve = (*i).second.end();
            for (; v != ve; v++) {
                ss << (*v) << " "; //endl;
            }
            ss << endl;
            //BESIndent::UnIndent();
        }
        BESIndent::UnIndent();
    }
    else {
        ss << BESIndent::LMarg << "keys: none" << endl;
    }
    BESIndent::UnIndent();
    return ss.str();
}



#define MAP_SEPARATOR ":"

bool parse_map_record(const string &map_record, const bool &case_insensitive_map_keys, string &key, string &value) {
    int primary_index = map_record.find(MAP_SEPARATOR);
    if (primary_index > 0) {
        key = map_record.substr(0, primary_index);
        if (case_insensitive_map_keys)
            key = BESUtil::lowercase(key);
        value = map_record.substr(primary_index + 1);
        BESDEBUG(MODULE, prolog << "key: '" << key << "'  value: " << value << endl);
        return true;
    }
    return false;
}


/**
 *
 * @param key
 * @param map_values
 * @param case_insensitive_map_keys
 * @param found
 */
void TheBESKeys::get_values(
        const std::string &key,
        std::map<std::string,std::string> &map_values,
        const bool &case_insensitive_map_keys,
        bool &found){

    vector<string> values;
    get_values(key, values, found);
    if(!found){
        return;
    }

    vector<string>::iterator it;
    for(it=values.begin();  it!=values.end(); it++){
        string map_key;
        string map_value;
        if(parse_map_record(*it,case_insensitive_map_keys,map_key,map_value)){
            map_values.insert( std::pair<string,string>(map_key,map_value));
        }
        else {
            BESDEBUG(MODULE, prolog << string("The configuration entry for the ") << key << " was not " <<
                "formatted as a map record. The offending entry: " << *it << " HAS BEEN SKIPPED." << endl);
        }
    }

}


/**
 *
 * @param key
 * @param map_values
 * @param case_insensitive_map_keys
 * @param found
 */
void TheBESKeys::get_values(
        const std::string &key,
        std::map< std::string, std::map<std::string,std::vector<std::string> > > &primary_map,
        const bool &case_insensitive_map_keys,
        bool &found){

    BESDEBUG(MODULE, prolog << "BEGIN" << endl);
    vector<string> values;
    get_values(key, values, found);
    if(!found){
        return;
    }

    vector<string>::iterator it;
    for(it=values.begin();  it!=values.end(); it++){
        string map_record = *it;
        string primary_map_key;
        string primary_map_value;
        if(parse_map_record(map_record,case_insensitive_map_keys,primary_map_key,primary_map_value)){
            string secondary_key;
            string secondary_value;
            if(parse_map_record(primary_map_value,case_insensitive_map_keys,secondary_key,secondary_value)){
                map<string, map<string,vector<string>>>::iterator pit;
                pit = primary_map.find(primary_map_key);
                if(pit!=primary_map.end()){
                    map<string,vector<string>>::iterator sit;
                    sit = pit->second.find(secondary_key);
                    if(sit!=pit->second.end()){
                        sit->second.push_back(secondary_value);
                    }
                    else {
                        // How to make a vector<string>> and poke in to the secondary_map??
                        vector<string> secondary_map_entry_values;
                        secondary_map_entry_values.push_back(secondary_value);
                        pit->second.insert(pair<string,vector<string>>(secondary_key,secondary_map_entry_values));
                    }
                }
                else {
                    // How to make a map<string,vector<string>> and poke in to the primary_map??
                    map<string,vector<string>> secondary_map_entry;
                    vector<string> secondary_map_entry_values;
                    secondary_map_entry_values.push_back(secondary_value);
                    secondary_map_entry.insert(pair<string,vector<string>>(secondary_key,secondary_map_entry_values));
                    primary_map.insert(pair<string, map<string,vector<string>>>(primary_map_key,secondary_map_entry));
                }
            }
            else {
                // Map entry improperly formatted.
                BESDEBUG(MODULE, prolog << string("The configuration entry for the ") << key << " was not " <<
                                        "formatted as a map record. The offending entry: " << map_record <<
                                        " HAS BEEN SKIPPED." << endl);
            }
        }
        else {
            BESDEBUG(MODULE, prolog << string("The configuration entry for the ") << key << " was not " <<
                                    "formatted as a map record. The offending entry: " << map_record <<
                                    " HAS BEEN SKIPPED." << endl);
        }
    }
    BESDEBUG(MODULE, prolog << "END" << endl);

}

bool TheBESKeys::using_dynamic_config(){
    return d_dynamic_config_in_use;
}


/**
 * @brief Loads the the applicable dynamic configuration or nothing if no configuration is applicable.
 * @param name
 */
void TheBESKeys::load_dynamic_config(const string name)
{
#if DYNAMIC_CONFIG_ENABLED

    BESDEBUG(MODULE, prolog << "BEGIN" << endl);

    // Clear the active keys and copy the original keys into
    // the active keys (resets the keys to 'as read from config files')
    if( d_dynamic_config_in_use ){
        BESDEBUG(MODULE, prolog << "Unloading DynamicConfig." << endl);
        d_the_keys->clear();
        *d_the_keys = *d_the_original_keys;
        d_dynamic_config_in_use =  false;
    }

    map<string, map<string, vector<string>>> dynamic_confg;
    bool found;
    get_values(DYNAMIC_CONFIG_KEY, dynamic_confg, true, found);
    if(!found){
        BESDEBUG(MODULE, prolog << "Unable to locate " << DYNAMIC_CONFIG_KEY
        << " in the configuration keys." << endl);
        return;
    }
    BESDEBUG(MODULE, prolog << "Found a " << DYNAMIC_CONFIG_KEY << " in TheBESKeys." << endl);

    string best_matching_config_name;
    long longest_match=0;
    map<string, map<string, vector<string>>>::iterator best_matching_config=dynamic_confg.end();

    map<string, map<string, vector<string>>>::iterator dcit;
    for(dcit = dynamic_confg.begin(); dcit != dynamic_confg.end(); dcit++){
        BESDEBUG(MODULE, prolog << "Processing " << DYNAMIC_CONFIG_KEY << "["<<dcit->first<< "]" << endl);

        map<string, vector<string>>::iterator rit;
        rit = dcit->second.find(DC_REGEX_KEY);
        if(rit==dcit->second.end()){
            BESDEBUG(MODULE, prolog << "Could not find a " << DC_REGEX_KEY << " (regular expression) for the "
            << DYNAMIC_CONFIG_KEY << " named: " << dcit->first << " SKIPPING!" << endl);
        }
        else {
            BESDEBUG(MODULE, prolog << "Found " << DC_REGEX_KEY << " vector for "
            << DYNAMIC_CONFIG_KEY << "["<< dcit->first << "]" << endl);
            vector<string>::iterator vit;
            for(vit = rit->second.begin(); vit != rit->second.end(); vit ++){ // For all the regex expressions
                BESDEBUG(MODULE, prolog << "Processing " << DC_REGEX_KEY << " value '" << *vit << "'" << endl);
                BESRegex regex((*vit).c_str()); // make BESRegex
                long match_length = regex.match(name.c_str(),name.size(),0); // Eval match

                BESDEBUG(MODULE, prolog << "The name '"<< name << (match_length<0?"' does not match ":"' matches ")
                << "the regular expression: '"<< *vit << "' (match_length: " << match_length << ")" << endl);
                if(match_length>longest_match){ // Is is a better match?
                    BESDEBUG(MODULE, prolog << "match_length of " << match_length
                    << " is larger than the current longest_match of "<< longest_match << endl);

                    map<string, vector<string>>::iterator cit;
                    cit = dcit->second.find(DC_CONFIG_KEY);
                    if(cit==dcit->second.end() || cit->second.empty()){ // does it have a config?
                        BESDEBUG(MODULE, prolog << "There were no " << DC_CONFIG_KEY
                        << " (configuration) values for the " << DYNAMIC_CONFIG_KEY << " named: "
                        << dcit->first << " SKIPPING!" << endl);
                    }
                    else {

                        best_matching_config = dcit;
                        longest_match = match_length;
                        best_matching_config_name = dcit->first;
                        BESDEBUG(MODULE, prolog << "Found new best " << DYNAMIC_CONFIG_KEY << " match for '" << name
                        << "' " << DYNAMIC_CONFIG_KEY << ": " << best_matching_config_name << endl);
                    }
                }
            }
        }
    }

    if( longest_match==0 || best_matching_config==dynamic_confg.end() ){
        BESDEBUG(MODULE, prolog << "None of the " << DYNAMIC_CONFIG_KEY
        << " regex patterns matched the name: " << name << endl);
        return;
    }

    {
        stringstream msg;
        msg << prolog << "Using " << DYNAMIC_CONFIG_KEY << ":" << best_matching_config_name << " for: " << name << endl;
        BESDEBUG(MODULE, msg.str());
        INFO_LOG( msg.str());
    }

    // Now load the specific keys from the dynamic config;
    map<string, vector<string>>::iterator cit;
    cit = best_matching_config->second.find(DC_CONFIG_KEY);
    vector<string>::iterator vit;
    for(vit=cit->second.begin(); vit != cit->second.end(); vit++){
        // Each value of this vector should be a regular BESKeys kvp. i.e. "BES.LogName=./opendap.log"
        // Which we just feed into the keys, since we just backed them up...
        BESDEBUG(MODULE, prolog << "Adding dynamic configuration BES Key: " << *vit << endl);
        set_key(*vit);
    }
    d_dynamic_config_in_use = true;

    BESDEBUG(MODULE, prolog << "END" << endl);
#endif

    BESDEBUG("bes:keys",dump());

}

