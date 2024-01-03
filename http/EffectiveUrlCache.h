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

#include <memory>
#include <map>
#include <unordered_map>
#include <string>
#include <mutex>

#include "BESObj.h"
#include "BESRegex.h"   // for std::unique_ptr<BESRegex>

namespace http {

class EffectiveUrl;
class url;

/**
 * This is a singleton class. It is used to associate a URL with its "effective" URL.  This means that
 * when a URL is dereferenced the request may go through a potentially large number of redirect actions
 * before the requested resource is retrieved. This final location, from which the requested bytes are transmitted,
 * is termed the "effective url" and that is stored in an in memory cache (std::map) so that later requests may
 * skip the redirects and just get required bytes from the actual source.
 *
 * @note This is the same as following a chain HTTP redirects to get the URL to the origin of the data.
 */
class EffectiveUrlCache : public BESObj {
private:
    EffectiveUrlCache() = default;

    std::mutex d_cache_lock_mutex;

    std::map<std::string, std::shared_ptr<http::EffectiveUrl>> d_effective_urls;

    // URLs that match are not cached.
    std::unique_ptr<BESRegex> d_skip_regex = nullptr;

    int d_enabled = -1;

    std::shared_ptr<EffectiveUrl> get_cached_eurl(std::string const &url_key);

    void set_skip_regex();

    bool is_enabled();

    friend class EffectiveUrlCacheTest;

public:
    /** @brief Get the singleton EffectiveUrlCache instance.
     *
     * This static method returns the instance of this singleton class.
     * The implementation will only build one instance of EffectiveUrlCache and
     * thereafter return a pointer to that instance.
     *
     * Thread safe with C++-11 and greater.
     *
     * This uses the pattern known as Meyer's Singleton.
     *
     * @return A pointer to the EffectiveUrlCache singleton
     */
    static EffectiveUrlCache *TheCache() {
        // Create a local static object the first time the function is called
        static EffectiveUrlCache instance;
        return &instance;
    }

    EffectiveUrlCache(const EffectiveUrlCache &src) = delete;
    EffectiveUrlCache &operator=(const EffectiveUrlCache &rhs) = delete;

    ~EffectiveUrlCache() override = default;

    std::shared_ptr<EffectiveUrl> get_effective_url(std::shared_ptr<url> source_url);

    void dump(std::ostream &strm) const override;

private:
    std::string dump() const {
        std::stringstream sstrm;
        dump(sstrm);
        return sstrm.str();
    }
};

} // namespace http

#endif // _bes_http_EffectiveUrlCache_h_

