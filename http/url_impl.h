
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

    std::string d_source_url;
    std::string d_protocol;
    std::string d_host;
    std::string d_path;
    std::string d_query;
    std::map<std::string, std::vector<std::string> * > d_query_kvp;
    time_t d_ingest_time;

protected:

public:

    void parse(const std::string &source_url);

    explicit url():d_source_url(""), d_ingest_time(0) {
    }


    // omitted copy, ==, accessors, ...
    explicit url(const std::string &url_s):d_source_url(url_s), d_ingest_time(0) {
        parse(url_s);
    }

    ~url();
    virtual std::string str() const { return d_source_url; }

    virtual std::string protocol() const { return d_protocol; }

    virtual std::string host() const { return d_host; }

    virtual std::string path() const { return d_path; }

    virtual std::string query() const { return d_query; }

    virtual time_t ingest_time() const { return d_ingest_time; }

    virtual void set_ingest_time(const time_t itime){
        d_ingest_time = itime;
    }

    virtual std::string query_parameter_value(const std::string &key) const;
    virtual void query_parameter_values(const std::string &key, std::vector<std::string> &values) const;

    virtual bool is_expired();

    virtual std::string dump();

};

} // namespace http
#endif /* _bes_http_url_HH_ */