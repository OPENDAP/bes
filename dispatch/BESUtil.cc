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

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <cstdio>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <cassert>

#include <sstream>
#include <iostream>

using std::istringstream;
using std::cout;
using std::endl;

#include "BESUtil.h"
#include "BESDebug.h"
#include "BESForbiddenError.h"
#include "BESNotFoundError.h"
#include "BESInternalError.h"

#define CRLF "\r\n"

#define debug_key "BesUtil"

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
    struct tm *stm = gmtime(&t);
    char d[256];

    snprintf(d, 255, "%s, %02d %s %4d %02d:%02d:%02d GMT", days[stm->tm_wday], stm->tm_mday, months[stm->tm_mon],
            1900 + stm->tm_year, stm->tm_hour, stm->tm_min, stm->tm_sec);
    d[255] = '\0';
    return string(d);
}

string BESUtil::unhexstring(string s)
{
    int val;
    istringstream ss(s);
    ss >> std::hex >> val;
    char tmp_str[2];
    tmp_str[0] = static_cast<char>(val);
    tmp_str[1] = '\0';
    return string(tmp_str);
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
    for (int j = 0; j < static_cast<int>(return_string.length()); j++) {
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
            new_str += s.substr(index, s.length() - index);
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

/** Check if the specified path is valid
 *
 * Checks to see if the specified path is a valid path or not. The root
 * directory specified is assumed to be valid, so we don't check that
 * part of the path. The path parameter is relative to the root
 * directory.
 *
 * If follow_sym_links is false, then if any part of the specified path
 * is a symbolic link, this function will return false, set the passed
 * has_sym_link parameter. No error message is specified.
 *
 * If there is a problem accessing the specified path then the error
 * string will be filled with whatever system error message is provided.
 *
 * param path path to check
 * param root root directory path, assumed to be valid
 * param follow_sym_links specifies whether allowed to follow symbolic links
 * throws BESForbiddenError if the user is not allowed to traverse the path
 * throws BESNotFoundError if there is a problem accessing the path or the
 * path does not exist.
 **/
void BESUtil::check_path(const string &path, const string &root, bool follow_sym_links)
{
    // if nothing is passed in path, then the path checks out since root is
    // assumed to be valid.
    if (path == "") return;

    // Rather than have two basically identical code paths for the two cases (follow and !follow symlinks)
    // We evaluate the follow_sym_links switch and use a function pointer to get the correct "stat"
    // function for the eval operation.
    int (*ye_old_stat_function)(const char *pathname, struct stat *buf);
    if (follow_sym_links) {
        BESDEBUG(debug_key, "eval_w10n_resourceId() - Using 'stat' function (follow_sym_links = true)" << endl);
        ye_old_stat_function = &stat;
    }
    else {
        BESDEBUG(debug_key, "eval_w10n_resourceId() - Using 'lstat' function (follow_sym_links = false)" << endl);
        ye_old_stat_function = &lstat;
    }

    // make sure there are no ../ in the directory, backing up in any way is
    // not allowed.
    string::size_type dotdot = path.find("..");
    if (dotdot != string::npos) {
        string s = (string) "You are not allowed to access the node " + path;
        throw BESForbiddenError(s, __FILE__, __LINE__);
    }

    // What I want to do is to take each part of path and check to see if it
    // is a symbolic link and it is accessible. If everything is ok, add the
    // next part of the path.
    bool done = false;

    // what is remaining to check
    string rem = path;
    if (rem[0] == '/') rem = rem.substr(1, rem.length() - 1);
    if (rem[rem.length() - 1] == '/') rem = rem.substr(0, rem.length() - 1);

    // full path of the thing to check
    string fullpath = root;
    if (fullpath[fullpath.length() - 1] == '/') {
        fullpath = fullpath.substr(0, fullpath.length() - 1);
    }

    // path checked so far
    string checked;
    while (!done) {
        size_t slash = rem.find('/');
        if (slash == string::npos) {
            fullpath = fullpath + "/" + rem;
            checked = checked + "/" + rem;
            done = true;
        }
        else {
            fullpath = fullpath + "/" + rem.substr(0, slash);
            checked = checked + "/" + rem.substr(0, slash);
            rem = rem.substr(slash + 1, rem.length() - slash);
        }

        struct stat buf;
        int statret = ye_old_stat_function(fullpath.c_str(), &buf);
        if (statret == -1) {
            int errsv = errno;
            // stat failed, so not accessible. Get the error string,
            // store in error, and throw exception
            char *s_err = strerror(errsv);
            string error = "Unable to access node " + checked + ": ";
            if (s_err) {
                error = error + s_err;
            }
            else {
                error = error + "unknown access error";
            }

            BESDEBUG(debug_key, "check_path() - error: "<< error << "   errno: " << errno << endl);

            // ENOENT means that the node wasn't found.
            // On some systems a file that doesn't exist returns ENOTDIR because: w.f.t?
            // Otherwise, access is being denied for some other reason
            if (errsv == ENOENT || errsv == ENOTDIR) {
                // On some systems a file that doesn't exist returns ENOTDIR because: w.f.t?
                throw BESNotFoundError(error, __FILE__, __LINE__);
            }
            else {
                throw BESForbiddenError(error, __FILE__, __LINE__);
            }
        }
        else {
            //The call to (stat | lstat) was successful, now check to see if it's a symlink.
            // Note that if follow_symlinks is true then this will never evaluate as true
            // because we'll be using 'stat' and not 'lstat' and stat will follow the link
            // and return information about the file/dir pointed to by the symlink
            if (S_ISLNK(buf.st_mode)) {
                string error = "You do not have permission to access " + checked;
                throw BESForbiddenError(error, __FILE__, __LINE__);
            }
        }
    }

#if 0
    while (!done) {
        size_t slash = rem.find('/');
        if (slash == string::npos) {
            fullpath = fullpath + "/" + rem;
            checked = checked + "/" + rem;
            done = true;
        }
        else {
            fullpath = fullpath + "/" + rem.substr(0, slash);
            checked = checked + "/" + rem.substr(0, slash);
            rem = rem.substr(slash + 1, rem.length() - slash);
        }

        if (!follow_sym_links) {
            struct stat buf;
            int statret = lstat(fullpath.c_str(), &buf);
            if (statret == -1) {
                int errsv = errno;
                // stat failed, so not accessible. Get the error string,
                // store in error, and throw exception
                char *s_err = strerror(errsv);
                string error = "Unable to access node " + checked + ": ";
                if (s_err) {
                    error = error + s_err;
                }
                else {
                    error = error + "unknown access error";
                }
                // ENOENT means that the node wasn't found. Otherwise, access
                // is denied for some reason
                if (errsv == ENOENT) {
                    throw BESNotFoundError(error, __FILE__, __LINE__);
                }
                else {
                    throw BESForbiddenError(error, __FILE__, __LINE__);
                }
            }
            else {
                // lstat was successful, now check if sym link
                if (S_ISLNK( buf.st_mode )) {
                    string error = "You do not have permission to access "
                    + checked;
                    throw BESForbiddenError(error, __FILE__, __LINE__);
                }
            }
        }
        else {
            // just do a stat and see if we can access the thing. If we
            // can't, get the error information and throw an exception
            struct stat buf;
            int statret = stat(fullpath.c_str(), &buf);
            if (statret == -1) {
                int errsv = errno;
                // stat failed, so not accessible. Get the error string,
                // store in error, and throw exception
                char *s_err = strerror(errsv);
                string error = "Unable to access node " + checked + ": ";
                if (s_err) {
                    error = error + s_err;
                }
                else {
                    error = error + "unknown access error";
                }
                // ENOENT means that the node wasn't found. Otherwise, access
                // is denied for some reason
                if (errsv == ENOENT) {
                    throw BESNotFoundError(error, __FILE__, __LINE__);
                }
                else {
                    throw BESForbiddenError(error, __FILE__, __LINE__);
                }
            }
        }
    }

#endif
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
                    string currval = str.substr(start, str.length() - start);
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
            if (str[qstart] != delim && qstart != str.length()) {
                string currval = str.substr(start, qstart - start);
                string err = "BESUtil::explode - No delim after end quote " + currval;
                throw BESInternalError(err, __FILE__, __LINE__);
            }
            if (qstart == str.length()) {
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
            aval = str.substr(start, str.length() - start);
            done = true;
        }
        else {
            aval = str.substr(start, adelim - start);
        }

        values.push_back(aval);
        start = adelim + 1;
        if (start == str.length()) {
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
    if (slash == string::npos) slash = rest.length();

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
    if (slash == string::npos) slash = rest.length();
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
 */
string BESUtil::assemblePath(const string &firstPart, const string &secondPart, bool ensureLeadingSlash)
{
#if 0
    assert(!firstPart.empty());

    // This version works but does not remove duplicate slashes
    string first = firstPart;
    string second = secondPart;

    // add a leading slash if needed
    if (ensureLeadingSlash && first[0] != '/')
        first = "/" + first;

    // if 'second' start with a slash, remove it
    if (second[0] == '/')
        second = second.substr(1);

    // glue the two parts together, adding a slash if needed
    if (first.back() == '/')
        return first.append(second);
    else
        return first.append("/").append(second);
#endif

#if 1
    BESDEBUG("util", "BESUtil::assemblePath() -  firstPart: "<< firstPart << endl);
    BESDEBUG("util", "BESUtil::assemblePath() -  secondPart: "<< secondPart << endl);

    assert(!firstPart.empty());

    string first = firstPart;
    string second = secondPart;

    if (ensureLeadingSlash) {
        if (first[0] != '/') first = "/" + first;
    }

    // make sure there are not multiple slashes at the end of the first part...
    // Note that this removes all of the slashes. jhrg 9/27/16
    while (!first.empty() && first.back() == '/') {
        first.pop_back();
    }

    // make sure second part does not BEGIN with a slash
    while (!second.empty() && second[0] == '/') {
        second = second.substr(1);
    }

    string newPath = first.append("/").append(second);

    BESDEBUG("util", "BESUtil::assemblePath() -  newPath: "<< newPath << endl);

    return newPath;
#endif

#if 0
    BESDEBUG("util", "BESUtil::assemblePath() -  firstPart: "<< firstPart << endl);
    BESDEBUG("util", "BESUtil::assemblePath() -  secondPart: "<< secondPart << endl);

    string first = firstPart;
    string second = secondPart;

    if (ensureLeadingSlash) {
        if (*first.begin() != '/') first = "/" + first;
    }

    // make sure there are not multiple slashes at the end of the first part...
    while (*first.rbegin() == '/' && first.length() > 0) {
        first = first.substr(0, first.length() - 1);
    }

    // make sure first part ends with a "/"
    if (*first.rbegin() != '/') {
        first += "/";
    }

    // make sure second part does not BEGIN with a slash
    while (*second.begin() == '/' && second.length() > 0) {
        second = second.substr(1);
    }

    string newPath = first + second;

    BESDEBUG("util", "BESUtil::assemblePath() -  newPath: "<< newPath << endl);

    return newPath;
#endif
}

/**
 * Returns true if (the value of) 'fullString' ends with (the value of) 'ending',
 * false otherwise.
 */
bool BESUtil::endsWith(std::string const &fullString, std::string const &ending)
{
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
    }
    else {
        return false;
    }
}
