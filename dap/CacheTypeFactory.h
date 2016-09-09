
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2016 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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

#ifndef cache_type_factory_h
#define cache_type_factory_h

#include <string>

#include <BaseTypeFactory.h>
#include "CachedSequence.h"

//class CachedSequence;

/**
 * A factory for types that work with data read from the (function)
 * response cache. Currently the only class that has to be specialized
 * to use cached data is Sequence (see CachedSequence)
 */
class CacheTypeFactory : public libdap::BaseTypeFactory {
public:
    CacheTypeFactory() {}
    virtual ~CacheTypeFactory() {}

    virtual libdap::Sequence *NewSequence(const std::string &n = "") const;
};

#endif // cache_type_factory_h
