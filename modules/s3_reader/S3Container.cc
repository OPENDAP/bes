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
#include <string>

#include "BESStopWatch.h"
#include "BESLog.h"
#include "BESSyntaxUserError.h"
#include "BESInternalError.h"
#include "BESDebug.h"
#include "BESContextManager.h"
#include "BESUtil.h"
#include "CurlUtils.h"
#include "RemoteResource.h"

#include "S3RequestHandler.h"
#include "S3Container.h"
#include "S3Names.h"

#define prolog std::string("S3Container::").append(__func__).append("() - ")

using namespace std;

namespace s3 {

void S3Container::_duplicate(S3Container &copy_to)
{
    if (d_dmrpp_rresource) {
        throw BESInternalError("The Container has already been accessed, cannot create a copy of this container.",
                               __FILE__, __LINE__);
    }

    copy_to.d_dmrpp_rresource = d_dmrpp_rresource;
    BESContainer::_duplicate(copy_to);
}

void S3Container::initialize()
{
    BESDEBUG(MODULE, prolog << "sym_name: " << get_symbolic_name() << endl);
    BESDEBUG(MODULE, prolog << "real_name: " << get_real_name() << endl);
    BESDEBUG(MODULE, prolog << "type: " << get_container_type() << endl);

    if (get_container_type().empty())
        set_container_type(S3_NAME);

    bool found;
    string uid = BESContextManager::TheManager()->get_context(EDL_UID_KEY, found);
    BESDEBUG(MODULE, prolog << "EDL_UID_KEY(" << EDL_UID_KEY << "): " << uid << endl);

    // Because we know the name is really a URL, then we know the "relative_name" is meaningless
    // So we set it to be the same as "name"
    set_relative_name(get_real_name());
}

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
S3Container::S3Container(const string &sym_name, const string &real_name, const string &type) :
    BESContainer(sym_name, real_name, type)
{
    initialize();
}

BESContainer *
S3Container::ptr_duplicate()
{
    auto container = new S3Container;
    _duplicate(*container);
    return container;
}

/**
 * @brief Filter the cached resource. Each key in content_filters is replaced with its associated map value.
 *
 * WARNING: Does not lock cache. This method assumes that the process has already
 * acquired an exclusive lock on the cache file.
 *
 * WARNING: This method will overwrite the cached data with the filtered result.
 *
 * @param content_filters A map of key value pairs which define the filter operation. Each key found in the
 * resource will be replaced with its associated value.
 */
void S3Container::filter_response(const map<string, string, std::less<>> &content_filters) const {

    string resource_content = BESUtil::file_to_string(d_dmrpp_rresource->get_filename());

    for (const auto &apair: content_filters) {
        unsigned int replace_count = BESUtil::replace_all(resource_content, apair.first, apair.second);
        BESDEBUG(MODULE, prolog << "Replaced " << replace_count << " instance(s) of template(" <<
                                apair.first << ") with " << apair.second << " in cached RemoteResource" << endl);
    }

    // This call will invalidate the file descriptor of the RemoteResource. jhrg 3/9/23
    BESUtil::string_to_file(d_dmrpp_rresource->get_filename(), resource_content);
}

/** @brief access the remote target response by making the remote request
 *
 * @return full path to the remote request response data file
 * @throws BESError if there is a problem making the remote request
 */
string S3Container::access()
{
    if (!d_dmrpp_rresource) {
        BESDEBUG(MODULE, prolog << "Building new RemoteResource (dmr++)." << endl);

        // Since this is S3 we know that the real_name is a URL.
        const string data_access_url_str = get_real_name();

        // And we know that the dmr++ file should be "right next to it" (side-car)
        const string dmrpp_url_str = data_access_url_str + ".dmrpp";

        // And if there's a missing data file (side-car) it should be "right there" too.
        const string missing_data_url_str = data_access_url_str+ ".missing";

        BESDEBUG(MODULE, prolog << " data_access_url: " << data_access_url_str << endl);
        BESDEBUG(MODULE, prolog << "       dmrpp_url: " << dmrpp_url_str << endl);
        BESDEBUG(MODULE, prolog << "missing_data_url: " << missing_data_url_str << endl);

        const string href = R"(href=")";

        const string data_access_url_key = href + DATA_ACCESS_URL_KEY + R"(")";
        const string missing_data_access_url_key = href + MISSING_DATA_ACCESS_URL_KEY + R"(")";

        const string trusted_url_hack = R"(" dmrpp:trust="true")";

        const string data_access_url_with_trusted_attr_str = href + data_access_url_str + trusted_url_hack;
        const string missing_data_url_with_trusted_attr_str = href + missing_data_url_str + trusted_url_hack;

        BESDEBUG(MODULE, prolog << "        data_access_url_key: " << data_access_url_key << endl);
        BESDEBUG(MODULE, prolog << "    data_access_url_trusted: " << data_access_url_with_trusted_attr_str << endl);
        BESDEBUG(MODULE, prolog << "missing_data_access_url_key: " << missing_data_access_url_key << endl);
        BESDEBUG(MODULE, prolog << "   missing_data_url_trusted: " << missing_data_url_with_trusted_attr_str << endl);

        auto dmrpp_url = std::make_shared<http::url>(dmrpp_url_str, true);

        {
            // This scope is here because of the BESStopWatch. jhrg 10/18/22
            //d_dmrpp_rresource = new http::RemoteResource(dmrpp_url);
            d_dmrpp_rresource = std::make_shared<http::RemoteResource>(dmrpp_url);

            BES_STOPWATCH_START(MODULE, prolog + "Timing DMR++ retrieval. Target url: " + dmrpp_url->str());
            d_dmrpp_rresource->retrieve_resource();

            // Substitute the data_access_url and missing_data_access_url in the dmr++ file.
            map<string, string, std::less<>> content_filters;
            if (S3RequestHandler::d_inject_data_url) {
                content_filters.insert(pair<string, string>(data_access_url_key, data_access_url_with_trusted_attr_str));
                content_filters.insert(pair<string, string>(missing_data_access_url_key, missing_data_url_with_trusted_attr_str));
            }

            filter_response(content_filters);
        }

        BESDEBUG(MODULE, prolog << "Done retrieving:  " << dmrpp_url->str() << " returning cached file "
                                << d_dmrpp_rresource->get_filename() << endl);
    }

    const auto type = d_dmrpp_rresource->get_type();
    set_container_type(type);

    BESDEBUG(MODULE, prolog << "Type: " << type << endl);
    BESDEBUG(MODULE, prolog << "END  (obj_addr: " << (void *) this << ")" << endl);

    return d_dmrpp_rresource->get_filename();    // this should return the dmr++ file name for the temporary file
}

/**
 * @brief release the resources
 *
 * Release the resource
 *
 * @return true if the resource is released successfully and false otherwise
 */
bool S3Container::release()
{
    if (d_dmrpp_rresource) {
        // delete d_dmrpp_rresource;
        d_dmrpp_rresource = nullptr;
    }

    return true;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * this container.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void S3Container::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "S3Container::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESContainer::dump(strm);
    if (d_dmrpp_rresource) {
        strm << BESIndent::LMarg << "RemoteResource.getCacheFileName(): " << d_dmrpp_rresource->get_filename()
             << endl;
        strm << BESIndent::LMarg << "response headers: ";
    }
    else {
        strm << BESIndent::LMarg << "response not yet obtained" << endl;
    }
    BESIndent::UnIndent();
}

} // s3 namespace
