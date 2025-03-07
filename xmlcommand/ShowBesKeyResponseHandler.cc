// -*- mode: c++; c-basic-offset:4 -*-
//
// ShowBesKeyResponseHandler.cc
//
// This file is part of the BES default command set
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
#include "ShowBesKeyResponseHandler.h"

#include "BESDebug.h"
#include "BESError.h"
#include "BESInfo.h"
#include "BESInfoList.h"
#include "BESDataNames.h"
#include "TheBESKeys.h"

#include "BESStopWatch.h"

using namespace std;

ShowBesKeyResponseHandler::ShowBesKeyResponseHandler(const string &name) :
    BESResponseHandler(name)
{
}

ShowBesKeyResponseHandler::~ShowBesKeyResponseHandler()
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
void ShowBesKeyResponseHandler::execute(BESDataHandlerInterface &dhi)
{

    BESStopWatch sw;
    if (BESDebug::IsSet(TIMING_LOG_KEY)) sw.start("ShowBesKeyResponseHandler::execute", dhi.data[REQUEST_ID_KEY]);

    BESInfo *info = BESInfoList::TheList()->build_info();
    d_response_object = info;

    string requested_bes_key = dhi.data[BES_KEY];

    BESDEBUG(SBK_DEBUG_KEY, __func__ << "() - requested key: " << requested_bes_key << endl);

    vector<string> key_values;
    getBesKeyValue(requested_bes_key, key_values);

    map<string, string, std::less<>> attrs;

    attrs[KEY] = requested_bes_key;

    info->begin_response(SHOW_BES_KEY_RESPONSE_STR, &attrs, dhi);

    // I think we can replace/remove 'emptyAttrs' and pass nullptr in its place.
    // jhrg 11/26/18
    map<string, string, std::less<>> emptyAttrs;
    for(unsigned long i = 0; i < key_values.size(); ++i)
        info->add_tag("value", key_values[i], &emptyAttrs);

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
void ShowBesKeyResponseHandler::transmit(BESTransmitter *transmitter, BESDataHandlerInterface &dhi)
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
void ShowBesKeyResponseHandler::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "ShowBesKeyResponseHandler::dump - (" << (void *) this << ")" << std::endl;
    BESIndent::Indent();
    BESResponseHandler::dump(strm);
    BESIndent::UnIndent();
}

BESResponseHandler *
ShowBesKeyResponseHandler::ShowBesKeyResponseBuilder(const string &name)
{
    return new ShowBesKeyResponseHandler(name);
}

void ShowBesKeyResponseHandler::getBesKeyValue(string key,  vector<string> &values)
{
    bool found;

    TheBESKeys::TheKeys()->get_values(key, values, found);
    if (!found) {
        BESDEBUG(SBK_DEBUG_KEY, __func__ << "() Failed to located BES key '" << key << "'" << endl);
        throw BESError("Ouch! The Key name '"+key+"' was not found in BESKeys",BES_NOT_FOUND_ERROR, __FILE__, __LINE__);
   }
}

