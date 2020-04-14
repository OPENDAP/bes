// -*- mode: c++; c-basic-offset:4 -*-
//
// This file is part of cnr_module, A C++ module that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.
//
// Copyright (c) 2018 OPeNDAP, Inc.
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

#include <BESSyntaxUserError.h>
#include <BESInternalError.h>
#include <BESNotFoundError.h>
#include <BESDebug.h>
#include <BESUtil.h>
#include <TheBESKeys.h>
#include <WhiteList.h>

#include "HttpdCatalogContainer.h"
#include "BESRemoteUtils.h"
#include "BESProxyNames.h"
#include "HttpdCatalog.h"
#include "BESRemoteHttpResource.h"

using namespace std;
using namespace bes;

#define prolog std::string("HttpdCatalogContainer::").append(__func__).append("() - ")

namespace httpd_catalog {

/** @brief Creates an instances of CmrContainer with symbolic name and real
 * name, which is the remote request.
 *
 * The real_name is the remote request URL.
 *
 * @param sym_name symbolic name representing this remote container
 * @param real_name the virtual  path to a dataset or file.
 * @throws BESSyntaxUserError if the path does not validate
 * @see CmrUtils
 */
HttpdCatalogContainer::HttpdCatalogContainer(const string &sym_name, const string &real_name, const string &type) :
    BESContainer(sym_name, real_name, type), d_remoteResource(0)
{

    BESDEBUG(MODULE, prolog << "BEGIN sym_name: " << sym_name << " real_name: " << real_name << " type: " << type << endl);

    string path = real_name;
    if (path.empty() || path[0] != '/') {
        path = "/" + path;
    }

#if 0
    // unused
    vector<string> path_elements = BESUtil::split(path);
    BESDEBUG(MODULE, prolog << "path: '" << path << "'  path_elements.size(): " << path_elements.size() << endl);
#endif


    set_relative_name(path);

    // The container type is set in the access() method when the remote resource is accessed using the
    // MIME type information using mappings between handlers (e.g., 'h5') and MIME types like application/x-hdf5.
    // However, bes/dispatchBESContainerStorageVolatile::add_container(BESContainer *) expects the field
    // to be not empty, so I'll add a place holder value. jhrg 1/25/19
    if (type == "")
         this->set_container_type("place_holder");

    BESDEBUG(MODULE, prolog << "END" << endl);
}

HttpdCatalogContainer::HttpdCatalogContainer(const HttpdCatalogContainer &copy_from) :
    BESContainer(copy_from), d_remoteResource(0)
{
    // we can not make a copy of this container once the request has
    // been made
    if (copy_from.d_remoteResource) {
        throw BESInternalError("The Container has already been accessed, cannot create a copy of this container.", __FILE__, __LINE__);
    }
}

void HttpdCatalogContainer::_duplicate(HttpdCatalogContainer &copy_to)
{
    if (copy_to.d_remoteResource) {
        throw BESInternalError("The Container has already been accessed, cannot duplicate this resource.", __FILE__, __LINE__);
    }
    copy_to.d_remoteResource = d_remoteResource;
    BESContainer::_duplicate(copy_to);
}

BESContainer *
HttpdCatalogContainer::ptr_duplicate()
{
    HttpdCatalogContainer *container = new HttpdCatalogContainer;
    _duplicate(*container);
    return container;
}

HttpdCatalogContainer::~HttpdCatalogContainer()
{
    if (d_remoteResource) {
        release();
    }
}

/** @brief access the remote target response by making the remote request
 *
 * @return full path to the remote request response data file
 * @throws BESError if there is a problem making the remote request
 */
string HttpdCatalogContainer::access()
{
    BESDEBUG(MODULE, prolog << "BEGIN" << endl);

    string path = get_real_name();
    BESDEBUG(MODULE, prolog << "path: " << path << endl);

    HttpdCatalog hc;
    string access_url = hc.path_to_access_url(path);

    if (!d_remoteResource) {
        BESDEBUG(MODULE, prolog << "Building new RemoteResource." << endl);
        d_remoteResource = new remote_http_resource::BESRemoteHttpResource(access_url);
        d_remoteResource->retrieveResource();
    }

    BESDEBUG(MODULE, prolog << "Located remote resource." << endl);

    string cachedResource = d_remoteResource->getCacheFileName();
    BESDEBUG(MODULE, prolog << "Using local cache file: " << cachedResource << endl);

    string type = d_remoteResource->getType();
    set_container_type(type);

    BESDEBUG(MODULE, prolog << "Type: " << type << endl);

    BESDEBUG(MODULE, prolog << "Done accessing " << get_real_name() << " returning cached file " << cachedResource << endl);
    BESDEBUG(MODULE, prolog << "Done accessing " << *this << endl);
    BESDEBUG(MODULE, prolog << "END" << endl);

    return cachedResource; // this should return the file name from the CmrCache
}

/** @brief release the resources
 *
 * Release the resource
 *
 * @return true if the resource is released successfully and false otherwise
 */
bool HttpdCatalogContainer::release()
{
    BESDEBUG(MODULE, prolog << "BEGIN" << endl);
    if (d_remoteResource) {
        BESDEBUG(MODULE, prolog << "Releasing RemoteResource" << endl);
        delete d_remoteResource;
        d_remoteResource = 0;
    }
    BESDEBUG(MODULE, prolog << "END" << endl);
    return true;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * this container.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void HttpdCatalogContainer::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << prolog<<"(" << (void *) this
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
        }
        else {
            strm << "none" << endl;
        }
    }
    else {
        strm << BESIndent::LMarg << "response not yet obtained" << endl;
    }

    BESIndent::UnIndent();
}

} // namespace http_catalog
