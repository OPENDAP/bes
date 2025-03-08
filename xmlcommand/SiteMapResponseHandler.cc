// -*- mode: c++; c-basic-offset:4 -*-
//
// SiteMapResponseHandler.cc
//
// This file is part of the BES default command set
//
// Copyright (c) 2018 OPeNDAP, Inc.
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
// Please read the full copyright statement in the file COPYRIGHT_URI.
//

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include <cerrno>
#include <cstring>

#include <sstream>
#include <fstream>

#include "ShowBesKeyCommand.h"
#include "SiteMapResponseHandler.h"


#include "BESTextInfo.h"
#include "BESDataNames.h"
#include "BESNames.h"
#include "TheBESKeys.h"
#include "BESCatalog.h"
#include "BESCatalogList.h"

#include "BESError.h"
#include "BESDebug.h"
#include "BESStopWatch.h"

using namespace std;

SiteMapResponseHandler::SiteMapResponseHandler(const string &name) :
    BESResponseHandler(name)
{
}

SiteMapResponseHandler::~SiteMapResponseHandler()
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
void SiteMapResponseHandler::execute(BESDataHandlerInterface &dhi)
{
    BESStopWatch sw;
    if (BESDebug::IsSet(TIMING_LOG_KEY)) sw.start("SiteMapResponseHandler::execute", &dhi);

    // Force this command to use a TextInfo object. The default Info object type
    // is set using a key in bes.conf. jheg 11/27/18
    BESInfo *info = new BESTextInfo(); // BESInfoList::TheList()->build_info();
    d_response_object = info;

    if (dhi.data[SITE_MAP_RESPONSE] != SITE_MAP_RESPONSE)
        throw BESInternalError("Not a Site Map command in SiteMapResponseHandler::execute().", __FILE__, __LINE__);

    // remove trailing '/' if present
    if (*(dhi.data[PREFIX].end()-1) == '/')
        dhi.data[PREFIX].erase(dhi.data[PREFIX].end()-1);

    ostringstream oss;
    const BESCatalogList *catalog_list = BESCatalogList::TheCatalogList();    // convenience variable

    info->begin_response(SITE_MAP_RESPONSE_STR, dhi);

    // If no catalog is named, return the concatenation of the site maps for
    // all of the catalogs. This mimics the way shwoNode returns information
    // for the path '/'.
    if (dhi.data[CATALOG].empty()) {
        BESCatalogList::catalog_citer i = catalog_list->first_catalog();
        BESCatalogList::catalog_citer e = catalog_list->end_catalog();
        for (; i != e; ++i) {
            BESCatalog *catalog = i->second;
            if (!catalog)
                throw BESInternalError(string("Build site map found a null catalog in the catalog list."), __FILE__, __LINE__);

            // Don't add 'default' to the prefix value here. For other catalogs, do add their
            // names to the prefix (i->first is the name of the catalog). This matches the
            // behavior of showNode, where the non-default catalogs 'appear' in the default
            // catalog's top level as 'phantom' directories.
            string prefix = (i->first == catalog_list->default_catalog_name()) ? dhi.data[PREFIX]: dhi.data[PREFIX] + "/" + i->first;
            catalog->get_site_map(prefix, dhi.data[NODE_SUFFIX], dhi.data[LEAF_SUFFIX], oss, "/");

            info->add_data(oss.str());

            // clearing oss to keep the size of the object small.
            // Maybe change get_site_map so it takes an Info object? jhrg 11/28/18
            oss.str("");
            oss.clear();
        }
    }
    else {
        BESCatalog *catalog = catalog_list->find_catalog(dhi.data[CATALOG]);
        if (!catalog)
            throw BESInternalError(string("Build site map could not find the catalog: ") + dhi.data[CATALOG], __FILE__, __LINE__);

        string prefix = (dhi.data[CATALOG] == catalog_list->default_catalog_name()) ? dhi.data[PREFIX]: dhi.data[PREFIX] + "/" + dhi.data[CATALOG];
        catalog->get_site_map(prefix, dhi.data[NODE_SUFFIX], dhi.data[LEAF_SUFFIX], oss, "/");

        info->add_data(oss.str());

        // As above, no sense keeping this in memory any longer than needed
        oss.str("");
        oss.clear();
    }

    info->end_response();
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
void SiteMapResponseHandler::transmit(BESTransmitter *transmitter, BESDataHandlerInterface &dhi)
{
    if (d_response_object) {
        BESInfo *info = dynamic_cast<BESInfo *>(d_response_object);
        if (!info) throw BESInternalError("Could not get the Info object in SiteMapResponseHandler::transmit()", __FILE__, __LINE__);
        info->transmit(transmitter, dhi);
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void SiteMapResponseHandler::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "SiteMapResponseHandler::dump - (" << (void *) this << ")" << std::endl;
    BESIndent::Indent();
    BESResponseHandler::dump(strm);
    BESIndent::UnIndent();
}

BESResponseHandler *
SiteMapResponseHandler::SiteMapResponseBuilder(const string &name)
{
    return new SiteMapResponseHandler(name);
}
