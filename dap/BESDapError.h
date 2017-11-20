// BESDapError.h

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

#ifndef BESDapError_h_
#define BESDapError_h_ 1

#include "BESError.h"
#include "BESDataHandlerInterface.h"

#include "Error.h"

/**
 * @brief error object created from libdap error objects and can handle
 * those errors
 *
 * The BESDapError is an error object that is created from libdap error
 * objects caught during libdap processing.
 *
 * The exception handling function handleException knows how to convert
 * libdap errors to dap error responses if the context is set to dap2
 */
class BESDapError: public BESError {
private:
    libdap::ErrorCode d_error_code;

protected:
    BESDapError() : d_error_code(unknown_error)
    {
    }

public:
    BESDapError(const string &s, bool fatal, libdap::ErrorCode ec, const string &file, int line) :
            BESError(s, 0, file, line), d_error_code(ec)
    {
        if (fatal)
            set_error_type(BES_INTERNAL_FATAL_ERROR);
        else
            set_error_type(BES_INTERNAL_ERROR);
    }

    virtual ~BESDapError()
    {
    }

    virtual int get_error_code() const
    {
        return d_error_code;
    }

    virtual void dump(ostream &strm) const;

    static int convert_error_code(int error_code, int current_error_type);
    static int handleException(BESError &e, BESDataHandlerInterface &dhi);
};

#endif // BESDapError_h_
