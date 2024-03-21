// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the OPeNDAP Back-End Server (BES)

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

#include "config.h"

#include <string>
#include <ostream>
#include <sstream>

#include "BESDebug.h"
#include "BESInfo.h"
#include "BESIndent.h"

#include "CatalogItem.h"

using namespace bes;
using namespace std;

#define MODULE "cat"
#define prolog std::string("CatalogItem::").append(__func__).append("() - ")

#define CATALOG_NAME_KEY "name"
#define CATALOG_TYPE_KEY "type"
#define CATALOG_LMT_KEY "lastModified"
#define CATALOG_SIZE_KEY "size"
#define CATALOG_IS_DATA_KEY "isData"
#define CATALOG_DAP_URL_KEY "dap_url"
#define CATALOG_ITEM_TAG "item"
#define CATALOG_DESCRIPTION_KEY "description"

/**
 * @brief Encode this CatalogItem in an info object
 *
 * A CatalogItem is encoded as XML using the following grammar, where
 * XML attributes in square brackets are optional.
 * ~~~{.xml}
 * <item name="path" itemType="leaf|node" lastModified="date T time"
 *       [size="size in bytes" isData="true|false"] >
 * ~~~
 * The <item> element may hold information about leaves or nodes elements.
 *
 * @param info Add information to this instance of BESInfo.
 * @see CatalogItem::encode_item()
 */
void CatalogItem::encode_item(BESInfo *info) const
{
    map<string, string, std::less<>> props;

    props[CATALOG_NAME_KEY] = get_name();
    props[CATALOG_TYPE_KEY] = get_type() == leaf ? "leaf": "node";
    props[CATALOG_LMT_KEY] = get_lmt();
    if (get_type() == leaf) {
        ostringstream oss;
        oss << get_size();
        props[CATALOG_SIZE_KEY] = oss.str();
        props[CATALOG_IS_DATA_KEY] = is_data() ? "true" : "false";
        string dap_service_url = get_dap_service_url();
        BESDEBUG(MODULE,prolog << "dap_service_url: " << dap_service_url << endl );
        if(!dap_service_url.empty()){
            props[CATALOG_DAP_URL_KEY] = dap_service_url;
        }
    }

    info->begin_tag(CATALOG_ITEM_TAG, &props);
    string description = get_description();
    if(!description.empty()){
        map<string, string, std::less<>> description_props;
        info->begin_tag(CATALOG_DESCRIPTION_KEY, &description_props);
        info->add_data(description);
        info->end_tag(CATALOG_DESCRIPTION_KEY);
    }

    info->end_tag(CATALOG_ITEM_TAG);

#if 0
    // TODO Should we support the serviceRef element? jhrg 7/22/18
    list<string> services = entry->get_service_list();
    if (services.size()) {
        list<string>::const_iterator si = services.begin();
        list<string>::const_iterator se = services.end();
        for (; si != se; si++) {
            info->add_tag("serviceRef", (*si));
        }
    }
#endif

}


/**
 * Dump out information about this object
 * @param strm Write to this stream
 */
void CatalogItem::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "CatalogItem::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();

    strm << BESIndent::LMarg << "name: " << d_name << endl;
    strm << BESIndent::LMarg << "size: " << d_size << endl;
    strm << BESIndent::LMarg << "last modified time: " << d_lmt << endl;
    strm << BESIndent::LMarg << "is_data: " << d_is_data << endl;
    strm << BESIndent::LMarg << "type: " << d_type << endl;

    BESIndent::UnIndent();
}
