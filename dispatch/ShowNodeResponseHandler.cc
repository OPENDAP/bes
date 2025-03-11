// ShowNodeResponseHandler.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2018 OPeNDAP, Inc
// Author: James Gallagher <jgallagher@opendap.org>
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

#include <memory>
#include <string>

#include "BESInfoList.h"
#include "BESInfo.h"
#include "BESCatalogList.h"
#include "BESCatalog.h"

#include "BESNames.h"
#include "BESDataNames.h"

#include "BESDebug.h"
#include "BESUtil.h"
#include "BESStopWatch.h"
#include "BESSyntaxUserError.h"

#include "CatalogNode.h"
#include "CatalogItem.h"
#include "ShowNodeResponseHandler.h"

using namespace bes;
using namespace std;

#define MODULE "bes"
#define prolog string("ShowNodeResponseHandler::").append(__func__).append("() - ")

/** @brief Execute the showNode command.
 *
 * The response object BESInfo is created to store the information.
 *
 * @param dhi structure that holds request and response information
 * @see BESDataHandlerInterface
 * @see BESInfo
 * @see BESRequestHandlerList
 */
void ShowNodeResponseHandler::execute(BESDataHandlerInterface &dhi)
{
    BES_STOPWATCH_START_DHI(MODULE, prolog + "Timing", &dhi);

    // Get the container. By convention, the path can start with a slash,
    // but doesn't have too. However, get_node() requires the leading '/'.
    string container = dhi.data[CONTAINER];

    BESDEBUG(MODULE, prolog << "Requested container: " << container << endl);

    // Look for the name of a catalog in the first part of the pathname in
    // 'container' and, if found, set 'catalog' to that catalog. The value
    // null is used as a sentinel that the path in 'container' does not
    // name a catalog.
    BESCatalog *catalog = 0;

    vector<string> path_tokens;
    BESUtil::tokenize(container, path_tokens);
    if (!path_tokens.empty()) {

        catalog = BESCatalogList::TheCatalogList()->find_catalog(path_tokens[0]);
        if (catalog) {
            string catalog_name = catalog->get_catalog_name();

            // Remove the catalog name from the start of 'container.' Now 'container'
            // is a relative path within the catalog.
            container = container.substr(container.find(catalog_name) + catalog_name.size());

            BESDEBUG(MODULE, prolog << "Modified container/path value to:  " << container << endl);
        }
    }

    // if we found no catalog name in the first part of the container name/path,
    // use the default catalog.
    if (!catalog) {
        catalog = BESCatalogList::TheCatalogList()->default_catalog();
        if (!catalog) throw BESInternalError(string("Could not find the default catalog."), __FILE__, __LINE__);
    }

    BESDEBUG(MODULE, prolog << "Using the '" << catalog->get_catalog_name() << "' catalog."<< endl);
    BESDEBUG(MODULE, prolog << "use_container: " << container << endl);

    // Get the node info from the catalog.
    unique_ptr<CatalogNode> node(catalog->get_node(container));

    // If the path name passed into this command is '/' we are using the default
    // catalog and must add any other catalogs to the set of nodes shown at the
    // top level. Since the variable 'container' may have been modified above,
    // use the value of dhi.data[CONTAINER] for the path passed to the command.
    if (dhi.data[CONTAINER] == "/") {

        BESDEBUG(MODULE, prolog << "Adding additional catalog nodes to top level node." << endl);

        auto i = BESCatalogList::TheCatalogList()->first_catalog();
        auto e = BESCatalogList::TheCatalogList()->end_catalog();
        for (; i != e; i++) {
            string catalog_name = i->first;
            BESCatalog *cat = i->second;

            BESDEBUG(MODULE, prolog << "Checking catalog '" << catalog_name << "' ptr: " << (void *) cat << endl);

            if (cat != BESCatalogList::TheCatalogList()->default_catalog()) {
                auto *collection = new CatalogItem(catalog_name, 0, BESUtil::get_time(), false, CatalogItem::node);
                node->add_node(collection);

                BESDEBUG(MODULE, prolog << "Added catalog node " << catalog_name << " to top level node." << endl);
            }
        }
    }

    BESInfo *info = BESInfoList::TheList()->build_info();

    // Transfer the catalog's node info to the BESInfo object
    info->begin_response(NODE_RESPONSE_STR, dhi);

    // calls encode_item()
    node->encode_node(info);

    info->end_response();

    // set the state and return
    dhi.action_name = NODE_RESPONSE_STR;
    d_response_object = info;
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
void ShowNodeResponseHandler::transmit(BESTransmitter *transmitter, BESDataHandlerInterface &dhi)
{
    if (d_response_object) {
        BESInfo *info = dynamic_cast<BESInfo *>(d_response_object);
        if (!info) throw BESInternalError("Expected the Response Object to be a BESInfo instance.", __FILE__, __LINE__);
        info->transmit(transmitter, dhi);
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void ShowNodeResponseHandler::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "ShowNodeResponseHandler::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESResponseHandler::dump(strm);
    BESIndent::UnIndent();
}

BESResponseHandler *
ShowNodeResponseHandler::ShowNodeResponseBuilder(const string &name)
{
    return new ShowNodeResponseHandler(name);
}

