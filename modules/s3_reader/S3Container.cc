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

#include <map>
#include <sstream>
#include <string>

#include "BESStopWatch.h"
#include "BESLog.h"
#include "BESSyntaxUserError.h"
#include "BESInternalError.h"
#include "BESDebug.h"
#include "TheBESKeys.h"
#include "BESContextManager.h"
#include "CurlUtils.h"
#include "RemoteResource.h"
#include "url_impl.h"

#include "S3Container.h"
#include "S3Names.h"

#define prolog std::string("S3Container::").append(__func__).append("() - ")

using namespace std;

namespace s3 {

/** @brief Creates an instances of S3Container with symbolic name and real
 * name, which is the remote request.
 *
 * The real_name is the remote request URL.
 *
 * @param sym_name symbolic name representing this remote container
 * @param real_name The S3 restified path.
 * @throws BESSyntaxUserError if the url does not validate
 * @see S3Utils
 */
S3Container::S3Container(const string &sym_name,
                             const string &real_name,
                             const string &type) :
        BESContainer(sym_name, real_name, type),
        d_dmrpp_rresource(nullptr) {
    initialize();
}


void S3Container::initialize()
{
    BESDEBUG(MODULE, prolog << "sym_name: "<< get_symbolic_name() << endl);
    BESDEBUG(MODULE, prolog << "real_name: "<< get_real_name() << endl);
    BESDEBUG(MODULE, prolog << "type: "<< get_container_type() << endl);

    bool found;

    if (get_container_type().empty())
        set_container_type("S3");

    string uid = BESContextManager::TheManager()->get_context(EDL_UID_KEY, found);
    BESDEBUG(MODULE, prolog << "EDL_UID_KEY(" << EDL_UID_KEY << "): " << uid << endl);

#if 0
    string data_access_url = S3_api.convert_S3_resty_path_to_data_access_url(get_real_name(), uid);

    set_real_name(data_access_url);
#endif
    // Because we know the name is really a URL, then we know the "relative_name" is meaningless
    // So we set it to be the same as "name"
    set_relative_name(get_real_name());
}

/**
 * TODO: I think this implementation of the copy constructor is incomplete/inadequate. Review and fix as needed.
 */
S3Container::S3Container(const S3Container &copy_from) :
        BESContainer(copy_from),
        d_dmrpp_rresource(copy_from.d_dmrpp_rresource) {
    BESDEBUG(MODULE, prolog << "BEGIN   object address: "<< (void *) this << " Copying from: " << (void *) &copy_from << endl);
    // we can not make a copy of this container once the request has
    // been made
    if (d_dmrpp_rresource) {
        string err = (string) "The Container has already been accessed, "
                     + "can not create a copy of this container.";
        throw BESInternalError(err, __FILE__, __LINE__);
    }
    BESDEBUG(MODULE, prolog << "object address: "<< (void *) this << endl);
}

void S3Container::_duplicate(S3Container &copy_to) {
    if (copy_to.d_dmrpp_rresource) {
        string err = (string) "The Container has already been accessed, "
                     + "can not duplicate this resource.";
        throw BESInternalError(err, __FILE__, __LINE__);
    }
    BESDEBUG(MODULE, prolog << "BEGIN   object address: "<< (void *) this << " Copying to: " << (void *) &copy_to << endl);
    copy_to.d_dmrpp_rresource = d_dmrpp_rresource;
    BESContainer::_duplicate(copy_to);
}

BESContainer *
S3Container::ptr_duplicate() {
    S3Container *container = new S3Container;
    _duplicate(*container);
    BESDEBUG(MODULE, prolog << "object address: "<< (void *) this << " to: " << (void *)container << endl);
    return container;
}

S3Container::~S3Container() {
    BESDEBUG(MODULE, prolog << "BEGIN  object address: "<< (void *) this <<  endl);
    if (d_dmrpp_rresource) {
        release();
    }
    BESDEBUG(MODULE, prolog << "END  object address: "<< (void *) this <<  endl);
}



/** @brief access the remote target response by making the remote request
 *
 * @return full path to the remote request response data file
 * @throws BESError if there is a problem making the remote request
 */
string S3Container::access() {
    BESDEBUG(MODULE, prolog << "BEGIN  (obj_addr: "<< (void *) this << ")" << endl);

    // Since this the S3 we know that the real_name is a URL.
    string data_access_url_str = get_real_name();

    // And we know that the dmr++ file should "right next to it" (side-car)
    string dmrpp_url_str = data_access_url_str + ".dmrpp";

    // And if there's a missing data file (side-car) it should be "right there" too.
    string missing_data_url_str = data_access_url_str + ".missing";

    BESDEBUG(MODULE, prolog << " data_access_url: " << data_access_url_str << endl);
    BESDEBUG(MODULE, prolog << "       dmrpp_url: " << dmrpp_url_str << endl);
    BESDEBUG(MODULE, prolog << "missing_data_url: " << missing_data_url_str << endl);

    string href="href=\"";
    string trusted_url_hack="\" dmrpp:trust=\"true\"";

    string data_access_url_key = href + DATA_ACCESS_URL_KEY + "\"";
    BESDEBUG(MODULE, prolog << "                   data_access_url_key: " << data_access_url_key << endl);

    string data_access_url_with_trusted_attr_str = href + data_access_url_str + trusted_url_hack;
    BESDEBUG(MODULE, prolog << " data_access_url_with_trusted_attr_str: " << data_access_url_with_trusted_attr_str << endl);

    string missing_data_access_url_key = href + MISSING_DATA_ACCESS_URL_KEY + "\"";
    BESDEBUG(MODULE, prolog << "           missing_data_access_url_key: " << missing_data_access_url_key << endl);

    string missing_data_url_with_trusted_attr_str = href + missing_data_url_str + trusted_url_hack;
    BESDEBUG(MODULE, prolog << "missing_data_url_with_trusted_attr_str: " << missing_data_url_with_trusted_attr_str << endl);

    string type = get_container_type();
    if (type == "S3")
        type = "";

    if (!d_dmrpp_rresource) {
        BESDEBUG(MODULE, prolog << "Building new RemoteResource (dmr++)." << endl);
        map<string,string> content_filters;
        if (inject_data_url()) {
            content_filters.insert(pair<string,string>(data_access_url_key, data_access_url_with_trusted_attr_str));
            content_filters.insert(pair<string,string>(missing_data_access_url_key, missing_data_url_with_trusted_attr_str));
        }
        shared_ptr<http::url> dmrpp_url(new http::url(dmrpp_url_str, true));
        {
            d_dmrpp_rresource = new http::RemoteResource(dmrpp_url);
            BESStopWatch besTimer;
            if (BESISDEBUG(MODULE) || BESDebug::IsSet(TIMING_LOG_KEY) || BESLog::TheLog()->is_verbose()){
                besTimer.start("DMR++ retrieval: "+ dmrpp_url->str());
            }
            d_dmrpp_rresource->retrieveResource(content_filters);
        }
        BESDEBUG(MODULE, prolog << "Retrieved remote resource: " << dmrpp_url->str() << endl);
    }

    // TODO This file should be read locked before leaving this method.
    //  10/8/21 I think the RemoteResource should do that. jhrg
    string cachedResource = d_dmrpp_rresource->getCacheFileName();
    BESDEBUG(MODULE, prolog << "Using local cache file: " << cachedResource << endl);

    type = d_dmrpp_rresource->getType();
    set_container_type(type);
    BESDEBUG(MODULE, prolog << "Type: " << type << endl);
    BESDEBUG(MODULE, prolog << "Done retrieving:  " << dmrpp_url_str << " returning cached file " << cachedResource << endl);
    BESDEBUG(MODULE, prolog << "END  (obj_addr: "<< (void *) this << ")" << endl);

    return cachedResource;    // this should return the dmr++ file name from the S3Cache
}


/** @brief release the resources
 *
 * Release the resource
 *
 * @return true if the resource is released successfully and false otherwise
 */
bool S3Container::release() {
    // TODO The cache file (that will be) read locked in the access() method must be unlocked here.
    //  If we make that part of the RemoteResource dtor, the unlock will happen here. jhrg
    if (d_dmrpp_rresource) {
        BESDEBUG(MODULE, prolog << "Releasing RemoteResource" << endl);
        delete d_dmrpp_rresource;
        d_dmrpp_rresource = 0;
    }

    BESDEBUG(MODULE, prolog << "Done releasing S3 response" << endl);
    return true;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * this container.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void S3Container::dump(ostream &strm) const {
    strm << BESIndent::LMarg << "S3Container::dump - (" << (void *) this
         << ")" << endl;
    BESIndent::Indent();
    BESContainer::dump(strm);
    if (d_dmrpp_rresource) {
        strm << BESIndent::LMarg << "RemoteResource.getCacheFileName(): " << d_dmrpp_rresource->getCacheFileName()
             << endl;
        strm << BESIndent::LMarg << "response headers: ";
        vector<string> *hdrs = d_dmrpp_rresource->getResponseHeaders();
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

bool S3Container::inject_data_url(){
    bool result = false;
    bool found;
    string key_value;
    TheBESKeys::TheKeys()->get_value(S3_INJECT_DATA_URL_KEY, key_value, found);
    if (found && key_value == "true") {
        result = true;
    }
    BESDEBUG(MODULE, prolog << "S3_INJECT_DATA_URL_KEY(" << S3_INJECT_DATA_URL_KEY << "): " << result << endl);
    return result;
}
}
