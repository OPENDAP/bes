// S3Container.cc

// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of S3_module, A C++ module that can be loaded in to
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

#include <BESServiceRegistry.h>
#include <TheBESKeys.h>
#include <BESUtil.h>

#include "S3RequestHandler.h"
#include "S3Names.h"

using namespace std;
using namespace libdap;
using namespace s3;

bool S3RequestHandler::d_inject_data_url;

S3RequestHandler::S3RequestHandler(const string &name) : BESRequestHandler(name)
{
    add_method(VERS_RESPONSE, S3RequestHandler::S3_build_vers);
    add_method(HELP_RESPONSE, S3RequestHandler::S3_build_help);

    d_inject_data_url = TheBESKeys::TheKeys()->read_bool_key(S3_INJECT_DATA_URL_KEY, false);
}

bool S3RequestHandler::S3_build_vers(BESDataHandlerInterface &dhi)
{
    auto info = dynamic_cast<BESVersionInfo *>(dhi.response_handler->get_response_object());
    if (!info) throw InternalErr(__FILE__, __LINE__, "Expected a BESVersionInfo instance");

    info->add_module(MODULE_NAME, MODULE_VERSION);
    return true;
}

bool S3RequestHandler::S3_build_help(BESDataHandlerInterface &dhi)
{
    auto info = dynamic_cast<BESInfo *>(dhi.response_handler->get_response_object());
    if (!info) throw InternalErr(__FILE__, __LINE__, "Expected a BESInfo instance");

    // This is an example. If you had a help file you could load it like
    // this and if your handler handled the following responses.
    map<string, string, std::less<>> attrs;
    attrs["name"] = MODULE_NAME;
    attrs["version"] = MODULE_VERSION;

    list<string> services;
    BESServiceRegistry::TheRegistry()->services_handled(S3_NAME, services);
    if (!services.empty()) {
        string handles = BESUtil::implode(services, ',');
        attrs["handles"] = handles;
    }
    info->begin_tag("module", &attrs);

    info->end_tag("module");

    return true;
}

void S3RequestHandler::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "S3RequestHandler::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESRequestHandler::dump(strm);
    BESIndent::UnIndent();
}
