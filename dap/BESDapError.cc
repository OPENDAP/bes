// BESDapError.cc

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

#include <sstream>

#include "BESDapError.h"

using namespace std;

BESDapError::BESDapError(string msg, bool fatal, libdap::ErrorCode ec, string file, unsigned int line) :
        BESError(std::move(msg), 0, std::move(file), line), d_dap_error_code(ec)
{
    set_bes_error_type(convert_error_code(ec, fatal));
}

/** @brief converts the libdap error code to the bes error type
 *
 * This function converts the libdap error codes in Error to the proper BES
 * error type.
 *
 *    undefined_error   1000 -> BES_INTERNAL_ERROR
 *    unknown_error     1001 -> BES_INTERNAL_ERROR
 *    internal_error    1002 -> BES_INTERNAL_FATAL_ERROR
 *    no_such_file      1003 -> BES_NOT_FOUND_ERROR
 *    no_such_variable  1004 -> BES_SYNTAX_USER_ERROR
 *    malformed_expr    1005 -> BES_SYNTAX_USER_ERROR
 *    no_authorization  1006 -> BES_FORBIDDEN_ERROR
 *    cannot_read_file  1007 -> BES_FORBIDDEN_ERROR
 *    dummy_message     1008 -> BES_FORBIDDEN_ERROR
 *
 * If the error type is already set to BES_INTERNAL_FATAL_ERROR it is not
 * changed.
 *
 * @param error_code The libdap error code to convert
 * @param current_error_type The current error type of the exception
 * @returns BES error type used in any error response
 */
int BESDapError::convert_error_code(int error_code, int current_error_type)
{
    if (current_error_type == BES_INTERNAL_FATAL_ERROR) return current_error_type;

    switch (error_code) {
    case undefined_error:
    case unknown_error: {
        return BES_INTERNAL_ERROR;
    }
    case internal_error: {
        return BES_INTERNAL_FATAL_ERROR;
    }
    case no_such_file: {
        return BES_NOT_FOUND_ERROR;
    }
    case no_such_variable:
    case malformed_expr: {
        return BES_SYNTAX_USER_ERROR;
    }
    case no_authorization:
    case cannot_read_file:
    case dummy_message: {
        return BES_FORBIDDEN_ERROR;
    }
    default: {
        return BES_INTERNAL_ERROR;
    }
    }
}

int BESDapError::convert_error_code(int error_code, bool fatal)
{
    return convert_error_code(error_code, (fatal) ? BES_INTERNAL_FATAL_ERROR: BES_INTERNAL_ERROR);
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance and the stored error code
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESDapError::dump(ostream &strm) const
{
	strm << BESIndent::LMarg << "BESDapError::dump - (" << (void *) this << ")" << endl;
	BESIndent::Indent();
	strm << BESIndent::LMarg << "error code = " << get_dap_error_code() << endl;
	BESError::dump(strm);
	BESIndent::UnIndent();
}
