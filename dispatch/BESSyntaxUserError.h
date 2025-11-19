// BESSyntaxUserError.h

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

#ifndef BESSyntaxUserError_h_
#define BESSyntaxUserError_h_ 1

#include "BESError.h"

/** @brief error thrown if there is a user syntax error in the request or
 * any other user error
 */
class BESSyntaxUserError : public BESError {
public:
    BESSyntaxUserError(std::string s, std::string file, unsigned int line)
        : BESError(std::move(s), BES_SYNTAX_USER_ERROR, std::move(file), line) {}

    ~BESSyntaxUserError() override = default;

    void dump(std::ostream &strm) const override {
        strm << "BESSyntaxUserError::dump - (" << (void *)this << ")" << std::endl;
        BESIndent::Indent();
        BESError::dump(strm);
        BESIndent::UnIndent();
    }
    std::string error_name() const override { return "BESSyntaxUserError"; }
};

#endif // BESSyntaxUserError_h_
