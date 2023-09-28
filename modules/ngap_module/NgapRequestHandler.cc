// NgapContainer.cc

// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of ngap_module, A C++ module that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.

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

#include <libdap/InternalErr.h>

#include <BESResponseHandler.h>
#include <BESResponseNames.h>
#include <BESVersionInfo.h>
#include <BESTextInfo.h>
#include "BESDapNames.h"
#include "BESDataDDSResponse.h"
#include "BESDDSResponse.h"
#include "BESDASResponse.h"
#include <BESConstraintFuncs.h>
#include <BESServiceRegistry.h>
#include <TheBESKeys.h>
#include <BESUtil.h>

#include "NgapRequestHandler.h"
#include "NgapNames.h"

using namespace std;
using namespace libdap;
using namespace ngap;

// CMR caching
unsigned int NgapRequestHandler::d_cmr_cache_threshold = 100;
unsigned int NgapRequestHandler::d_cmr_cache_space = 20;

constexpr static auto USE_CMR_CACHE = "NGAP.UseCMRCache";
constexpr static auto CMR_CACHE_THRESHOLD = "NGAP.CMRCacheThreshold";
constexpr static auto CMR_CACHE_SPACE = "NGAP.CMRCacheSpace";

unordered_map<string, string> NgapRequestHandler::d_cmr_cache;
queue<string> NgapRequestHandler::d_cmr_cache_entries;
bool NgapRequestHandler::d_use_cmr_cache = true;

// DMR++ caching
unsigned int NgapRequestHandler::d_dmrpp_cache_threshold = 100;
unsigned int NgapRequestHandler::d_dmrpp_cache_space = 20;

constexpr static auto USE_DMRPP_CACHE = "NGAP.UseDMRppCache";
constexpr static auto DMRPP_CACHE_THRESHOLD = "NGAP.DMRppCacheThreshold";
constexpr static auto DMRPP_CACHE_SPACE = "NGAP.DMRppCacheSpace";

unordered_map<string, string> NgapRequestHandler::d_dmrpp_cache;
queue<string> NgapRequestHandler::d_dmrpp_cache_entries;
bool NgapRequestHandler::d_use_dmrpp_cache = true;

NgapRequestHandler::NgapRequestHandler(const string &name) :
        BESRequestHandler(name)
{
    add_method(VERS_RESPONSE, NgapRequestHandler::ngap_build_vers);
    add_method(HELP_RESPONSE, NgapRequestHandler::ngap_build_help);

    // Read BES keys to determine if the caches should be used. jhrg 9/22/23
    NgapRequestHandler::d_use_cmr_cache 
        = TheBESKeys::TheKeys()->read_bool_key(USE_CMR_CACHE, NgapRequestHandler::d_use_cmr_cache);
    if (NgapRequestHandler::d_use_cmr_cache) {
        NgapRequestHandler::d_cmr_cache_threshold
                = TheBESKeys::TheKeys()->read_int_key(CMR_CACHE_THRESHOLD, NgapRequestHandler::d_cmr_cache_threshold);
        NgapRequestHandler::d_cmr_cache_space
                = TheBESKeys::TheKeys()->read_int_key(CMR_CACHE_SPACE, NgapRequestHandler::d_cmr_cache_space);
    }

    NgapRequestHandler::d_use_dmrpp_cache
            = TheBESKeys::TheKeys()->read_bool_key(USE_DMRPP_CACHE, NgapRequestHandler::d_use_dmrpp_cache);
    if (NgapRequestHandler::d_use_dmrpp_cache) {
        NgapRequestHandler::d_dmrpp_cache_threshold
                = TheBESKeys::TheKeys()->read_int_key(DMRPP_CACHE_THRESHOLD,  NgapRequestHandler::d_dmrpp_cache_threshold);
        NgapRequestHandler::d_dmrpp_cache_space
                = TheBESKeys::TheKeys()->read_int_key(DMRPP_CACHE_SPACE, NgapRequestHandler::d_dmrpp_cache_space);
    }
}

bool NgapRequestHandler::ngap_build_vers(BESDataHandlerInterface &dhi)
{
    auto info = dynamic_cast<BESVersionInfo *>(dhi.response_handler->get_response_object());
    if (!info) throw InternalErr(__FILE__, __LINE__, "Expected a BESVersionInfo instance");

    info->add_module(MODULE_NAME, MODULE_VERSION);
    return true;
}

bool NgapRequestHandler::ngap_build_help(BESDataHandlerInterface &dhi)
{
    auto info = dynamic_cast<BESInfo *>(dhi.response_handler->get_response_object());
    if (!info) throw InternalErr(__FILE__, __LINE__, "Expected a BESInfo instance");

    // This is an example. If you had a help file you could load it like
    // this and if your handler handled the following responses.
    map<string, string> attrs;
    attrs["name"] = MODULE_NAME;
    attrs["version"] = MODULE_VERSION;

    list<string> services;
    BESServiceRegistry::TheRegistry()->services_handled(NGAP_NAME, services);
    if (!services.empty()) {
        string handles = BESUtil::implode(services, ',');
        attrs["handles"] = handles;
    }
    info->begin_tag("module", &attrs);
    info->end_tag("module");

    return true;
}

void NgapRequestHandler::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "NgapRequestHandler::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESRequestHandler::dump(strm);
    BESIndent::UnIndent();
}
