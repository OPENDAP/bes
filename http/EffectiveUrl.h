// -*- mode: c++; c-basic-offset:4 -*-
//
// EffectiveUrl.h
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


#ifndef HYRAX_GIT_EFFECTIVEURL_H
#define HYRAX_GIT_EFFECTIVEURL_H

#include <chrono>

#include <memory>
#include <string>
#include <map>
#include <utility>

#include "HttpNames.h"
#include "url_impl.h"

namespace http {

/**
 * An EffectiveUrl is always acquired by following redirects and so may include response
 * headers received with the final redirect response.
 */
class EffectiveUrl : public url {
    // inherit constructors. jhrg 12/29/22
    using url::url;

    // We need order, so we use two vectors instead of a map to hold the header "map"
    std::vector<std::string> d_response_header_names;
    std::vector<std::string> d_response_header_values;

public:
    EffectiveUrl() = default;
    EffectiveUrl(const EffectiveUrl &src_url) = default;

    EffectiveUrl(const std::string &url_s, bool trusted) : http::url(url_s, trusted) {};
    EffectiveUrl(const std::string &url_s, const std::vector<std::string> &resp_hdrs, bool trusted = false)
            : http::url(url_s, trusted) {
        ingest_response_headers(resp_hdrs);
    }

    explicit EffectiveUrl(const http::url &src_url) : http::url(src_url) {}

    explicit EffectiveUrl(const std::shared_ptr<http::EffectiveUrl> &source_url)
        : http::url(source_url),
          d_response_header_names(source_url->d_response_header_names),
          d_response_header_values(source_url->d_response_header_values) {}
    explicit EffectiveUrl(const std::shared_ptr<http::EffectiveUrl> &source_url, bool trusted)
        : http::url(source_url,trusted),
          d_response_header_names(source_url->d_response_header_names),
          d_response_header_values(source_url->d_response_header_values) {}

    ~EffectiveUrl() override = default;

    bool is_expired() override;

    void get_header(const std::string &name, std::string &value, bool &found );

    void ingest_response_headers(const std::vector<std::string> &resp_hdrs);

    std::string dump() override;
};

} // namespace http

#endif //HYRAX_GIT_EFFECTIVEURL_H
