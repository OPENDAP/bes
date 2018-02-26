// BESCatalogResponseHandler.cc

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

#include "BESCatalogResponseHandler.h"
#include "BESInfoList.h"
#include "BESInfo.h"
#include "BESRequestHandlerList.h"
#include "BESRequestHandler.h"
#include "BESNames.h"
#include "BESDataNames.h"
#include "BESCatalogList.h"
#include "BESCatalog.h"
#include "BESCatalogEntry.h"
#include "BESCatalogUtils.h"
#include "BESSyntaxUserError.h"
#include "BESNotFoundError.h"
#include "BESDebug.h"
#include "BESStopWatch.h"

BESCatalogResponseHandler::BESCatalogResponseHandler(const string &name) :
    BESResponseHandler(name)
{
}

BESCatalogResponseHandler::~BESCatalogResponseHandler()
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
void BESCatalogResponseHandler::execute(BESDataHandlerInterface &dhi)
{

    BESStopWatch sw;
    if (BESISDEBUG(TIMING_LOG)) sw.start("BESCatalogResponseHandler::execute", dhi.data[REQUEST_ID]);

    BESInfo *info = BESInfoList::TheList()->build_info();
    d_response_object = info;

    string container = dhi.data[CONTAINER];
    string catname;
    string defcatname = BESCatalogList::TheCatalogList()->default_catalog();
    BESCatalog *defcat = BESCatalogList::TheCatalogList()->find_catalog(defcatname);
    if (!defcat) {
        string err = (string) "Not able to find the default catalog " + defcatname;
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    // remove all of the leading slashes from the container name
    string::size_type notslash = container.find_first_not_of("/", 0);
    if (notslash != string::npos) {
        container = container.substr(notslash);
    }

    // see if there is a catalog name here. It's only a possible catalog
    // name
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

    string coi = dhi.data[CATALOG_OR_INFO];

    BESCatalogEntry *entry = 0;
    if (catobj) {
        entry = catobj->show_catalog(container, coi, entry);
    }
    else {
        // we always want to get the container information from the
        // default catalog, whether the node is / or not
        entry = defcat->show_catalog(container, coi, entry);

        // we only care to get the list of catalogs if the container is
        // slash (/)
        int num_cats = BESCatalogList::TheCatalogList()->num_catalogs();
        if (container == "/" && num_cats > 1) {
            entry = BESCatalogList::TheCatalogList()->show_catalogs(entry, false);
        }
    }

    if (!entry) {
        string err = (string) "Failed to find node " + container;
        throw BESNotFoundError(err, __FILE__, __LINE__);
    }

    // now that we have all the catalog entry information, display it
    // start the response depending on if show catalog or show info
    if (coi == CATALOG_RESPONSE) {
        info->begin_response(CATALOG_RESPONSE_STR, dhi);
        dhi.action_name = CATALOG_RESPONSE_STR;
    }
    else {
        info->begin_response(SHOW_INFO_RESPONSE_STR, dhi);
        dhi.action_name = SHOW_INFO_RESPONSE_STR;
    }

    // start with the first level entry
    BESCatalogUtils::display_entry(entry, info);

    // if we are doing a catalog response, then go one deeper
    if (coi == CATALOG_RESPONSE) {
        BESCatalogEntry::catalog_citer ei = entry->get_beginning_entry();
        BESCatalogEntry::catalog_citer ee = entry->get_ending_entry();
        for (; ei != ee; ei++) {
            BESCatalogUtils::display_entry((*ei).second, info);
            info->end_tag("dataset");
        }
    }
    info->end_tag("dataset");

    // end the response object
    info->end_response();

    delete entry;
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
void BESCatalogResponseHandler::transmit(BESTransmitter *transmitter, BESDataHandlerInterface &dhi)
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
void BESCatalogResponseHandler::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESCatalogResponseHandler::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESResponseHandler::dump(strm);
    BESIndent::UnIndent();
}

BESResponseHandler *
BESCatalogResponseHandler::CatalogResponseBuilder(const string &name)
{
    return new BESCatalogResponseHandler(name);
}

