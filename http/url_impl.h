
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

#ifndef _bes_http_url_HH_
#define _bes_http_url_HH_ 1

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <chrono>

namespace http {

/**
 * @brief Parse a URL into the protocol, host, path and query parts
 * @note This class also manages time and time-based expiration using
 * KVP info that AWS puts in the query string of the URL.
 */
class url {
public:
    using kvp_map_t = std::map<std::string, std::vector<std::string>>;

private:
    std::string d_source_url_str;
    std::string d_protocol;
    std::string d_host;
    std::string d_path;
    std::string d_query;
    kvp_map_t d_query_kvp;
    std::chrono::system_clock::time_point d_ingest_time = std::chrono::system_clock::now();
    bool d_trusted = false;

    void parse();
    void parse_query_string();

    friend class HttpUrlTest;

public:
    url() = default;

    explicit url(std::string url_s, bool trusted = false) :
            d_source_url_str(std::move(url_s)),
            d_trusted(trusted) {
        parse();
    }

    url(const http::url &src_url) = default;

    // TODO Remove these shared_ptr methods if possible. jhrg 2/20/25
    explicit url(const std::shared_ptr<http::url> &source_url) :
            d_source_url_str(source_url->d_source_url_str),
            d_protocol(source_url->d_protocol),
            d_host(source_url->d_host),
            d_path(source_url->d_path),
            d_query(source_url->d_query),
            d_query_kvp(source_url->d_query_kvp),
            d_ingest_time(source_url->d_ingest_time),
            d_trusted(source_url->d_trusted) {
    }

    url(const std::shared_ptr<http::url> &source_url, bool trusted) :
            d_source_url_str(source_url->d_source_url_str),
            d_protocol(source_url->d_protocol),
            d_host(source_url->d_host),
            d_path(source_url->d_path),
            d_query(source_url->d_query),
            d_query_kvp(source_url->d_query_kvp),
            d_ingest_time(source_url->d_ingest_time),
            d_trusted(trusted) {
    }

    virtual ~url() = default;

    url &operator=(const url &rhs) = delete;

    virtual std::string str() const { return d_source_url_str; }

    virtual std::string protocol() const { return d_protocol; }

    virtual std::string host() const { return d_host; }

    virtual std::string path() const { return d_path; }

    virtual std::string query() const { return d_query; }

    virtual std::time_t  ingest_time() const {
        return std::chrono::system_clock::to_time_t(d_ingest_time);
    }

    virtual void set_ingest_time(const std::time_t &itime) {
        d_ingest_time = std::chrono::system_clock::from_time_t(itime);
    }

    virtual std::string query_parameter_value(const std::string &key) const;
    virtual size_t query_parameter_values_size(const std::string &key) const;
    virtual const std::vector<std::string> &query_parameter_values(const std::string &key) const;

    virtual bool is_expired();
    virtual bool is_trusted() const { return d_trusted; };

    virtual std::string dump();
};

} // namespace http
#endif /* _bes_http_url_HH_ */