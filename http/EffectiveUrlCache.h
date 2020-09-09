// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES http package, part of the Hyrax data server.

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

#ifndef _bes_http_EffectiveUrlCache_h_
#define _bes_http_EffectiveUrlCache_h_ 1

#include <map>
#include <string>

#include "BESObj.h"
#include "BESDataHandlerInterface.h"
#include "BESRegex.h"
#include "url_impl.h"


namespace http {

/**
 *
 */
class EffectiveUrlCache: public BESObj {
private:
    static EffectiveUrlCache * d_instance;


    std::map<std::string , http::url *> d_effective_urls;

    // Things that match get skipped.
    BESRegex *d_skip_regex;

    int d_enabled;

    static void initialize_instance();
    static void delete_instance();

    friend class EffectiveUrlCacheTest;

public:

    static EffectiveUrlCache *TheCache();

    EffectiveUrlCache();
    virtual ~EffectiveUrlCache();

    void add(const std::string  &source_url, http::url *effective_url);

    http::url *get(const std::string  &source_url);

    virtual void dump(std::ostream &strm) const;

    void cache_effective_url(const std::string &source_url);
    void cache_effective_url(const std::string &source_url, BESRegex *skip_regex);
    bool is_enabled();
    BESRegex *get_cache_effective_urls_skip_regex();

};

} // namespace http

#endif // BES_http_EffectiveUrlCache_h_

