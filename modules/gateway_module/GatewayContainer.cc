// GatewayContainer.cc

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

// Authors:
//      pcw       Patrick West <pwest@ucar.edu>

#include <BESSyntaxUserError.h>
#include <BESInternalError.h>
#include <BESDebug.h>
#include <BESUtil.h>
#include <TheBESKeys.h>
#include <RemoteAccess.h>

#include "GatewayContainer.h"
#include "GatewayUtils.h"
#include "GatewayResponseNames.h"
#include "RemoteHttpResource.h"

using namespace std;
using namespace gateway;
using bes::RemoteAccess;

/** @brief Creates an instances of GatewayContainer with symbolic name and real
 * name, which is the remote request.
 *
 * The real_name is the remote request URL.
 *
 * @param sym_name symbolic name representing this remote container
 * @param real_name the remote request URL
 * @throws BESSyntaxUserError if the url does not validate
 * @see GatewayUtils
 */
GatewayContainer::GatewayContainer(const string &sym_name,
        const string &real_name, const string &type) :
        BESContainer(sym_name, real_name, type), d_remoteResource(0) {

    if (type.empty())
        set_container_type("gateway");

    BESUtil::url url_parts;
    BESUtil::url_explode(real_name, url_parts);
    url_parts.uname = "";
    url_parts.psswd = "";
    string use_real_name = BESUtil::url_create(url_parts);

    if (!RemoteAccess::Is_Whitelisted(use_real_name)) {
        string err = (string) "The specified URL " + real_name
                + " does not match any of the accessible services in"
                + " the white list.";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }

    // Because we know the name is really a URL, then we know the "relative_name" is meaningless
    // So we set it to be the same as "name"
    set_relative_name(real_name);
}

/**
 * TODO: I think this implementation of the copy constructor is incomplete/inadequate. Review and fix as needed.
 */
GatewayContainer::GatewayContainer(const GatewayContainer &copy_from) :
        BESContainer(copy_from), d_remoteResource(copy_from.d_remoteResource) {
    // we can not make a copy of this container once the request has
    // been made
    if (d_remoteResource) {
        string err = (string) "The Container has already been accessed, "
                + "can not create a copy of this container.";
        throw BESInternalError(err, __FILE__, __LINE__);
    }
}

void GatewayContainer::_duplicate(GatewayContainer &copy_to) {
    if (copy_to.d_remoteResource) {
        string err = (string) "The Container has already been accessed, "
                + "can not duplicate this resource.";
        throw BESInternalError(err, __FILE__, __LINE__);
    }
    copy_to.d_remoteResource = d_remoteResource;
    BESContainer::_duplicate(copy_to);
}

BESContainer *
GatewayContainer::ptr_duplicate() {
    GatewayContainer *container = new GatewayContainer;
    _duplicate(*container);
    return container;
}

GatewayContainer::~GatewayContainer() {
    if (d_remoteResource) {
        release();
    }
}

/** @brief access the remote target response by making the remote request
 *
 * @return full path to the remote request response data file
 * @throws BESError if there is a problem making the remote request
 */
string GatewayContainer::access() {

    BESDEBUG( "gateway", "GatewayContainer::access() - BEGIN" << endl);

    // Since this the Gateway we know that the real_name is a URL.
    string url  = get_real_name();

    BESDEBUG( "gateway", "GatewayContainer::access() - Accessing " << url << endl);

    string type = get_container_type();
    if (type == "gateway")
        type = "";

    if(!d_remoteResource) {
        BESDEBUG( "gateway", "GatewayContainer::access() - Building new RemoteResource." << endl );
        d_remoteResource = new gateway::RemoteHttpResource(url);
        d_remoteResource->retrieveResource();
    }
    BESDEBUG( "gateway", "GatewayContainer::access() - Located remote resource." << endl );


    string cachedResource = d_remoteResource->getCacheFileName();
    BESDEBUG( "gateway", "GatewayContainer::access() - Using local cache file: " << cachedResource << endl );

    type = d_remoteResource->getType();
    set_container_type(type);
    BESDEBUG( "gateway", "GatewayContainer::access() - Type: " << type << endl );


    BESDEBUG( "gateway", "GatewayContainer::access() - Done accessing " << get_real_name() << " returning cached file " << cachedResource << endl);
    BESDEBUG( "gateway", "GatewayContainer::access() - Done accessing " << *this << endl);
    BESDEBUG( "gateway", "GatewayContainer::access() - END" << endl);

    return cachedResource;    // this should return the file name from the GatewayCache
}



/** @brief release the resources
 *
 * Release the resource
 *
 * @return true if the resource is released successfully and false otherwise
 */
bool GatewayContainer::release() {
    if (d_remoteResource) {
        BESDEBUG( "gateway", "GatewayContainer::release() - Releasing RemoteResource" << endl);
        delete d_remoteResource;
        d_remoteResource = 0;
    }

    BESDEBUG( "gateway", "done releasing gateway response" << endl);
    return true;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * this container.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void GatewayContainer::dump(ostream &strm) const {
    strm << BESIndent::LMarg << "GatewayContainer::dump - (" << (void *) this
            << ")" << endl;
    BESIndent::Indent();
    BESContainer::dump(strm);
    if (d_remoteResource) {
        strm << BESIndent::LMarg << "RemoteResource.getCacheFileName(): " << d_remoteResource->getCacheFileName()
                << endl;
        strm << BESIndent::LMarg << "response headers: ";
        vector<string> *hdrs = d_remoteResource->getResponseHeaders();
        if (hdrs) {
            strm << endl;
            BESIndent::Indent();
            vector<string>::const_iterator i = hdrs->begin();
            vector<string>::const_iterator e = hdrs->end();
            for (; i != e; i++) {
                string hdr_line = (*i);
                strm << BESIndent::LMarg << hdr_line << endl;
            }
            BESIndent::UnIndent();
        } else {
            strm << "none" << endl;
        }
    } else {
        strm << BESIndent::LMarg << "response not yet obtained" << endl;
    }
    BESIndent::UnIndent();
}
