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
#include <atomic>

class BESUtil {
private:
    static std::string rfc822_date(time_t t);

    static std::string entity(char c);

public:
    static long get_current_memory_usage() noexcept;

    static void trim_if_trailing_slash(std::string &value);
    static void trim_if_surrounding_quotes(std::string &value);

    /** These functions are used to create the MIME headers for a message
     from a server to a client.

     NB: These functions actually write both the response status line
     <i>and</i> the header.

     @name MIME utility functions
     @see libdap::escaping.cc
     @see libdap::mime_util.cc
     */
    static void set_mime_text(std::ostream &strm);
    static void set_mime_html(std::ostream &strm);

    /** This functions are used to unescape hex characters from strings **/
    static std::string www2id(const std::string &in, const std::string &escape = "%", const std::string &except = "");
    static std::string unhexstring(const std::string& s);

    /** Convert a string to all lower case **/
    static std::string lowercase(const std::string &s);

    /** Unescape characters with backslash before them **/
    static std::string unescape(const std::string &s);

    /** Check if the specified path is valid **/
    static void check_path(const std::string &path, const std::string &root, bool follow_sym_links);

    /** convert pid and place in provided buffer **/
    static char * fastpidconverter(char *buf, int base);
    static char * fastpidconverter(long val, char *buf, int base);

    /** remove leading and trailing blanks from a string **/
    static void removeLeadingAndTrailingBlanks(std::string &key);

    /** convert characters not allowed in xml to escaped characters **/
    static std::string id2xml(std::string in, const std::string &not_allowed = "><&'\"");

    /** unescape xml escaped characters **/
    static std::string xml2id(std::string in);

    /** explode a string into an array given a delimiter **/
    static void explode(char delim, const std::string &str, std::list<std::string> &values);

    /** implode a list of values into a single string delimited by delim **/
    static std::string implode(const std::list<std::string> &values, char delim);

    struct url {
    	std::string protocol;
    	std::string domain;
    	std::string uname;
        std::string psswd;
        std::string port;
        std::string path;
    };

    static void url_explode(const std::string &url_str, BESUtil::url &url_parts);
    static std::string url_create(BESUtil::url &url_parts);
    // static string assemblePath(const string &firstPart, const string &secondPart, bool leadingSlash = false);
    static std::string assemblePath(const std::string &firstPart, const std::string &secondPart, bool leadingSlash = false, bool trailingSlash = false);
    static std::string pathConcat(const std::string &firstPart, const std::string &secondPart, char separator='/');

    static bool endsWith(std::string const &fullString, std::string const &ending);
    static void conditional_timeout_cancel();
    static void exit_on_request_timeout();

    /** Convert a string to all lower case **/
    static unsigned int replace_all(std::string &s, std::string find_this, std::string replace_with_this);
    static std::string normalize_path(const std::string &path, bool leading_separator, bool trailing_separator, std::string separator = "/");
    static void tokenize(const std::string& str, std::vector<std::string>& tokens, const std::string& delimiters = "/");
    static std::string get_time(bool use_local_time = false);
    static std::string get_time(time_t the_time, bool use_local_time = false);
    static std::vector<std::string> split(const std::string &s, char delim='/', bool skip_empty=true);

    static BESCatalog *separateCatalogFromPath(std::string &path);

    static uint64_t file_to_stream(const std::string &file_name, std::ostream &o_strm, uint64_t read_start_position=0);

    static std::string get_dir_name(const std::string &p);
    static bool is_directory(const std::string &p);

    static int mkdir_p(const std::string &path, mode_t mode);
    static std::string file_to_string(const std::string &filename);
    static std::string file_to_string(const std::string &filename, std::string &error_msg);
    static int make_temp_file(const std::string &temp_file_dir, std::string &temp_file_name);
    static void string_to_file(const std::string &filename, const std::string &content);

    static std::string &remove_crlf(std::string &str);

};

#endif // E_BESUtil_h
