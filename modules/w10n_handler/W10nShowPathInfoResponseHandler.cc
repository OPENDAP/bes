// -*- mode: c++; c-basic-offset:4 -*-
//
// W10NResponseHandler.cc
//
// This file is part of BES w10n handler
//
// Copyright (c) 2015v OPeNDAP, Inc.
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
// Please read the full copyright statement in the file COPYRIGHT_URI.
//

#include "config.h"

#include "W10nShowPathInfoResponseHandler.h"
#include "W10nShowPathInfoCommand.h"
#include "W10NNames.h"
#include "w10n_utils.h"
#include "BESDebug.h"

#include "BESInfoList.h"
#include "BESInfo.h"
#include "BESRequestHandlerList.h"
#include "BESRequestHandler.h"
#include "BESNames.h"
#include "BESDapNames.h"
#include "BESDataNames.h"
#include "BESCatalogList.h"
#include "BESCatalog.h"
#include "BESCatalogEntry.h"
#include "BESCatalogUtils.h"
#include "BESSyntaxUserError.h"
#include "BESNotFoundError.h"
#include "BESStopWatch.h"

#define W10N_PATH_INFO_RESPONSE "W10nPathInfo"

#define PATH "path"
#define VALID_PATH "validPath"
#define REMAINDER  "remainder"
#define IS_DATA "isData"
#define IS_FILE "isFile"
#define IS_DIR  "isDir"

#define prolog string("W10nJsonTransmitter::").append(__func__).append("() - ")

W10nShowPathInfoResponseHandler::W10nShowPathInfoResponseHandler(const string &name) :
    BESResponseHandler(name)
{
}

W10nShowPathInfoResponseHandler::~W10nShowPathInfoResponseHandler()
{
}

/** @brief executes the command 'show catalog|leaves [for &lt;node&gt;];' by
 * returning nodes or leaves at the top level or at the specified node.
 *
 * The response object BESInfo is created to store the information.
 *
 * @param dhi structure that holds request and response information
 * @see BESDataHandlerInterface
 * @see BESInfo
 * @see BESRequestHandlerList
 */
void W10nShowPathInfoResponseHandler::execute(BESDataHandlerInterface &dhi)
{
    BES_STOPWATCH_START_DHI(W10N_DEBUG_KEY, prolog + "Timing", &dhi);

    BESDEBUG(W10N_DEBUG_KEY, prolog << "BEGIN" << endl );

    BESInfo *info = BESInfoList::TheList()->build_info();
    d_response_object = info;

    string container = dhi.data[CONTAINER];
#if 0
    string catname;
    string defcatname = BESCatalogList::TheCatalogList()->default_catalog_name();
#endif

    BESCatalog *defcat = BESCatalogList::TheCatalogList()->default_catalog();
    if (!defcat)
        throw BESInternalError("Not able to find the default catalog.", __FILE__, __LINE__);

    // remove all of the leading slashes from the container name
    string::size_type notslash = container.find_first_not_of("/", 0);
    if (notslash != string::npos) {
        container = container.substr(notslash);
    }

    // see if there is a catalog name here. It's only a possible catalog name
    string catname;
    string::size_type slash = container.find_first_of("/", 0);
    if (slash != string::npos) {
        catname = container.substr(0, slash);
    }
    else {
        catname = container;
    }

    // see if this catalog exists. If it does, then remove the catalog
    // name from the container (node)
    BESCatalog *catobj = BESCatalogList::TheCatalogList()->find_catalog(catname);
    if (catobj) {
        if (slash != string::npos) {
            container = container.substr(slash + 1);

            // remove repeated slashes
            notslash = container.find_first_not_of("/", 0);
            if (notslash != string::npos) {
                container = container.substr(notslash);
            }
        }
        else {
            container = "";
        }
    }

    if (container.empty()) container = "/";

    BESDEBUG(W10N_DEBUG_KEY, prolog << "w10n_id: " << container << endl );

    info->begin_response(W10N_SHOW_PATH_INFO_REQUEST, dhi);
    //string coi = dhi.data[CATALOG_OR_INFO];

    map<string, string, std::less<>> pathInfoAttrs;
    pathInfoAttrs[PATH] = container;

    info->begin_tag(W10N_PATH_INFO_RESPONSE, &pathInfoAttrs);

    string validPath, remainder;
    bool isFile, isDir;

    BESCatalogUtils *utils = BESCatalogList::TheCatalogList()->default_catalog()->get_catalog_utils();
    w10n::eval_resource_path(container, utils->get_root_dir(), utils->follow_sym_links(), validPath, isFile, isDir,
        remainder);

    // Now that we know what part of the path is actually something
    // we can access, find out if the BES sees it as a dataset
    bool isData = false;

    // If the valid path is an empty string then we KNOW it's not a dataset
    if (validPath.size() != 0) {

        // Get the catalog entry.
        BESCatalogEntry *entry = 0;
        //string coi = dhi.data[CATALOG];
        entry = defcat->show_catalog(validPath, /*coi,*/entry);
        if (!entry) {
            string err = (string) "Failed to find the validPath node " + validPath
                + " this should not be possible. Some thing BAD is happening.";
            throw BESInternalError(err, __FILE__, __LINE__);
        }

        // Retrieve the valid services list
        list<string> services = entry->get_service_list();

        // See if there's an OPENDAP_SERVICE available for the node.
        if (services.size()) {
            list<string>::const_iterator si = services.begin();
            list<string>::const_iterator se = services.end();
            for (; si != se; si++) {
                if ((*si) == OPENDAP_SERVICE) isData = true;
            }
        }
    }

    map<string, string, std::less<>> validPathAttrs;
    validPathAttrs[IS_DATA] = isData ? "true" : "false";
    validPathAttrs[IS_FILE] = isFile ? "true" : "false";
    validPathAttrs[IS_DIR] = isDir ? "true" : "false";

    info->add_tag(VALID_PATH, validPath, &validPathAttrs);
    info->add_tag(REMAINDER, remainder);

    info->end_tag(W10N_PATH_INFO_RESPONSE);

    // end the response object
    info->end_response();

    BESDEBUG(W10N_DEBUG_KEY, prolog << "END" << endl );
    }

    /** @brief transmit the response object built by the execute command
     * using the specified transmitter object
     *
     * If a response object was built then transmit it as text
     *
     * @param transmitter object that knows how to transmit specific basic types
     * @param dhi structure that holds the request and response information
     * @see BESInfo
     * @see BESTransmitter
     * @see BESDataHandlerInterface
     */
void W10nShowPathInfoResponseHandler::transmit(BESTransmitter *transmitter, BESDataHandlerInterface &dhi)
{
    if (d_response_object) {
        BESInfo *info = dynamic_cast<BESInfo *>(d_response_object);
        if (!info) throw BESInternalError("cast error", __FILE__, __LINE__);
        info->transmit(transmitter, dhi);
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void W10nShowPathInfoResponseHandler::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << prolog << "(" << (void *) this << ")" << std::endl;
    BESIndent::Indent();
    BESResponseHandler::dump(strm);
    BESIndent::UnIndent();
}

BESResponseHandler *
W10nShowPathInfoResponseHandler::W10nShowPathInfoResponseBuilder(const string &name)
{
    return new W10nShowPathInfoResponseHandler(name);
}

