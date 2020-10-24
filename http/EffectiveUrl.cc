// -*- mode: c++; c-basic-offset:4 -*-
//
// EffectiveUrl.cc
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

#include "config.h"

#include <string>
#include <sstream>
#include <map>
#include <vector>

#include <time.h>

#include "BESDebug.h"
#include "BESUtil.h"
#include "BESLog.h"

#include "HttpNames.h"
#include "url_impl.h"
#include "EffectiveUrl.h"

using std::string;
using std::map;
using std::pair;
using std::vector;
using std::endl;
using std::stringstream;

#define CACHE_CONTROL_HEADER_KEY "cache-control"
#define REFRESH_THRESHOLD 60

#define MODULE HTTP_MODULE
#define prolog std::string("EffectiveUrl::").append(__func__).append("() - ")

namespace http {


    bool EffectiveUrl::is_expired() {

        BESDEBUG(MODULE, prolog << "BEGIN" << endl);
        bool expired = false;
        bool found = false;
        string cc_hdr_val;

        get_header(CACHE_CONTROL_HEADER_KEY,cc_hdr_val,found);
        if(found){
            BESDEBUG(MODULE, prolog << CACHE_CONTROL_HEADER_KEY << " '" << cc_hdr_val << "'" << endl);
            time_t now;
            time(&now);  /* get current time; same as: timer = time(NULL)  */
            BESDEBUG(MODULE, prolog << "now: " << now << endl);

            // Example: 'Cache-Control: private, max-age=600'
            string max_age_key("max-age=");
            size_t max_age_index = cc_hdr_val.find(max_age_key);
            if(max_age_index != cc_hdr_val.npos){
                string max_age_str = cc_hdr_val.substr(max_age_index + max_age_key.size());
                time_t max_age;
                std::istringstream(max_age_str) >> max_age; // Returns 0 if the parse fails.
                time_t expires = ingest_time() + max_age;
                time_t remaining = expires - now;
                BESDEBUG(MODULE, prolog << "expires: " << expires <<
                                        "  remaining: " << remaining <<
                                        " threshold: " << REFRESH_THRESHOLD << endl);

                expired = remaining < REFRESH_THRESHOLD;
                BESDEBUG(MODULE, prolog << "expired: " << (expired?"true":"false") << endl);
            }
        }
        if(!expired){
            expired = url::is_expired();
        }
        BESDEBUG(MODULE, prolog << "END expired: " << (expired?"true":"false") << endl);
        return expired;
    };




    void EffectiveUrl::get_header(const std::string &name, std::string &value, bool &found ) {
        found = false;
        string lc_name = BESUtil::lowercase(name);
        auto rname_itr = d_response_header_names.rbegin();
        auto rvalue_itr = d_response_header_values.rbegin();
        while(!found && rname_itr != d_response_header_names.rend()){
            string hdr_name = *rname_itr;
            found = (lc_name == hdr_name);
            if(found){
                value = *rvalue_itr;
            }
            ++rname_itr;
            ++rvalue_itr;
        }
    }

    string EffectiveUrl::dump(){
        stringstream ss;
        string indent_inc = "  ";
        string indent = indent_inc;

        ss << url::dump();
        auto name_itr = d_response_header_names.begin();
        auto value_itr = d_response_header_values.begin();
        while(name_itr!=d_response_header_names.end()){
            ss << indent << "Header: " << *name_itr << ": " << *value_itr << endl;
            ++name_itr;
            ++value_itr;
        }
        return ss.str();
    }


    void EffectiveUrl::set_response_headers(const vector<string> &resp_hdrs)
    {
        d_resp_hdr_lines.clear();
        d_resp_hdr_lines = resp_hdrs;
        d_response_header_names.clear();
        d_response_header_values.clear();

        auto index = resp_hdrs.begin();
        while(index!=resp_hdrs.end()){
            size_t colon = (*index).find(":");
            if(colon!=(*index).npos){
                string key((*index).substr(0,colon));
                key = BESUtil::lowercase(key);
                string value((*index).substr(colon));
                d_response_header_names.push_back(key);
                d_response_header_values.push_back(value);
                BESDEBUG(MODULE, prolog << "Ingested header: " << key << ": " << value << "(size: " << d_response_header_values.size() << ")" << endl);
            }
            else {
                ERROR_LOG(prolog << "Encounter malformed response header! Missing ':' delimiter. SKIPPING" << endl);
            }
            index++;
        }
    }




} // namespace http