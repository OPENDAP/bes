// BuildDmrppRequestHandler.cc

// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of builddmrpp_module, A C++ module that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.

// Copyright (c) 2023 OPeNDAP, Inc.
// Author: Daniel Holloway <dholloway@opendap.org>
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
//      dan       Daniel Holloway <dholloway@opendap.org>

#include "config.h"

#include <string>
#include <ostream>

#include <libdap/InternalErr.h>

#include <BESResponseHandler.h>
#include <BESResponseNames.h>
#include <BESVersionInfo.h>
#include <BESServiceRegistry.h>
#include <BESUtil.h>

#include "BuildDmrppRequestHandler.h"
#include "NgapNames.h"

using std::endl;
using std::map;
using std::list;
using namespace libdap;
using namespace builddmrpp;

BuildDmrppRequestHandler::BuildDmrppRequestHandler(const string &name) :
        BESRequestHandler(name)
{
    add_method(VERS_RESPONSE, BuildDmrppRequestHandler::mkdmrpp_build_vers);
    add_method(HELP_RESPONSE, BuildDmrppRequestHandler::mkdmrpp_build_help);
}

bool BuildDmrppRequestHandler::mkdmrpp_build_vers(BESDataHandlerInterface &dhi)
{
    bool ret = true;
    auto *info = dynamic_cast<BESVersionInfo *>(dhi.response_handler->get_response_object());
    if (!info) throw InternalErr(__FILE__, __LINE__, "Expected a BESVersionInfo instance");
    info->add_module(MODULE_NAME, MODULE_VERSION);
    return ret;
}

bool BuildDmrppRequestHandler::mkdmrpp_build_help(BESDataHandlerInterface &dhi)
{
    bool ret = true;
    auto *info = dynamic_cast<BESInfo *>(dhi.response_handler->get_response_object());
    if (!info) throw InternalErr(__FILE__, __LINE__, "Expected a BESInfo instance");

    // This is an example. If you had a help file you could load it like
    // this and if your handler handled the following responses.
    map<string, string, std::less<>> attrs;
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

    return ret;
}

void BuildDmrppRequestHandler::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BuildDmrppRequestHandler::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESRequestHandler::dump(strm);
    BESIndent::UnIndent();
}
