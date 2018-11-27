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


#if 0
#include "BESInfo.h"
#include "BESInfoList.h"
#endif

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
    if (BESISDEBUG(TIMING_LOG)) sw.start("SiteMapResponseHandler::execute", dhi.data[REQUEST_ID]);

    // Force this command to use a TextInfo object. The default Info object type
    // is set using a key in bes.conf. jheg 11/27/18
    BESInfo *info = new BESTextInfo(); // BESInfoList::TheList()->build_info();
    d_response_object = info;

    if (dhi.data[SITE_MAP_RESPONSE] != SITE_MAP_RESPONSE)
        throw BESInternalError("Not a Site Map command in SiteMapResponseHandler::execute().", __FILE__, __LINE__);

    ostringstream oss;

    BESCatalog *catalog = BESCatalogList::TheCatalogList()->find_catalog(dhi.data["catalog"]);
    if (!catalog)
        throw BESInternalError(string("Build site map could not find the catalog: ") + dhi.data["catalog"], __FILE__, __LINE__);

    // remove trailing '/' if present
    if (*(dhi.data[PREFIX].end()-1) == '/')
        dhi.data[PREFIX].erase(dhi.data[PREFIX].end()-1);

    catalog->get_site_map(dhi.data[PREFIX], dhi.data[NODE_SUFFIX], dhi.data[LEAF_SUFFIX], oss, "/");

    info->begin_response(SITE_MAP_RESPONSE_STR, dhi);

    info->add_data(oss.str());

    // end the response object
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
