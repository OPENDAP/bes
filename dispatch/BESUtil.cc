// BESUtil.cc

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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>

#include <fcntl.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds
#include <string>     // std::string, std::stol
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <cassert>
#include <vector>
#include <list>
#include <sstream>
#include <iostream>
#include <algorithm>

#include "TheBESKeys.h"
#include "BESUtil.h"
#include "BESDebug.h"
#include "BESForbiddenError.h"
#include "BESNotFoundError.h"
#include "BESInternalError.h"
#include "BESLog.h"
#include "BESCatalogList.h"

#include "BESInternalFatalError.h"
#include "RequestServiceTimer.h"

using namespace std;

#define CRLF "\r\n"

#define MODULE "util"
#define prolog string("BESUtil::").append(__func__).append("() - ")

const string BES_KEY_TIMEOUT_CANCEL = "BES.CancelTimeoutOnSend";

/**
 * @brief Get the Resident Set Size in KB
 * @return The RSS or 0 if getrusage() returns an error
 */
long
BESUtil::get_current_memory_usage() noexcept
{
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) { // getrusage()  successful?
#ifdef __APPLE__
        // get the max size (man page says it is in bytes). This function returns the
        // size in KB like Linux. jhrg 3/29/22
        return usage.ru_maxrss / 1024;
#else
        return usage.ru_maxrss; // get the max size (man page says it is in kilobytes)
#endif
    }
    else {
        return 0;
    }
}

/**
 * @brief If the string ends in a slash, remove it
 * This function works for empty strings (doing nothing). If the string
 * ends in a '/' it will be removed.
 * @note See https://stackoverflow.com/questions/2310939/remove-last-character-from-c-string
 * @param value The string, modified in place
 */
void BESUtil::trim_if_trailing_slash(string &value)
{
    if (!value.empty() && value.back() == '/')
        value.pop_back();   // requires C++-11
    // value.erase(value.end () -1);
}

/**
 * @brief Remove double quotes around a string
 * This function will remove a leading and/or trailing double quote surrounding a
 * string.
 * @param value The string, modified
 */
void BESUtil::trim_if_surrounding_quotes(std::string &value)
{
    if (!value.empty() && value[0] == '"')
        value.erase(0, 1);
    if (!value.empty() && value.back() == '"')
        value.pop_back();   // requires C++-11
    // value.erase(value.end () -1);
}

/** @brief Generate an HTTP 1.0 response header for a text document.

 @param strm Write the MIME header to this ostream.
 */
void BESUtil::set_mime_text(ostream &strm)
{
    strm << "HTTP/1.0 200 OK" << CRLF;
    strm << "XBES-Server: " << PACKAGE_STRING << CRLF;

    const time_t t = time(0);
    strm << "Date: " << rfc822_date(t).c_str() << CRLF;
    strm << "Last-Modified: " << rfc822_date(t).c_str() << CRLF;

    strm << "Content-Type: text/plain" << CRLF;
    // Note that Content-Description is from RFC 2045 (MIME, pt 1), not 2616.
    strm << "Content-Description: unknown" << CRLF;
    strm << CRLF;
}

/** @brief Generate an HTTP 1.0 response header for a html document.

 @param strm Write the MIME header to this ostream.
 */
void BESUtil::set_mime_html(ostream &strm)
{
    strm << "HTTP/1.0 200 OK" << CRLF;
    strm << "XBES-Server: " << PACKAGE_STRING << CRLF;

    const time_t t = time(0);
    strm << "Date: " << rfc822_date(t).c_str() << CRLF;
    strm << "Last-Modified: " << rfc822_date(t).c_str() << CRLF;

    strm << "Content-type: text/html" << CRLF;
    // Note that Content-Description is from RFC 2045 (MIME, pt 1), not 2616.
    strm << "Content-Description: unknown" << CRLF;
    strm << CRLF;
}

// Return a MIME rfc-822 date. The grammar for this is:
//       date-time   =  [ day "," ] date time        ; dd mm yy
//                                                   ;  hh:mm:ss zzz
//
//       day         =  "Mon"  / "Tue" /  "Wed"  / "Thu"
//                   /  "Fri"  / "Sat" /  "Sun"
//
//       date        =  1*2DIGIT month 2DIGIT        ; day month year
//                                                   ;  e.g. 20 Jun 82
//                   NB: year is 4 digit; see RFC 1123. 11/30/99 jhrg
//
//       month       =  "Jan"  /  "Feb" /  "Mar"  /  "Apr"
//                   /  "May"  /  "Jun" /  "Jul"  /  "Aug"
//                   /  "Sep"  /  "Oct" /  "Nov"  /  "Dec"
//
//       time        =  hour zone                    ; ANSI and Military
//
//       hour        =  2DIGIT ":" 2DIGIT [":" 2DIGIT]
//                                                   ; 00:00:00 - 23:59:59
//
//       zone        =  "UT"  / "GMT"                ; Universal Time
//                                                   ; North American : UT
//                   /  "EST" / "EDT"                ;  Eastern:  - 5/ - 4
//                   /  "CST" / "CDT"                ;  Central:  - 6/ - 5
//                   /  "MST" / "MDT"                ;  Mountain: - 7/ - 6
//                   /  "PST" / "PDT"                ;  Pacific:  - 8/ - 7
//                   /  1ALPHA                       ; Military: Z = UT;
//                                                   ;  A:-1; (J not used)
//                                                   ;  M:-12; N:+1; Y:+12
//                   / ( ("+" / "-") 4DIGIT )        ; Local differential
//                                                   ;  hours+min. (HHMM)

static const char *days[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static const char *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

/** Given a constant pointer to a <tt>time_t</tt>, return a RFC
 822/1123 style date.

 This function returns the RFC 822 date with the exception that the RFC
 1123 modification for four-digit years is implemented.

 @return The RFC 822/1123 style date in a C++ string.
 @param t A const <tt>time_t</tt> pointer.
 */
string BESUtil::rfc822_date(const time_t t)
{
    struct tm stm{};
    gmtime_r(&t, &stm);
    char d[256];

    snprintf(d, 255, "%s, %02d %s %4d %02d:%02d:%02d GMT", days[stm.tm_wday], stm.tm_mday,
             months[stm.tm_mon], 1900 + stm.tm_year, stm.tm_hour, stm.tm_min, stm.tm_sec);
    d[255] = '\0';
    return {d};
}

string BESUtil::unhexstring(const string& s)
{
    int val;
    istringstream ss(s);
    ss >> std::hex >> val;
    char tmp_str[2];
    tmp_str[0] = static_cast<char>(val);
    tmp_str[1] = '\0';
    return {tmp_str};
}

// I modified this to mirror the version in libdap. The change allows several
// escape sequences to by listed in 'except'. jhrg 2/18/09
string BESUtil::www2id(const string &in, const string &escape, const string &except)
{
    string::size_type i = 0;
    string res = in;
    while ((i = res.find_first_of(escape, i)) != string::npos) {
        if (except.find(res.substr(i, 3)) != string::npos) {
            i += 3;
            continue;
        }
        res.replace(i, 3, unhexstring(res.substr(i + 1, 2)));
    }

    return res;
}

string BESUtil::lowercase(const string &s)
{
    string return_string = s;
    for (int j = 0; j < static_cast<int>(return_string.size()); j++) {
        return_string[j] = (char) tolower(return_string[j]);
    }

    return return_string;
}

string BESUtil::unescape(const string &s)
{
    bool done = false;
    string::size_type index = 0;
    /* string::size_type new_index = 0 ; */
    string new_str;
    while (!done) {
        string::size_type bs = s.find('\\', index);
        if (bs == string::npos) {
            new_str += s.substr(index, s.size() - index);
            done = true;
        }
        else {
            new_str += s.substr(index, bs - index);
            new_str += s[bs + 1];
            index = bs + 2;
        }
    }

    return new_str;
}

/**
 * @brief convenience routine for check_path() error messages.
 * @param pathname The pathname that failed
 * @param error_number The error number (from errno)
 */
static void throw_access_error(const string &pathname, long error_number)
{
    switch(error_number) {
        case ENOENT:
        case ENOTDIR: {
            string message = string("Failed to locate '").append(pathname).append("'\n");
            INFO_LOG(message);
            throw BESNotFoundError(message, __FILE__, __LINE__);
        }

        default: {
            string message = string("Not allowed to access '").append(pathname).append("'\n");
            INFO_LOG(message);
            throw BESForbiddenError(message, __FILE__, __LINE__);
        }
    }
}

/**
 * @param path Look for symbolic links in this path
 * @param search_limit Search only the first N nodes of path. Used to avoid searching
 * a root component of path that is known to be free of sym links.
 * @return Return true if the any part of the given pathname contains a symbolic link
 */
bool pathname_contains_symlink(const string &path, int search_limit)
{
    // This kludge to remove a trailing '/' is needed because lstat and readlinkat fail
    // to detect a dir symlink when the dir name ends in '/'. On OSX readlinkat (and readlink)
    // does detect embedded links, but not on Linux. The lstat() service doesn't detect
    // embedded links anywhere. jhrg 1/3/22
    string pathname = path;
    if (!pathname.empty() && pathname.back() == '/') {
        pathname.pop_back();
    }

    bool is_link = false;
    size_t pos;
    int i = 0; // used with search_limit
    do {
        // test pathname
        struct stat buf;
        int status = lstat(pathname.c_str(), &buf);
        if (status == 0) {
            is_link = S_ISLNK(buf.st_mode);
        }
        else {
            string msg = "Could not resolve path when testing for symbolic links: ";
            msg.append(strerror(errno));
            BESDEBUG(MODULE, prolog << msg << endl);
            throw BESInternalError(msg, __FILE__, __LINE__);
        }

        // remove the last part of pathname, including the trailing '/'
        pos = pathname.find_last_of('/');
        if (pos != string::npos)    // find_last_of returns npos if the char is not found
            pathname.erase(pos);
    } while (++i < search_limit && !is_link && pos != string::npos && !pathname.empty());

    return is_link;

#if 0
    // ssize_t readlink(const char *restrict pathname, char *restrict buf, size_t bufsiz);
    // readlinkat (or readlink) can be used to detect sym links in a path or to get the path
    // to the linked file. Here we used it to test for sym links. 1/3/22 jhrg
    ssize_t len = readlinkat(AT_FDCWD, pathname.c_str(), nullptr, 0);
    if (len == -1) {
        // either errno is EINVAL meaning this is not a link or there's really an error
        switch (errno) {
            case EINVAL:
                return false;
            default:
                string msg = "Could not resolve path when testing for symbolic links: ";
                msg.append(strerror(errno));
                throw BESInternalError(msg, __FILE__, __LINE__);
        }
    }

    return true;    // If readlinkat() does not return -1, it's a symlink
#endif
}

/**
 * @brief Is the combination of root + path a pathname the BES can/should access?
 *
 * @note If follow_sym_links is false and any part of the specified path is a
 * symbolic link, this function will return false.
 *
 * @param path The path relative to the BES catalog root directory
 * @param root The BES catalog root directory
 * @param follow_sym_links True if the bes conf allows symbolic links, false by default
 */
void BESUtil::check_path(const string &path, const string &root, bool follow_sym_links) {
    // if nothing is passed in path, then the path checks out since root is assumed to be valid.
    if (path == "") return;

    if (path.find("..") != string::npos) {
        throw_access_error(path, EACCES);   // use the code for 'access would be denied'
    }

    // Check if the combination of root + path exists on this machine. If so, check if it
    // has symbolic links. Return BESNotFoundError if it does not exist and BESForbiddenError
    // if it does exist but contains symbolic links and follow_sym_links is false. jhrg 12/30/21

    string pathname = root;

    if (pathname.back() != '/' && path.front() != '/')
        pathname.append("/");

    pathname.append(path);
    if (access(pathname.c_str(), R_OK) != 0) {
        throw_access_error(pathname, errno);
    }

    if (follow_sym_links == false) {
        auto n = count(path.begin(), path.end(), '/');
        // using 'n' for the search_limit may not be optimal (when path ends in '/', an extra
        // component may be searched) but it's better than testing for a trailing '/' on every call.
        if (pathname_contains_symlink(pathname, n)) {
            throw_access_error(pathname, EACCES);   // use the code for 'access would be denied'
        }
    }
}

char *
BESUtil::fastpidconverter(char *buf, int base)
{
    return fastpidconverter(getpid(), buf, base);
}

char *
BESUtil::fastpidconverter(long val, /* value to be converted */
char *buf, /* output string         */
int base) /* conversion base       */
{
    ldiv_t r; /* result of val / base  */

    if (base > 36 || base < 2) /* no conversion if wrong base */
    {
        *buf = '\0';
        return buf;
    }
    if (val < 0) *buf++ = '-';
    r = ldiv(labs(val), base);

    /* output digits of val/base first */

    if (r.quot > 0) buf = fastpidconverter(r.quot, buf, base);
    /* output last digit */

    *buf++ = "0123456789abcdefghijklmnopqrstuvwxyz"[(int) r.rem];
    *buf = '\0';
    return buf;
}

void BESUtil::removeLeadingAndTrailingBlanks(string &key)
{
    if (!key.empty()) {
        string::size_type first = key.find_first_not_of(" \t\n\r");
        string::size_type last = key.find_last_not_of(" \t\n\r");
        if (first == string::npos)
            key = "";
        else {
            string::size_type num = last - first + 1;
            string new_key = key.substr(first, num);
            key = new_key;
        }
    }
}

string BESUtil::entity(char c)
{
    switch (c) {
    case '>':
        return "&gt;";
    case '<':
        return "&lt;";
    case '&':
        return "&amp;";
    case '\'':
        return "&apos;";
    case '\"':
        return "&quot;";
    default:
        return string(1, c); // is this proper default, just the char?
    }
}

/** Replace characters that are not allowed in XML

 @param in The string in which to replace characters.
 @param not_allowed The set of characters that are not allowed in XML.
 default: ><&'(single quote)"(double quote)
 @return The modified identifier. */
string BESUtil::id2xml(string in, const string &not_allowed)
{
    string::size_type i = 0;

    while ((i = in.find_first_of(not_allowed, i)) != string::npos) {
        in.replace(i, 1, entity(in[i]));
        i++;
    }

    return in;
}

/** Given a string that contains XML escape sequences (i.e., entities),
 translate those back into ASCII characters. Return the modified string.

 @param in The string to modify.
 @return The modified string. */
string BESUtil::xml2id(string in)
{
    string::size_type i = 0;

    while ((i = in.find("&gt;", i)) != string::npos)
        in.replace(i, 4, ">");

    i = 0;
    while ((i = in.find("&lt;", i)) != string::npos)
        in.replace(i, 4, "<");

    i = 0;
    while ((i = in.find("&amp;", i)) != string::npos)
        in.replace(i, 5, "&");

    i = 0;
    while ((i = in.find("&apos;", i)) != string::npos)
        in.replace(i, 6, "'");

    i = 0;
    while ((i = in.find("&quot;", i)) != string::npos)
        in.replace(i, 6, "\"");

    return in;
}

/** Given a string of values separated by a delimiter, break out the values
 * and store in the list.
 *
 * Quoted values must be escaped.
 *
 * If values contain the delimiter then the value must be wrapped in quotes.
 *
 * @param delim delimiter separating the values
 * @param str the original string
 * @param values list of the delimited values returned to caller
 * @throws BESInternalError if missing ending quote or delimiter does not
 * follow end quote
 */
void BESUtil::explode(char delim, const string &str, list<string> &values)
{
    std::string::size_type start = 0;
    std::string::size_type qstart = 0;
    std::string::size_type adelim = 0;
    std::string::size_type aquote = 0;
    bool done = false;
    while (!done) {
        string aval;
        if (str[start] == '"') {
            bool endquote = false;
            qstart = start + 1;
            while (!endquote) {
                aquote = str.find('"', qstart);
                if (aquote == string::npos) {
                    string currval = str.substr(start, str.size() - start);
                    string err = "BESUtil::explode - No end quote after value " + currval;
                    throw BESInternalError(err, __FILE__, __LINE__);
                }
                // could be an escaped escape character and an escaped
                // quote, or an escaped escape character and a quote
                if (str[aquote - 1] == '\\') {
                    if (str[aquote - 2] == '\\') {
                        endquote = true;
                        qstart = aquote + 1;
                    }
                    else {
                        qstart = aquote + 1;
                    }
                }
                else {
                    endquote = true;
                    qstart = aquote + 1;
                }
            }
            if (str[qstart] != delim && qstart != str.size()) {
                string currval = str.substr(start, qstart - start);
                string err = "BESUtil::explode - No delim after end quote " + currval;
                throw BESInternalError(err, __FILE__, __LINE__);
            }
            if (qstart == str.size()) {
                adelim = string::npos;
            }
            else {
                adelim = qstart;
            }
        }
        else {
            adelim = str.find(delim, start);
        }
        if (adelim == string::npos) {
            aval = str.substr(start, str.size() - start);
            done = true;
        }
        else {
            aval = str.substr(start, adelim - start);
        }

        values.push_back(aval);
        start = adelim + 1;
        if (start == str.size()) {
            values.push_back("");
            done = true;
        }
    }
}

/** Given a list of string values create a single string of values
 * delimited by delim
 *
 * If the delimiter exists in a value in the list then that value must
 * be enclosed in quotes
 *
 * @param values list of string values to implode
 * @param delim the delimiter to use in creating the resulting string
 * @return the delim delimited string of values
 */
string BESUtil::implode(const list<string> &values, char delim)
{
    string result;
    list<string>::const_iterator i = values.begin();
    list<string>::const_iterator e = values.end();
    bool first = true;
    string::size_type d; // = string::npos ;
    for (; i != e; i++) {
        if (!first) result += delim;
        d = (*i).find(delim);
        if (d != string::npos && (*i)[0] != '"') {
            string err = (string) "BESUtil::implode - delimiter exists in value " + (*i);
            throw BESInternalError(err, __FILE__, __LINE__);
        }
        //d = string::npos ;
        result += (*i);
        first = false;
    }
    return result;
}

/** @brief Given a url, break the url into its different parts
 *
 * The different parts are the protocol, the domain name, a username if
 * specified, a password if specified, a port if specified, and a path
 * if specified.
 *
 *  struct url
 *  {
 *      string protocol ;
 *      string domain ;
 *      string uname ;
 *      string psswd ;
 *      string port ;
 *      string path ;
 *  } ;

 * @param url string representation of the URL
 * @param 
 */
void BESUtil::url_explode(const string &url_str, BESUtil::url &url_parts)
{
    string rest;

    string::size_type colon = url_str.find(":");
    if (colon == string::npos) {
        string err = "BESUtil::url_explode: missing colon for protocol";
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    url_parts.protocol = url_str.substr(0, colon);

    if (url_str.substr(colon, 3) != "://") {
        string err = "BESUtil::url_explode: no :// in the URL";
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    colon += 3;
    rest = url_str.substr(colon);

    string::size_type slash = rest.find("/");
    if (slash == string::npos) slash = rest.size();

    string::size_type at = rest.find("@");
    if ((at != string::npos) && (at < slash)) {
        // everything before the @ is username:password
        string up = rest.substr(0, at);
        colon = up.find(":");
        if (colon != string::npos) {
            url_parts.uname = up.substr(0, colon);
            url_parts.psswd = up.substr(colon + 1);
        }
        else {
            url_parts.uname = up;
        }
        // everything after the @ is domain/path
        rest = rest.substr(at + 1);
    }
    slash = rest.find("/");
    if (slash == string::npos) slash = rest.size();
    colon = rest.find(":");
    if ((colon != string::npos) && (colon < slash)) {
        // everything before the colon is the domain
        url_parts.domain = rest.substr(0, colon);
        // everything after the folon is port/path
        rest = rest.substr(colon + 1);
        slash = rest.find("/");
        if (slash != string::npos) {
            url_parts.port = rest.substr(0, slash);
            url_parts.path = rest.substr(slash + 1);
        }
        else {
            url_parts.port = rest;
            url_parts.path = "";
        }
    }
    else {
        slash = rest.find("/");
        if (slash != string::npos) {
            url_parts.domain = rest.substr(0, slash);
            url_parts.path = rest.substr(slash + 1);
        }
        else {
            url_parts.domain = rest;
        }
    }
}

string BESUtil::url_create(BESUtil::url &url_parts)
{
    string url = url_parts.protocol + "://";
    if (!url_parts.uname.empty()) {
        url += url_parts.uname;
        if (!url_parts.psswd.empty()) url += ":" + url_parts.psswd;
        url += "@";
    }
    url += url_parts.domain;
    if (!url_parts.port.empty()) url += ":" + url_parts.port;
    if (!url_parts.path.empty()) url += "/" + url_parts.path;

    return url;
}


/**
 * @brief Concatenate path fragments making sure that they are separated by a single '/' character.
 *
 * Returns a new string made from appending secondPart to firstPart while
 * ensuring that a single separator appears between the two parts.
 *
 * @param firstPart The first string to concatenate.
 * @param secondPart The second string to concatenate.
 * @param separator The separator character to use between the two concatenated strings. Default: '/'
 */
string BESUtil::pathConcat(const string &firstPart, const string &secondPart, char separator)
{
    string first = firstPart;
    string second = secondPart;
    string sep(1,separator);

    // make sure there are not multiple slashes at the end of the first part...
    // Note that this removes all the slashes. jhrg 9/27/16
    while (!first.empty() && *first.rbegin() == separator) {
        // C++-11 first.pop_back();
        first = first.substr(0, first.size() - 1);
    }
    // make sure second part does not BEGIN with a slash
    while (!second.empty() && second[0] == separator) {
        // erase is faster? second = second.substr(1);
        second.erase(0, 1);
    }
    string newPath;
    if (first.empty()) {
        newPath = second;
    }
    else if (second.empty()) {
        newPath = first;
    }
    else {
        newPath = first.append(sep).append(second);
    }
    return newPath;
}
/**
 * @brief Assemble path fragments making sure that they are separated by a single '/' character.
 *
 * If the parameter ensureLeadingSlash is true then the returned string will begin with
 * a single '/' character followed by the string firstPart, a single '/' character, and
 * the string secondPart.
 *
 * @note I replaced the code here with a simpler version that assumes the two string
 * arguments do not contain multiple consecutive slashes - I don't think the original
 * version will work in cases where the string is only slashes because it will dereference
 * the return value of begin()
 *
 * @param firstPart The first string to concatenate.
 * @param secondPart The second string to concatenate.
 * @param leadingSlash If this bool value is true then the returned string will have a leading slash.
 *  If the value of leadingSlash is false then the first character  of the returned string will
 *  be the first character of the passed firstPart, which _may_ be a slash. Default False.
 *  @param trailingSlash If this bool is true then the returned string will end it a slash. If
 *   trailingSlash is false, then the returned string will not end with a slash. If trailing
 *   slash(es) need to be removed to accomplish this, then they will be removed. Default False.
 */
string BESUtil::assemblePath(const string &firstPart, const string &secondPart, bool leadingSlash, bool trailingSlash)
{
    BESDEBUG(MODULE, prolog << "firstPart:  '" << firstPart << "'" << endl);
    BESDEBUG(MODULE, prolog << "secondPart: '" << secondPart << "'" << endl);

    string newPath = BESUtil::pathConcat(firstPart, secondPart);
    if (leadingSlash) {
        if (newPath.empty()) {
            newPath = "/";
        }
        else if (newPath.front() != '/') {
            newPath = "/" + newPath;
        }
    }

    if (trailingSlash) {
        if (newPath.empty() || newPath.back() != '/') {
            newPath.append("/");
        }
    }
    else {
        while (!newPath.empty() && newPath.back() == '/')
            newPath.erase(newPath.size()-1);
    }

    BESDEBUG(MODULE, prolog << "newPath: " << newPath << endl);
    return newPath;
}

/**
 * Returns true if (the value of) 'fullString' ends with (the value of) 'ending',
 * false otherwise.
 */
bool BESUtil::endsWith(string const &fullString, string const &ending)
{
    if (fullString.size() >= ending.size()) {
        return (0 == fullString.compare(fullString.size() - ending.size(), ending.size(), ending));
    }
    else {
        return false;
    }
}

/**
 * @brief Checks if the timeout alarm should be canceled based on the value of the BES key BES.CancelTimeoutOnSend
 *
 * If the value of the BES Key BES.CancelTimeoutOnSend is false || no, then
 * do not cancel the timeout alarm.
 * The intent of this is to stop the timeout counter once the
 * BES starts sending data back since, the network link used by a remote
 * client may be low-bandwidth and data providers might want to ensure those
 * users get their data (and don't submit second, third, ..., requests when/if
 * the first one fails). The timeout is initiated in the BES framework when it
 * first processes the request.
 *
 * Default: If the BES key BES.CancelTimeoutOnSend is not set, or if it is set
 * to true || yes then the timeout alrm will be canceled.
 *
 * @note The BES timeout is set/controlled in bes/dispatch/BESInterface
 * in the 'int BESInterface::execute_request(const string &from)' method.
 *
 * @see See the send_data(BESResponseObject *obj, BESDataHandlerInterface &dhi)
 * methods of the children of BESTransmitter
 */
void BESUtil::exit_on_request_timeout()
{
    if (RequestServiceTimer::TheTimer()->is_expired()) {
        stringstream msg;
        msg << "The submitted request took too long to service.";
        throw BESInternalFatalError(msg.str(), __FILE__, __LINE__);
    }
}

/**
 * @brief Checks if the timeout alarm should be canceled based on the value of the BES key BES.CancelTimeoutOnSend
 *
 * If the value of the BES Key BES.CancelTimeoutOnSend is false || no, then
 * do not cancel the timeout alarm.
 * The intent of this is to stop the timeout counter once the
 * BES starts sending data back since, the network link used by a remote
 * client may be low-bandwidth and data providers might want to ensure those
 * users get their data (and don't submit second, third, ..., requests when/if
 * the first one fails). The timeout is initiated in the BES framework when it
 * first processes the request.
 *
 * Default: If the BES key BES.CancelTimeoutOnSend is not set, or if it is set
 * to true || yes then the timeout alarm will be canceled.
 *
 * @note The BES timeout is set/controlled in bes/dispatch/BESInterface
 * in the 'int BESInterface::execute_request(const string &from)' method.
 *
 * @see See the send_data(BESResponseObject *obj, BESDataHandlerInterface &dhi)
 * methods of the children of BESTransmitter
 */
void BESUtil::conditional_timeout_cancel()
{
    const string false_str = "false";
    const string no_str = "no";

    bool cancel_timeout_on_send = true;
    bool found = false;
    string value;

    TheBESKeys::TheKeys()->get_value(BES_KEY_TIMEOUT_CANCEL, value, found);
    if (found) {
        value = BESUtil::lowercase(value);
        if ( value == false_str || value == no_str) cancel_timeout_on_send = false;
    }
    BESDEBUG(MODULE, __func__ << "() - cancel_timeout_on_send: " << (cancel_timeout_on_send ? "true" : "false") << endl);
    if (cancel_timeout_on_send) {
        RequestServiceTimer::TheTimer()->disable_timeout();
        alarm(0);
    }
}

/**
 * @brief Operates on the string 's' to replaces every occurrence of the value of the string
 * 'find_this' with the value of the string 'replace_with_this'
 * @param
 */
unsigned int BESUtil::replace_all(string &s, string find_this, string replace_with_this)
{
    unsigned int replace_count = 0;
    size_t pos = s.find(find_this);
    while (pos != string::npos) {
        // Replace current matching substring
        s.replace(pos, find_this.size(), replace_with_this);
        // Get the next occurrence from current position
        pos = s.find(find_this, pos + replace_with_this.size());
        replace_count++;
    }
    return replace_count;
}

/**
 * @brief Removes duplicate separators and provides leading and trailing separators as directed.
 *
 * @param raw_path The string to normalize
 * @param leading_separator if true then the result will begin with a single separator character. If false
 * the result will not begin with a separator character.
 * @param trailing_separator If true the result will end with a single separator character. If false
 * the result will not end with a separator character.
 * @param separator A string, of length one, containing the separator character for the path. This
 * parameter is optional and its value defaults to the slash '/' character.
 */
string BESUtil::normalize_path(const string &raw_path, bool leading_separator, bool trailing_separator, const string separator /* = "/" */)
{
    if (separator.size() != 1)
        throw BESInternalError("Path separators must be a single character. The string '" + separator + "' does not qualify.", __FILE__, __LINE__);
    char separator_char = separator[0];
    string double_separator;
    double_separator = double_separator.append(separator).append(separator);

    string path(raw_path);

    replace_all(path, double_separator, separator);

    if (path.empty()) {
        path = separator;
    }
    if (path == separator) {
        return path;
    }
    if (leading_separator) {
        if (path[0] != separator_char) {
            path = string(separator).append(path);
        }
    }
    else {
        if (path[0] == separator_char) {
            path = path.substr(1);
        }
    }
    if (trailing_separator) {
        if (*path.rbegin() != separator_char) {
            path = path.append(separator);
        }
    }
    else {
        if (*path.rbegin() == separator_char) {
            path = path.substr(0, path.size() - 1);
        }
    }
    return path;
}

/**
 * A call out thanks to:
 * http://www.oopweb.com/CPP/Documents/CPPHOWTO/Volume/C++Programming-HOWTO-7.html
 * for the tokenizer.
 */
void BESUtil::tokenize(const string& str, vector<string>& tokens, const string& delimiters /* = "/" */)
{
    // Skip delimiters at beginning.
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    string::size_type pos = str.find_first_of(delimiters, lastPos);
    while (string::npos != pos || string::npos != lastPos) {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
}

/**
 * Returns the current time as an ISO8601 string.
 *
 * @param use_local_time True to use the local time, false (default) to use GMT
 * @return The time, either local or GMT/UTC as an ISO8601 string
 */
string BESUtil::get_time(bool use_local_time)
{
    return get_time(time(0), use_local_time);
}

/**
 * @brief Returns the time represented by 'the_time' as an ISO8601 string.
 *
 * @param the_time A time_t value
 * @param use_local_time True to use the local time, false (default) to use GMT
 * @return The time, either local or GMT/UTC as an ISO8601 string
 */
string BESUtil::get_time(time_t the_time, bool use_local_time)
{
    char buf[sizeof "YYYY-MM-DDTHH:MM:SS zones"];
    int status = 0;

    // From StackOverflow:
    // This will work too, if your compiler doesn't support %F or %T:
    // strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M:%S%Z", gmtime(&now));
    //
    // UTC is the default. Override to local time based on the
    // passed parameter 'use_local_time'
    struct tm result{};
    if (!use_local_time) {
        gmtime_r(&the_time, &result);
        status = strftime(buf, sizeof buf, "%FT%T%Z", &result);
    }
    else {
        localtime_r(&the_time, &result);
        status = strftime(buf, sizeof buf, "%FT%T%Z", &result);
    }

    if (!status) {
        ERROR_LOG(prolog + "Error formatting time value!");
        return "date-format-error";
    }

    return buf;
}

/**
 * @brief Splits the string s into the return vector of tokens using the delimiter delim
 * and skipping empty values as instructed by skip_empty.
 *
 * @param s The string to tokenize.
 * @param delim The character delimiter to utilize during tokenization. default: '/'
 * @param skip_empty A boolean flag which controls if empty tokens are returned.
 * @return A vector of strings each of which is a token in the string read left to right.
 * @see BESUtil::tokenize() is probably faster
 */
vector<string> BESUtil::split(const string &s, char delim /* '/' */, bool skip_empty /* true */)
{
    stringstream ss(s);
    string item;
    vector<string> tokens;

    while (getline(ss, item, delim)) {

        if (item.empty() && skip_empty)
            continue;

        tokens.push_back(item);
    }

    return tokens;
}

BESCatalog *BESUtil::separateCatalogFromPath(std::string &ppath)
{
    BESCatalog *catalog = 0;    // pointer to a singleton; do not delete
    vector<string> path_tokens;

    // BESUtil::normalize_path() removes duplicate separators and adds leading and trailing separators as directed.
    string path = BESUtil::normalize_path(ppath, false, false);
    BESDEBUG(MODULE, prolog << "Normalized path: " << path << endl);

    // Because we may need to alter the container/file/resource name by removing
    // a catalog name from the first node in the path we use "use_container" to store
    // the altered container path.
    string use_container = ppath;

    // Breaks path into tokens
    BESUtil::tokenize(path, path_tokens);
    if (!path_tokens.empty()) {
        BESDEBUG(MODULE, "First path token: " << path_tokens[0] << endl);
        catalog = BESCatalogList::TheCatalogList()->find_catalog(path_tokens[0]);
        if (catalog) {
            BESDEBUG(MODULE, prolog << "Located catalog " << catalog->get_catalog_name() << " from path component" << endl);
            // Since the catalog name is in the path we
            // need to drop it this should leave container
            // with a leading
            ppath = BESUtil::normalize_path(path.substr(path_tokens[0].size()), true, false);
            BESDEBUG(MODULE, prolog << "Modified container/path value to:  " << use_container << endl);
        }
    }

    return catalog;
}

void ios_state_msg(std::ios &ios_ref, std::stringstream &msg) {
    msg << " {ios.good()=" << (ios_ref.good() ? "true" : "false") << "}";
    msg << " {ios.eof()="  <<  (ios_ref.eof()?"true":"false") << "}";
    msg << " {ios.fail()=" << (ios_ref.fail()?"true":"false") << "}";
    msg << " {ios.bad()="  <<  (ios_ref.bad()?"true":"false") << "}";
}

// size of the buffer used to read from the temporary file built on disk and
// send data to the client over the network connection (socket/stream)
#define OUTPUT_FILE_BLOCK_SIZE 4096

/**
 * @brief Copies the contents of the file identified by file_name to the stream o_strm
 *
 * Thanks to O'Reilly: https://www.oreilly.com/library/view/c-cookbook/0596007612/ch10s08.html
 * @param file_name
 * @param o_strm
 * @return The number of bytes read/written
 */
uint64_t BESUtil::file_to_stream(const std::string &file_name, std::ostream &o_strm, uint64_t read_start_position)
{
#ifndef NDEBUG
    stringstream msg;
    msg << prolog << "Using ostream: " << (void *) &o_strm << " cout: " << (void *) &cout << endl;
    BESDEBUG(MODULE,  msg.str());
    INFO_LOG( msg.str());
#endif

    vector<char> rbuffer(OUTPUT_FILE_BLOCK_SIZE);
    std::ifstream i_stream(file_name, std::ios_base::in | std::ios_base::binary);  // Use binary mode so we can

    // good() returns true if !(eofbit || badbit || failbit)
    if(!i_stream.good()){
        stringstream msg;
        msg << prolog << "Failed to open file " << file_name;
        ios_state_msg(i_stream, msg);
        BESDEBUG(MODULE, msg.str() << endl);
        throw BESInternalError(msg.str(),__FILE__,__LINE__);
    }

    // good() returns true if !(eofbit || badbit || failbit)
    if(!o_strm.good()){
        stringstream msg;
        msg << prolog << "Problem with ostream. " << file_name;
        ios_state_msg(i_stream, msg);
        BESDEBUG(MODULE, msg.str() << endl);
        throw BESInternalError(msg.str(),__FILE__,__LINE__);
    }
    // this is where we advance to the last byte that was read
    i_stream.seekg(read_start_position);

    //vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    // This is where the file is copied.
    uint64_t tcount = 0;
    while (i_stream.good() && o_strm.good()){
        i_stream.read(rbuffer.data(), OUTPUT_FILE_BLOCK_SIZE);      // Read at most n bytes into
        o_strm.write(rbuffer.data(), i_stream.gcount()); // buf, then write the buf to
        tcount += i_stream.gcount();
    }
    o_strm.flush();

    // fail() is true if failbit || badbit got set, but does not consider eofbit
    if(i_stream.fail() && !i_stream.eof()){
        stringstream msg;
        msg << prolog << "There was an ifstream error when reading from: " << file_name;
        ios_state_msg(i_stream, msg);
        msg << " last_lap: " << i_stream.gcount() << " bytes";
        msg << " total_read: " << tcount << " bytes";
        BESDEBUG(MODULE, msg.str() << endl);
        throw BESInternalError(msg.str(),__FILE__,__LINE__);
    }

    // If we're not at the eof of the input stream then we have failed.
    if (!i_stream.eof()){
        stringstream msg;
        msg << prolog << "Failed to reach EOF on source file: " << file_name;
        ios_state_msg(i_stream, msg);
        msg << " last_lap: " << i_stream.gcount() << " bytes";
        msg << " total_read: " << tcount << " bytes";
        BESDEBUG(MODULE, msg.str() << endl);
        throw BESInternalError(msg.str(),__FILE__,__LINE__);
    }

    // And if something went wrong on the output stream we have failed.
    if(!o_strm.good()){
        stringstream msg;
        msg << prolog << "There was an ostream error during transmit. Transmitted " << tcount  << " bytes.";
        ios_state_msg(o_strm, msg);
        auto crntpos = o_strm.tellp();
        msg << " current_position: " << crntpos << endl;
        BESDEBUG(MODULE, msg.str());
        ERROR_LOG(msg.str());
    }

#ifndef NDEBUG
    msg.str("");
    msg << prolog << "Sent "<< tcount << " bytes from file '" << file_name<< "'. " << endl;
    BESDEBUG(MODULE,msg.str());
    INFO_LOG(msg.str());
#endif

    return tcount;
}

/// Is the given path a directory?
bool BESUtil::is_directory(const string &p) {
    struct stat st{};
    if (stat(p.c_str(), &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    return false;
}

/**
 * For the given path, return the directory name. If the path is a directory,
 * return the path. If the path is a file, return the directory name. This
 * does not check if the path exists, but instead just parses the path.
 */
string BESUtil::get_dir_name(const string &p) {
    size_t pos = p.find_last_of('/');
    if (pos == string::npos) {
        return ".";
    }
    else if (pos == 0) {
        return "/";
    }
    else {
        return p.substr(0, pos);
    }
}

/**
 * Create a directory and any parent directories that do not exist.
 * @param path
 * @param mode
 * @return The return value from mkdir() or 0 if the directory already exists.
 */
int BESUtil::mkdir_p(const string &path, mode_t mode) {
    if (path.empty()) {
        return 0;
    }

    string p = path;
    if (p[p.size() - 1] == '/') {
        p = p.substr(0, p.size() - 1);
    }

    if (p.empty()) {
        return 0;
    }

    if (is_directory(p)) {
        return 0;
    }

    int rc = mkdir_p(get_dir_name(p), mode);
    if (rc == 0) {
        rc = mkdir(p.c_str(), mode);
    }

    return rc;
}

string BESUtil::file_to_string(const string &filename) {
    std::ifstream t(filename);
    if (!t.is_open()) {
        throw BESInternalError("Could not open file: " + filename, __FILE__, __LINE__);
    }
    std::stringstream buffer;
    buffer << t.rdbuf();
    return buffer.str();
}

/**
 * @brief Make and open a temporary file.
 * The file is opened such that we know it is unique and not in use by another process.
 * The name and file descriptor are set in the RemoteResource object.
 * @param temp_file_dir The directory to hold the temporary file.
 * @param temp_file_name The name of the temporary file. A value-result parameter.
 */
int BESUtil::make_temp_file(const string &temp_file_dir, string &temp_file_name) {
    temp_file_name = BESUtil::assemblePath(temp_file_dir, "/bes_util_XXXXXX");

    // Open truncated for update. NB: mkstemp() returns a file descriptor.
    // man mkstemp says "... The file is opened with the O_EXCL flag,
    // guaranteeing that when mkstemp returns successfully we are the only
    // user." 09/19/02 jhrg
    // The 'hack' &temp_file_name[0] is explicitly supported by the C++ 11 standard.
    // jhrg 3/9/23
    int fd = mkstemp(&temp_file_name[0]); // fd mode is 666 or 600 (Unix)
    if (fd < 0) {
        throw BESInternalError(string("mkstemp() for ") + temp_file_name + " failed (" + strerror(errno) + ").",
                               __FILE__, __LINE__);
    }

    return fd;
}

/**
 * @brief Write a string to a file
 * @note Truncates the file if it exists.
 * @param filename
 * @param content
 */
void BESUtil::string_to_file(const string &filename, const string &content) {
    std::ofstream t(filename, std::ios::out | std::ios::trunc);
    if (!t.is_open()) {
        throw BESInternalError("Could not open file: " + filename, __FILE__, __LINE__);
    }
    t << content;
}
