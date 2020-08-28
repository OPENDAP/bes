
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
#include <time.h>


namespace http {



class  url {
private:
    void parse(const std::string &source_url);

    std::string d_source_url;
    std::string d_protocol;
    std::string d_host;
    std::string d_path;
    std::string d_query;
    std::map<std::string, std::vector<std::string> * > d_query_kvp;
    time_t d_ingest_time;

public:

    // omitted copy, ==, accessors, ...
    explicit url(const std::string &url_s):d_source_url(url_s), d_ingest_time(0) {
        parse(url_s);
    }

    ~url();
    std::string str() const { return d_source_url; }

    std::string protocol() const { return d_protocol; }

    std::string host() const { return d_host; }

    std::string path() const { return d_path; }

    std::string query() const { return d_query; }

    time_t ingest_time() const { return d_ingest_time; }

    void set_ingest_time(const time_t itime){
        d_ingest_time = itime;
    }

    std::string query_parameter_value(const std::string &key) const;
    void query_parameter_values(const std::string &key, std::vector<std::string> &values) const;

    bool is_expired();

    std::string to_string();

};

} // namespace http
#endif /* _bes_http_url_HH_ */