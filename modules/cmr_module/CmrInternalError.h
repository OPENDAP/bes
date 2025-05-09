// -*- mode: c++; c-basic-offset:4 -*-
//
// CmrInternalError.h
//
// This file is part of BES cmr_module
//
// Copyright (c) 2022 OPeNDAP, Inc.
// Author: Nathan Potter <ndp@opendap.org>
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
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// Please read the full copyright statement in the file COPYRIGHT_URI.
//
#include <BESInternalError.h>

#ifndef MODULES_CMR_MODULE_CMRERROR_H_
#define MODULES_CMR_MODULE_CMRERROR_H_

namespace cmr {

class CmrInternalError: public BESInternalError {

public:
    CmrInternalError(const std::string &msg, const std::string &file, unsigned int line) :
        BESInternalError("CmrInternalError " + msg, file, line)
    { }

    ~CmrInternalError() override = default;

    void dump(std::ostream &strm) const override
    {
        strm << "CmrInternalError::dump - (" << (void *) this << ")" << std::endl;
        BESIndent::Indent();
        BESInternalError::dump(strm);
        BESIndent::UnIndent();
    }

};

} /* namespace cmr */

#endif /* MODULES_CMR_MODULE_CMRERROR_H_ */
