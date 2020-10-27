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

#include <pthread.h>

#include "BESObj.h"
#include "BESDataHandlerInterface.h"
#include "BESRegex.h"
#include "BESLog.h"
#include "EffectiveUrl.h"


namespace http {

    /**
 * RAII. Lock access to the get_easy_handle() and release_handle() methods.
 */
class EucLock {
private:
    pthread_mutex_t &m_mutex;
    EucLock();
    EucLock(const EucLock &rhs);
public:
    EucLock(pthread_mutex_t &lock);
    ~EucLock();
};


    /**
 *
 */
class EffectiveUrlCache: public BESObj {
private:
    static EffectiveUrlCache * d_instance;
    std::map<std::string , http::EffectiveUrl *> d_effective_urls;
    pthread_mutex_t d_get_effective_url_cache_mutex;

    // Things that match get skipped.
    BESRegex *d_skip_regex;

    int d_enabled;

    static void initialize_instance();
    static void delete_instance();

    friend class EffectiveUrlCacheTest;
    http::EffectiveUrl *get(const std::string  &source_url);
    void add(const std::string  &source_url, http::EffectiveUrl *effective_url);
    BESRegex *get_skip_regex();

    EffectiveUrlCache();

    virtual ~EffectiveUrlCache();

public:

    static EffectiveUrlCache *TheCache();
    bool is_enabled();

    http::EffectiveUrl *get_effective_url(const std::string &source_url);
    http::EffectiveUrl *get_effective_url(const std::string &source_url, BESRegex *skip_regex);

    virtual void dump(std::ostream &strm) const;
    virtual std::string dump() const;

};

} // namespace http

#endif // BES_http_EffectiveUrlCache_h_

