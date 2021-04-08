
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

class EffectiveUrlCache;

class  url {
private:

    std::string d_source_url_str;
    std::string d_protocol;
    std::string d_host;
    std::string d_path;
    std::string d_query;
    std::map<std::string, std::vector<std::string> * > d_query_kvp;
    // time_t d_ingest_time;
    std::chrono::system_clock::time_point d_ingest_time;
    bool d_trusted;

    void parse();

protected:


public:

    explicit url() :
            d_source_url_str(""),
            d_protocol(""),
            d_host(""),
            d_path(""),
            d_query(""),
            d_ingest_time(std::chrono::system_clock::now()),
            d_trusted(false) {
    }
    explicit url(const std::string &url_s, bool trusted=false) :
            d_source_url_str(url_s),
            d_protocol(""),
            d_host(""),
            d_path(""),
            d_query(""),
            d_ingest_time(std::chrono::system_clock::now()),
            d_trusted(trusted) {
        parse();
    }

    url(http::url const &src_url){
        d_source_url_str = src_url.d_source_url_str;
        d_protocol = src_url.d_protocol;
        d_host = src_url.d_host;
        d_path = src_url.d_path;
        d_query = src_url.d_query;
        d_ingest_time = src_url.d_ingest_time;
        d_trusted = src_url.d_trusted;
    }

    explicit url(const std::shared_ptr<http::url> &source_url){
        d_source_url_str = source_url->d_source_url_str;
        d_protocol = source_url->d_protocol;
        d_host = source_url->d_host;
        d_path = source_url->d_path;
        d_query = source_url->d_query;
        d_ingest_time = source_url->d_ingest_time;
        d_trusted = source_url->d_trusted;
    }

    explicit url(const std::shared_ptr<http::url> &source_url, bool trusted){
        d_source_url_str = source_url->d_source_url_str;
        d_protocol = source_url->d_protocol;
        d_host = source_url->d_host;
        d_path = source_url->d_path;
        d_query = source_url->d_query;
        d_ingest_time = source_url->d_ingest_time;
        d_trusted = trusted;
    }

    virtual ~url();

    virtual std::string str() const { return d_source_url_str; }

    virtual std::string protocol() const { return d_protocol; }

    virtual std::string host() const { return d_host; }

    virtual std::string path() const { return d_path; }

    virtual std::string query() const { return d_query; }

    virtual std::time_t  ingest_time() const {
        return std::chrono::system_clock::to_time_t(d_ingest_time);
    }

    virtual void set_ingest_time(const std::time_t &itime){
        d_ingest_time = std::chrono::system_clock::from_time_t(itime);
    }

    virtual std::string query_parameter_value(const std::string &key) const;
    virtual void query_parameter_values(const std::string &key, std::vector<std::string> &values) const;

    virtual bool is_expired();
    virtual bool is_trusted() { return d_trusted; };

    virtual std::string dump();

};

} // namespace http
#endif /* _bes_http_url_HH_ */