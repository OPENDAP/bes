// GatewayRequestHandler.cc

// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of gateway_module, A C++ module that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
// Author: Patrick West <pwest@ucar.edu>
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

#include "config.h"

#include <libdap/InternalErr.h>

#include <BESResponseHandler.h>
#include <BESResponseNames.h>
#include <BESVersionInfo.h>
#include <BESConstraintFuncs.h>
#include <BESServiceRegistry.h>
#include <BESUtil.h>

#include "GatewayRequestHandler.h"
#include "GatewayNames.h"

using std::endl;
using std::map;
using std::list;
using namespace libdap;
using namespace gateway;

GatewayRequestHandler::GatewayRequestHandler(const string &name) :
    BESRequestHandler(name)
{
    add_method(VERS_RESPONSE, GatewayRequestHandler::gateway_build_vers);
    add_method(HELP_RESPONSE, GatewayRequestHandler::gateway_build_help);
}

GatewayRequestHandler::~GatewayRequestHandler()
{
}

bool GatewayRequestHandler::gateway_build_vers(BESDataHandlerInterface &dhi)
{
    bool ret = true;
    BESVersionInfo *info = dynamic_cast<BESVersionInfo *>(dhi.response_handler->get_response_object());
    if (!info) throw InternalErr(__FILE__, __LINE__, "Expected a BESVersionInfo instance");
#if 0
    info->add_module(PACKAGE_NAME, PACKAGE_VERSION);
#endif
    info->add_module(GATEWAY_MODULE, GATEWAY_MODULE_VERSION);
    return ret;
}

bool GatewayRequestHandler::gateway_build_help(BESDataHandlerInterface &dhi)
{
    bool ret = true;
    BESInfo *info = dynamic_cast<BESInfo *>(dhi.response_handler->get_response_object());
    if (!info) throw InternalErr(__FILE__, __LINE__, "Expected a BESInfo instance");

    // This is an example. If you had a help file you could load it like
    // this and if your handler handled the following responses.
    map<string, string, std::less<>> attrs;
    attrs["name"] = GATEWAY_MODULE;
    attrs["version"] = GATEWAY_MODULE_VERSION;
#if 0
    attrs["name"] = PACKAGE_NAME;
    attrs["version"] = PACKAGE_VERSION;
#endif
    list<string> services;
    BESServiceRegistry::TheRegistry()->services_handled(GATEWAY_MODULE, services);
    if (services.size() > 0) {
        string handles = BESUtil::implode(services, ',');
        attrs["handles"] = handles;
    }
    info->begin_tag("module", &attrs);
    //info->add_data_from_file( "Gateway.Help", "Gateway Help" ) ;
    info->end_tag("module");

    return ret;
}

void GatewayRequestHandler::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "GatewayRequestHandler::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESRequestHandler::dump(strm);
    BESIndent::UnIndent();
}
