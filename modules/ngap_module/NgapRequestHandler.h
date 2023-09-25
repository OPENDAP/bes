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

#include "BESRequestHandler.h"

#if 0
#include "BESFileLockingCache.h"
#endif

namespace ngap {

    class NgapRequestHandler: public BESRequestHandler {
        static unsigned int d_cmr_cache_threshold;  // max number of entries
        static unsigned int d_cmr_cache_space;      // remove this many during purge

        static std::unordered_map<std::string, std::string> d_cmr_cache;
        static std::queue<std::string> d_cmr_cache_entries;
        static bool d_use_cmr_cache;

        static unsigned int d_dmrpp_cache_threshold;  // max number of entries
        static unsigned int d_dmrpp_cache_space;      // remove this many during purge

        static std::unordered_map<std::string, std::string> d_dmrpp_cache;
        static std::queue<std::string> d_dmrpp_cache_entries;
        static bool d_use_dmrpp_cache;

#if 0
        static BESFileLockingCache d_dmrpp_file_cache;
        static bool d_use_dmrpp_file_cache;
#endif

        friend class NgapContainer;   // give NgapContainer access to d_translated_urls, etc.

    public:
        NgapRequestHandler(const std::string &name);
        virtual ~NgapRequestHandler(void);

        virtual void dump(std::ostream &strm) const;

        static bool ngap_build_vers(BESDataHandlerInterface &dhi);
        static bool ngap_build_help(BESDataHandlerInterface &dhi);
    };

} // namespace ngap

#endif // NgapRequestHandler.h
