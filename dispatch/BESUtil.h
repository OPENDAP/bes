// BESUtil.h

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

#ifndef E_BESUtil_h
#define E_BESUtil_h 1

#include <string>
#include <list>
#include <iostream>
#include <vector>
#include <BESCatalog.h>

using std::string;
using std::list;
using std::ostream;

class BESUtil {
private:
    static string rfc822_date(const time_t t);

    static string entity(char c);
public:
    /** These functions are used to create the MIME headers for a message
     from a server to a client.

     NB: These functions actually write both the response status line
     <i>and</i> the header.

     @name MIME utility functions
     @see libdap::escaping.cc
     @see libdap::mime_util.cc
     */
    static void set_mime_text(ostream &strm);
    static void set_mime_html(ostream &strm);

    /** This functions are used to unescape hex characters from strings **/
    static string www2id(const string &in, const string &escape = "%", const string &except = "");
    static string unhexstring(string s);

    /** Convert a string to all lower case **/
    static string lowercase(const string &s);

    /** Unescape characters with backslash before them **/
    static string unescape(const string &s);

    /** Check if the specified path is valid **/
    static void check_path(const string &path, const string &root, bool follow_sym_links);

    /** convert pid and place in provided buffer **/
    static char * fastpidconverter(char *buf, int base);
    static char * fastpidconverter(long val, char *buf, int base);

    /** remove leading and trailing blanks from a string **/
    static void removeLeadingAndTrailingBlanks(string &key);

    /** convert characters not allowed in xml to escaped characters **/
    static string id2xml(string in, const string &not_allowed = "><&'\"");

    /** unescape xml escaped characters **/
    static string xml2id(string in);

    /** explode a string into an array given a delimiter **/
    static void explode(char delim, const string &str, list<string> &values);

    /** implode a list of values into a single string delimited by delim **/
    static string implode(const list<string> &values, char delim);

    struct url {
        string protocol;
        string domain;
        string uname;
        string psswd;
        string port;
        string path;
    };

    static void url_explode(const string &url_str, BESUtil::url &url_parts);
    static string url_create(BESUtil::url &url_parts);
    // static string assemblePath(const string &firstPart, const string &secondPart, bool leadingSlash = false);
    static string assemblePath(const string &firstPart, const string &secondPart, bool leadingSlash = false, bool trailingSlash = false);
    static string pathConcat(const string &firstPart, const string &secondPart, char separator='/');

    static bool endsWith(std::string const &fullString, std::string const &ending);
    static void conditional_timeout_cancel();

    static void replace_all(std::string &s, std::string find_this, std::string replace_with_this);
    static std::string normalize_path(const std::string &path, bool leading_separator, bool trailing_separator, const string separator = "/");
    static void tokenize(const std::string& str, std::vector<std::string>& tokens, const std::string& delimiters = "/");
    static string get_time(bool use_local_time = false);
    static string get_time(time_t the_time, bool use_local_time = false);
    static std::vector<std::string> split(const string &s, char delim='/', bool skip_empty=true);

    static BESCatalog *separateCatalogFromPath(std::string &path);


};




#endif // E_BESUtil_h

