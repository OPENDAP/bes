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

#include <string>
#include <map>

#include "url_impl.h"

namespace http {

    class EffectiveUrl : public url {
    private:
        std::vector<std::string> d_response_header_names;
        std::vector<std::string> d_response_header_values;
        //std::map<std::string, std::string> d_response_headers;
        std::vector<std::string> d_resp_hdr_lines;

    public:

        explicit EffectiveUrl(const std::string &url_s, const std::vector<std::string> &resp_hdrs) : http::url(url_s) {
            set_response_headers(resp_hdrs);
        };

        explicit EffectiveUrl(const std::string &url_s) : http::url(url_s), d_response_header_names(), d_response_header_values() {};
        explicit EffectiveUrl() : http::url(""), d_response_header_names(), d_response_header_values() {};

        virtual ~EffectiveUrl(){ }

        bool is_expired() override;

        void get_header(const std::string &name, std::string &value, bool &found );

        void set_response_headers(const std::vector<std::string> &resp_hdrs);

        void url(std::string url){
            parse(url);
        }

        std::string dump() override;
    };
} // namespace http

#endif //HYRAX_GIT_EFFECTIVEURL_H
