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

#if 0
#include <BESFileLockingCache.h>
#endif

#include <BESUtil.h>

#include "NgapRequestHandler.h"
#include "NgapNames.h"

using namespace std;
using namespace libdap;
using namespace ngap;

unordered_map<std::string, std::string> NgapRequestHandler::d_translated_urls;
bool NgapRequestHandler::d_use_cmr_cache = true;

#if 0
BESFileLockingCache NgapRequestHandler::d_dmrpp_file_cache;
bool NgapRequestHandler::d_use_dmrpp_file_cache = true;
#endif


NgapRequestHandler::NgapRequestHandler(const string &name) :
        BESRequestHandler(name)
{
    add_method(VERS_RESPONSE, NgapRequestHandler::ngap_build_vers);
    add_method(HELP_RESPONSE, NgapRequestHandler::ngap_build_help);

#if 0
    // TODO Replace these hard-coded values jhrg 9/20/23
    NgapRequestHandler::d_dmrpp_file_cache.initialize("/tmp/ngap-dmrpp", "dmrpp:", 100 /* MB */);
#endif
}

NgapRequestHandler::~NgapRequestHandler()
{
}

bool NgapRequestHandler::ngap_build_vers(BESDataHandlerInterface &dhi)
{
    bool ret = true;
    BESVersionInfo *info = dynamic_cast<BESVersionInfo *>(dhi.response_handler->get_response_object());
    if (!info) throw InternalErr(__FILE__, __LINE__, "Expected a BESVersionInfo instance");
#if 0
    info->add_module(PACKAGE_NAME, PACKAGE_VERSION);
#endif
    info->add_module(MODULE_NAME, MODULE_VERSION);
    return ret;
}

bool NgapRequestHandler::ngap_build_help(BESDataHandlerInterface &dhi)
{
    bool ret = true;
    BESInfo *info = dynamic_cast<BESInfo *>(dhi.response_handler->get_response_object());
    if (!info) throw InternalErr(__FILE__, __LINE__, "Expected a BESInfo instance");

    // This is an example. If you had a help file you could load it like
    // this and if your handler handled the following responses.
    map<string, string> attrs;
    attrs["name"] = MODULE_NAME;
    attrs["version"] = MODULE_VERSION;
#if 0
    attrs["name"] = PACKAGE_NAME;
    attrs["version"] = PACKAGE_VERSION;
#endif
    list<string> services;
    BESServiceRegistry::TheRegistry()->services_handled(NGAP_NAME, services);
    if (services.size() > 0) {
        string handles = BESUtil::implode(services, ',');
        attrs["handles"] = handles;
    }
    info->begin_tag("module", &attrs);
    //info->add_data_from_file( "Ngap.Help", "Ngap Help" ) ;
    info->end_tag("module");

    return ret;
}

void NgapRequestHandler::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "NgapRequestHandler::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESRequestHandler::dump(strm);
    BESIndent::UnIndent();
}
