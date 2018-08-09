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
/** @brief Execute the showCatalog command.
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
    string prolog = string("ShowNodeResponseHandler::") + __func__ + "() - ";
    BESStopWatch sw;
    if (BESISDEBUG(TIMING_LOG)) sw.start("ShowNodeResponseHandler::execute", dhi.data[REQUEST_ID]);

    // Get the container. By convention, the path can start with a slash,
    // but doesn't have too. However, get_node() requires the leading '/'.
    string container = dhi.data[CONTAINER];
    if (container[0] != '/') container = string("/").append(container);
    BESDEBUG(MODULE, prolog << "container: " << container << endl);

    //---------------------------------------------------------------
    // In this next section we look for a catalog name in the first
    // node of the container/file/resource name/path. If we find that
    // the name/path starts with a catalog name then we utilize the
    // specified catalog and we remove the catalog name from the
    // name/path and store the result in the variable "use_container".

    BESCatalogList *datCatalogList = BESCatalogList::TheCatalogList(); // convenience variable
    BESCatalog *catalog = 0;    // pointer to a singleton; do not delete
    vector<string> path_tokens;

    // Because we may need to alter the container/file/resource name by removing
    // a catalog name from the first node in the path we use "use_container" to store
    // the altered container path.
    string use_container = container;
    // Normalize removes duplicate separators and tidys up leading and trailing separators as directed.
    string path = BESUtil::normalize_path(container, false, false);
    BESDEBUG(MODULE, prolog << "Normalized path: " << path << endl);
    // Breaks path into tokens
    BESUtil::tokenize(path, path_tokens);
    if(!path_tokens.empty()){
        BESDEBUG(MODULE, "First path token: " << path_tokens[0] << endl);
        catalog = datCatalogList->find_catalog(path_tokens[0]);
        if(catalog){
            BESDEBUG(MODULE, prolog << "Located catalog " << catalog->get_catalog_name() << " from path component" << endl);
           // Since the catalog name is in the path we
            // need to drop it this should leave container
            // with a leading
            use_container = BESUtil::normalize_path(path.substr(path_tokens[0].length()), true, false);
            BESDEBUG(MODULE, prolog << "Modified container/path value to:  " << use_container << endl);
        }
    }

    if(!catalog){
        // No obvious catalog name in the path, so we go with the default catalog..
        catalog = datCatalogList->default_catalog();
        if (!catalog)
            throw BESSyntaxUserError(string("Could not find the default catalog."), __FILE__, __LINE__);
    }
    BESDEBUG(MODULE, prolog << "Using the '" << catalog->get_catalog_name()  << "' catalog."<< endl);
    //---------------------------------------------------------------

#if 0

    string catname = dhi.data[CATALOG];
    BESCatalog *catalog = 0;    // pointer to a singleton; do not delete
    if (!catname.empty()) {
        catalog = BESCatalogList::TheCatalogList()->find_catalog(catname);
        if (!catalog)
            throw BESSyntaxUserError(string("Could not find catalog: ").append(catname), __FILE__, __LINE__);
    }
    else {
        catalog = BESCatalogList::TheCatalogList()->default_catalog();
        if (!catalog)
            throw BESSyntaxUserError(string("Could not find the default catalog."), __FILE__, __LINE__);
    }
#endif

    BESDEBUG(MODULE, prolog << "use_container: " << use_container << endl);

    // Get the node info from the catalog.
    auto_ptr<CatalogNode> node(catalog->get_node(use_container));

    //---------------------------------------------------------------
    // Now, if this is the top level catalog (which is always the default catalog)
    // we need to add the other catalogs, if any, as child nodes.
    // We check 'container', the unmodified name from the dhi.data[], to see if this is the top
    // catalog
    if(container == "/"){
        BESDEBUG(MODULE, prolog << "Preparing to add alternate catalog nodes to top level of default catalog. "
                "(default: " << (void *) datCatalogList->default_catalog() << ")" << endl);
        BESCatalogList::catalog_citer catItr;
        for(catItr=datCatalogList->first_catalog(); catItr!=datCatalogList->end_catalog(); catItr++){
            string catalog_name = catItr->first;
            BESCatalog *catalog = catItr->second;
            BESDEBUG(MODULE, prolog << "Checking catalog '" << catalog_name << "' ptr: " << (void *) catalog  << endl);
            if(catalog != datCatalogList->default_catalog()){
                CatalogItem *collection = new CatalogItem();
                collection->set_type(CatalogItem::node);
                collection->set_name(catalog_name);
                collection->set_is_data(false);
                collection->set_lmt(BESUtil::get_time());
                collection->set_size(0);
                node->add_node(collection);
                BESDEBUG(MODULE, prolog << "Added catalog node " << catalog_name << " to top level node." << endl);
            }
        }
    }
    //---------------------------------------------------------------

    BESInfo *info = BESInfoList::TheList()->build_info();

    // Transfer the catalog's node info to the BESInfo object
    info->begin_response(NODE_RESPONSE_STR, dhi);

    node->encode_node(info);    // calls encode_item()

    // end the response object
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

