// NgapContainer.cc

// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of ngap_module, A C++ module that can be loaded in to
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

#ifndef I_NgapRequestHandler_H
#define I_NgapRequestHandler_H

#include <string>
#include <ostream>
#include <queue>
#include <unordered_map>

#include "MemoryCache.h"
#include "FileCache.h"
#include "BESRequestHandler.h"

namespace ngap {

class NgapRequestHandler : public BESRequestHandler {

    static unsigned int d_cmr_cache_size;  // max number of entries
    static unsigned int d_cmr_cache_purge;      // remove this many during purge

    static bool d_use_cmr_cache;
    static MemoryCache<std::string> d_cmr_mem_cache;

    static unsigned int d_dmrpp_mem_cache_size;  // max number of entries
    static unsigned int d_dmrpp_mem_cache_purge;      // remove this many during purge

    static bool d_use_dmrpp_cache;
    static MemoryCache<std::string> d_dmrpp_mem_cache;

    static unsigned long long d_dmrpp_file_cache_size;
    static unsigned long long d_dmrpp_file_cache_purge_size;
    static std::string d_dmrpp_file_cache_dir;

    static FileCache d_dmrpp_file_cache;

    friend class NgapContainer;   // give NgapContainer access to the cache parameters
    friend class NgapContainerTest;
    friend class NgapRequestHandlerTest;

public:
    explicit NgapRequestHandler(const std::string &name);

    NgapRequestHandler() = delete;

    ~NgapRequestHandler() override = default;

    NgapRequestHandler(const NgapRequestHandler &src) = delete;

    NgapRequestHandler &operator=(const NgapRequestHandler &rhs) = delete;

    void dump(std::ostream &strm) const override;

    static bool ngap_build_vers(BESDataHandlerInterface &dhi);

    static bool ngap_build_help(BESDataHandlerInterface &dhi);
};

} // namespace ngap

#endif // NgapRequestHandler.h
