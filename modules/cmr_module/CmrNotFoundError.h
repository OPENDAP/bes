//
// Created by ndp on 11/22/22.
//


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

#ifndef BES_CMRNOTFOUNDERROR_H
#define BES_CMRNOTFOUNDERROR_H

#include "config.h"
#include <string>
#include <ostream>


#include "BESError.h"
#include "BESNotFoundError.h"

namespace cmr {

class CmrNotFoundError: public BESNotFoundError {

public:
    CmrNotFoundError(const std::string &msg, const std::string &file, unsigned int line) :
            BESNotFoundError("CmrNotFoundError " + msg, file, line)
    { }

    ~CmrNotFoundError() override = default;

    void dump(std::ostream &strm) const override
    {
        strm << "CmrNotFoundError::dump - (" << (void *) this << ")" << std::endl;
        BESIndent::Indent();
        BESNotFoundError::dump(strm);
        BESIndent::UnIndent();
    }

};

} /* namespace cmr */

#endif //BES_CMRNOTFOUNDERROR_H
