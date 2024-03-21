// SampleRequestHandler.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
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
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include "config.h"

#include "BESResponseHandler.h"
#include "BESResponseNames.h"
#include "BESVersionInfo.h"
#include "BESTextInfo.h"
#include "BESConstraintFuncs.h"
#include "BESInternalError.h"

#include "SampleRequestHandler.h"
#include "SampleResponseNames.h"

using std::endl;
using std::ostream;
using std::string;
using std::map;

SampleRequestHandler::SampleRequestHandler(const string &name) :
    BESRequestHandler(name)
{
    add_method( VERS_RESPONSE, SampleRequestHandler::sample_build_vers);
    add_method( HELP_RESPONSE, SampleRequestHandler::sample_build_help);
}

SampleRequestHandler::~SampleRequestHandler()
{
}

bool SampleRequestHandler::sample_build_vers(BESDataHandlerInterface &dhi)
{
    bool ret = true;

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESVersionInfo *info = dynamic_cast<BESVersionInfo *>(response);
    if (!info) throw BESInternalError("cast error", __FILE__, __LINE__);
    info->add_module( PACKAGE_NAME, PACKAGE_VERSION);

    return ret;
}

bool SampleRequestHandler::sample_build_help(BESDataHandlerInterface &dhi)
{
    bool ret = true;

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESInfo *info = dynamic_cast<BESInfo *>(response);
    if (!info) throw BESInternalError("cast error", __FILE__, __LINE__);

    map<string, string, std::less<>> attrs;
    attrs["name"] = PACKAGE_NAME;
    attrs["version"] = PACKAGE_VERSION;
    info->begin_tag("module", &attrs);
    info->add_data_from_file("Sample.Help", "Sample Help");
    info->end_tag("module");

    return ret;
}

void SampleRequestHandler::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "SampleRequestHandler::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESRequestHandler::dump(strm);
    BESIndent::UnIndent();
}

