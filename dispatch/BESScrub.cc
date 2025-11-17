// BESScrub.cc

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

#include <limits.h>
#include <memory>

#include "BESRegex.h"
#include "BESScrub.h"

using std::string;
using std::unique_ptr;

/** @name Security functions */
//@{

/** @brief sanitize command line arguments

    Test the given command line argument to protect against command
    injections

    @param arg argument to check
    @return true if ok, false otherwise
*/
bool BESScrub::command_line_arg_ok(const string &arg) {
    if (arg.size() > 255)
        return false;

    return true;
}

/** @brief sanitize the size of an array.
    Test for integer overflow when dynamically allocating an array.
    @param nelem Number of elements.
    @param sz size of each element.
    @return True if the \c nelem elements of \c sz size will overflow an array. */
bool BESScrub::size_ok(unsigned int sz, unsigned int nelem) { return (sz > 0 && nelem < UINT_MAX / sz); }

/** @brief Does the string name a potentailly valid pathname?
    Test the given pathname to verfiy that it is a valid name. We define this
    as: Contains only printable characters; and Is less then 256 characters.
    If \e strict is true, test that the pathname consists of only letters,
    digits, and underscore, dash and dot characters instead of the more general
    case where a pathname can be composed of any printable characters.

    @note Using this function does not guarentee that the path is valid, only
    that the path \e could be valid. The intent is foil attacks where an
    exploit is encoded in a string then passed to a library function. This code
    does not address whether the pathname references a valid resource.

    @param path The pathname to test
    @param strict Apply more restrictive tests (true by default)
    @return true if the pathname consists of legal characters and is of legal
    size, false otherwise. */
bool BESScrub::pathname_ok(const string &path, bool strict) {
    if (path.size() > 255)
        return false;

    if (strict) {
        const BESRegex name("[[:alpha:][:digit:]_./-]+");
        return name.match(path) == static_cast<int>(path.size());
    } else {
        const BESRegex name("[:print:]+");
        return name.match(path) == static_cast<int>(path.size());
    }

#if 0
    BESRegex name("[[:alpha:][:digit:]_./-]+");
    if (!strict)
        name = "[:print:]+";
        
    string::size_type len = path.size() ;
    int ret = name.match( path.c_str(), len ) ;
    if( ret != static_cast<int>(len) )
        return false ;
    return true ;
#endif
}

//@}
