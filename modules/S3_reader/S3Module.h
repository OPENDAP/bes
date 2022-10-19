// S3Container.cc

// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of S3_module, A C++ module that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.

// Copyright (c) 2020 OPeNDAP, Inc.
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
// Authors:
//      ndp       Nathan Potter <ndp@opendap.org>
//      jhrg      James Gallagher <jgallagher@opendap.org>

#ifndef I_S3Module_H
#define I_S3Module_H 1

#include <string>
#include <ostream>

#include "BESAbstractModule.h"

namespace S3 {

class S3Module : public BESAbstractModule {
public:
    S3Module() = default;

    ~S3Module() override = default;

    void initialize(const std::string &modname) override;

    void terminate(const std::string &modname) override;

    void dump(std::ostream &strm) const override;
};

} //namespace S3

#endif // I_S3Module_H
